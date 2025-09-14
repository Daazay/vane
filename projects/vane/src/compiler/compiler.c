#include "vane/compiler/compiler.h"

#include <stdio.h>

#include "vane/utils/path.h"
#include "vane/utils/terminal.h"
#include "vane/utils/string_builder.h"
#include "vane/utils/env.h"

#include "vane/ast/ast_visitor/ast_dot_printer.h"
#include "vane/ast/ast_visitor/ast_simple_printer.h"

#include "vane/sema/scope.h"

typedef enum CompilerPipelineStage CompilerPipelineStage;

enum CompilerPipelineStage {
    COMPILER_PIPE_PARSE_AST = 0,
    COMPILER_PIPE_RESOLVE_IMPORTS,
    COMPILER_PIPE_RESOLVE_SYMBOLS,
    COMPILER_PIPE_BIND_SYMBOLS,
    COMPILER_PIPE_RESOLVE_TYPES,
    COMPILER_PIPE_BUILD_CFGS,
    COMPILER_PIPE_BUILD,
};

static inline bool compiler_should_halt(const Compiler* compiler) {
    // If any error
    if (compiler->rc.sev_count[DIAG_SEV_ERROR] > 0) {
        return true;
    }
    // If any warning and we treat em like errors
    if (compiler->build_options->werror && compiler->rc.sev_count[DIAG_SEV_WARNING] > 0) {
        return true;
    }
    return false;
}

static inline void compiler_dump_requested(const Compiler* compiler) {
    assert(compiler != NULL && compiler->build_options != NULL);

    if (IS_FLAG_SET(compiler->build_options->dump_mask, DUMP_FLAG_AST_TEXT)) {
        //compiler_dump_ast(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->dump_mask, DUMP_FLAG_AST_DOT)) {
        compiler_dump_ast_dot(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->dump_mask, DUMP_FLAG_SYMBOLS)) {
        compiler_dump_symbols(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->dump_mask, DUMP_FLAG_TYPES)) {
        compiler_dump_types(compiler);
    }
}

static inline StringView compiler_resolve_vane_root(Compiler* compiler) {
    assert(compiler != NULL && compiler->build_options != NULL);

    // CLI override: --collection vane_root=<path>
    {
        StringView key = STR_LIT("vane_root");
        StringView sv = compiler_get_collection_path(compiler, key);

        if (!is_string_view_empty(sv)) {
            compiler->build_options->vane_root_path = path_get_absolute(sv);
            return string_get_view(compiler->build_options->vane_root_path);
        }
    }

    // environment: VANE_ROOT
    {
        String env = env_get_var(STR_LIT("VANE_ROOT"));
        if (!is_string_empty(env)) {
            compiler->build_options->vane_root_path = env;
            return string_get_view(compiler->build_options->vane_root_path);
        }
    }

    return STRING_VIEW_EMPTY;
}

static inline DirListStatus compiler_get_entries_in_dir(Vector* entries, StringView path, ReportCollector* rc) {
    assert(entries != NULL);
    DirListStatus status = directory_list(path, entries, false);
    switch (status) {
    case DIR_LIST_OK: break;
    case DIR_LIST_ERR_INVALID_PATH:  REPORT_ERROR(rc, DIAG_DRIVER_FS, "invalid path: '" SV_FMT"'.", SV_ARG(path)); break;
    case DIR_LIST_ERR_NOT_FOUND:     REPORT_ERROR(rc, DIAG_DRIVER_FS, "path not found: '" SV_FMT"'.", SV_ARG(path)); break;
    case DIR_LIST_ERR_ACCESS_DENIED: REPORT_ERROR(rc, DIAG_DRIVER_FS, "access denied for path: '" SV_FMT"'.", SV_ARG(path)); break;
    case DIR_LIST_ERR_NOT_DIR:       REPORT_ERROR(rc, DIAG_DRIVER_FS, "path is not a directory: '" SV_FMT"'.", SV_ARG(path)); break;
    case DIR_LIST_ERR_OPEN:          REPORT_ERROR(rc, DIAG_DRIVER_FS, "failed to open directory: '" SV_FMT"'.", SV_ARG(path)); break;
    case DIR_LIST_ERR_READ:          REPORT_ERROR(rc, DIAG_DRIVER_FS, "failed to read directory: '" SV_FMT"'.", SV_ARG(path)); break;
    case DIR_LIST_ERR_STAT:          REPORT_ERROR(rc, DIAG_DRIVER_FS, "failed to stat directory: '" SV_FMT"'.", SV_ARG(path)); break;
    default: break;
    }

    return status;
}

static inline Package* compiler_find_subpackage_named(const Package* parent, StringView name) {
    if (parent == NULL) {
        return NULL;
    }

    for (u32 i = 0; i < parent->subpackages.size; ++i) {
        Package* p = vector_at(parent->subpackages, i);
        StringView base = path_get_basename(p->path);
        if (string_view_eq_sv(base, name)) {
            return p;
        }
    }

    return NULL;
}

static inline void compiler_setup_prelude_scope(Compiler* compiler, Package* core) {
    assert(compiler != NULL);
    assert(compiler->global_scope != NULL);

    if (compiler->prelude_scope != NULL) {
        return;
    }

    Scope* chain_tail = compiler->global_scope;

    if (core != NULL) {
        Package* base = compiler_find_subpackage_named(core, STR_LIT("base"));
        Package* builtin = compiler_find_subpackage_named(base, STR_LIT("builtin"));

        // create builtin package scope chain first so prelude can parent it
        if (builtin != NULL) {
            if (!package_resolve_symbol_decls(builtin, compiler->global_scope)) {
                REPORT_ERROR(&compiler->rc, DIAG_SEMA_SYMBOLS, "failed to create core builtin scope.");
            }
            else {
                chain_tail = builtin->scope;
            }
        }
    }

    compiler->prelude_scope = scope_create(SCOPE_PRELUDE, chain_tail, NULL);
    REPORT_DEBUG(&compiler->rc, DIAG_SEMA_SYMBOLS, "created prelude scope.");
}

static inline bool compiler_populate_global_builtins(Compiler* compiler) {
    assert(compiler != NULL);
    assert(compiler->global_scope != NULL);

#define ADD_BUILTIN(NAME, KIND) do { \
    Symbol* symbol = symbol_create(SYMBOL_TYPEALIAS, STR_LIT(NAME), NULL); \
    symbol->as.typed.type = &compiler->ts.builtin_types[(KIND)]; \
    symbol->as.typed.type_state = TYPE_STATE_RESOLVED; \
    scope_add_symbol(compiler->global_scope, symbol); \
} while (false)

    ADD_BUILTIN("void", TYPE_BUILTIN_VOID);
    ADD_BUILTIN("bool", TYPE_BUILTIN_BOOL);
    ADD_BUILTIN("any", TYPE_BUILTIN_ANY);

    ADD_BUILTIN("u8", TYPE_BUILTIN_U8);
    ADD_BUILTIN("i8", TYPE_BUILTIN_I8);
    ADD_BUILTIN("u16", TYPE_BUILTIN_U16);
    ADD_BUILTIN("i16", TYPE_BUILTIN_I16);
    ADD_BUILTIN("u32", TYPE_BUILTIN_U32);
    ADD_BUILTIN("i32", TYPE_BUILTIN_I32);
    ADD_BUILTIN("u64", TYPE_BUILTIN_U64);
    ADD_BUILTIN("i64", TYPE_BUILTIN_I64);

#undef ADD_BUILTIN

    REPORT_DEBUG(&compiler->rc, DIAG_SEMA_TYPES, "populated global builtins.");
    return true;
}
static inline bool compiler_run_upto(Compiler* compiler, CompilerPipelineStage stage) {
    assert(compiler != NULL);

    Package* core = compiler_load_core_collection(compiler);

    if (!compiler_parse_source_files(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_PARSE_AST) return !compiler_should_halt(compiler);

    if (!compiler_resolve_imports(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_RESOLVE_IMPORTS) return !compiler_should_halt(compiler);

    compiler_setup_prelude_scope(compiler, core);

    if (!compiler_resolve_symbol_decls(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_RESOLVE_SYMBOLS) return !compiler_should_halt(compiler);

    if (!compiler_bind_symbols(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_BIND_SYMBOLS) return !compiler_should_halt(compiler);

    if (!compiler_resolve_types(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_RESOLVE_TYPES) return !compiler_should_halt(compiler);

    if (!compiler_build_cfgs(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_BUILD_CFGS) return !compiler_should_halt(compiler);

    return !compiler_should_halt(compiler);
}

Compiler compiler_create(BuildOptions* build_options) {
    assert(build_options != NULL);

    Compiler compiler = { 0 };
    compiler.build_options = build_options;

    compiler.packages = hashmap_create(COMPILER_DEFAULT_PACKAGE_COUNT,
        HASHMAP_KEY_SPECS(String, &string_view_item_hash, &string_view_item_eq, &string_destroy),
        HASHMAP_VALUE_SPECS(Package*, &package_destroy)
    );

    compiler.source_files = hashmap_create(COMPILER_DEFAULT_SOURCE_FILE_COUNT,
        HASHMAP_KEY_SPECS(String, &string_view_item_hash, &string_view_item_eq, &string_destroy),
        HASHMAP_VALUE_SPECS(SourceFile*, &source_file_destroy)
    );

    compiler.source_files_queue = vector_create(
        COMPILER_DEFAULT_SOURCE_FILE_QUEUE_SIZE,
        VECTOR_SPECS(SourceFile*, NULL)
    );

    compiler.entry_point = NULL;
    compiler.global_scope = scope_create(SCOPE_GLOBAL, NULL, NULL);

    compiler.ts = (TypeSystem) { 0 };

    compiler.rc = report_collector_create(build_options->log_verbosity);

    return compiler;
}

void compiler_destroy(Compiler* compiler) {
    if (compiler == NULL) {
        return;
    }

    hashmap_destroy(&compiler->packages);
    hashmap_destroy(&compiler->source_files);
    vector_destroy(&compiler->source_files_queue);

    scope_destroy(compiler->global_scope);

    type_system_destroy(&compiler->ts);
    report_collector_destroy(&compiler->rc);
}

bool compiler_run_command(Compiler* compiler) {
    assert(compiler != NULL && compiler->build_options != NULL);

    if (compiler->build_options->command == BUILD_COMMAND_HELP) {
        print_usage("vane");
        return true;
    }

    if (is_string_view_empty(string_get_view(compiler->build_options->project_path))) {
        REPORT_ERROR(&compiler->rc, DIAG_DRIVER_CONFIG, "no package path provided.");
        return false;
    }

    // Load entry package (and its tree). Even for parse/check we want discovery.
    Package* root = compiler_load_package(compiler, string_get_view(compiler->build_options->project_path), false);
    if (root == NULL) {
        REPORT_ERROR(&compiler->rc, DIAG_DRIVER_PROJECT, "failed to load project '"SV_FMT"'.", SV_ARG(compiler->build_options->project_path));
        return false;
    }

    bool status = true;
    switch (compiler->build_options->command) {
    case BUILD_COMMAND_PARSE:
        status = compiler_run_upto(compiler, COMPILER_PIPE_PARSE_AST);
        break;

    case BUILD_COMMAND_CHECK:
        status = compiler_run_upto(compiler, COMPILER_PIPE_RESOLVE_TYPES);
        break;

    case BUILD_COMMAND_CFG:
        status = compiler_run_upto(compiler, COMPILER_PIPE_BUILD_CFGS);
        break;

    case BUILD_COMMAND_BUILD:
        status = compiler_run_upto(compiler, COMPILER_PIPE_BUILD);
        break;

    default:
        REPORT_ERROR(&compiler->rc, DIAG_DRIVER_CONFIG, "unsupported command.");
        status = false;
        break;
    }

    if (!status) {
        return false;
    }

    compiler_dump_requested(compiler);
    return true;
}

StringView compiler_get_collection_path(Compiler* compiler, StringView collection_name) {
    assert(compiler != NULL);
    const StringView* path = hashmap_get(&compiler->build_options->collections, &collection_name);
    return path != NULL ? *path : STRING_VIEW_EMPTY;
}

Package* compiler_load_core_collection(Compiler* compiler) {
    assert(compiler != NULL);

    StringView vane_root = compiler_resolve_vane_root(compiler);
    if (is_string_view_empty(vane_root)) {
        REPORT_NOTE(&compiler->rc, DIAG_DRIVER_PROJECT, "no core collection configured (use --collection vane_root=<path> or set VANE_ROOT).");
        return NULL;
    }

    REPORT_INFO(&compiler->rc, DIAG_DRIVER_PROJECT, "loading core collection from '"SV_FMT"'.", SV_ARG(vane_root));

    Package* core = compiler_load_package(compiler, vane_root, true);
    if (core == NULL) {
        REPORT_ERROR(&compiler->rc, DIAG_DRIVER_PROJECT, "failed to load core collection from '"SV_FMT"'.", SV_ARG(vane_root));
        return NULL;
    }

    core->is_core = true;
    return core;
}

Package* compiler_load_package(Compiler* compiler, StringView dirpath, bool is_core) {
    assert(compiler != NULL);

    // Resolve absolute path
    String abs = path_get_absolute(dirpath);
    StringView abs_sv = string_get_view(abs);
    StringView package_name = path_get_basename(abs_sv);

    REPORT_DEBUG(&compiler->rc, DIAG_DRIVER_FS, "discovering packages in '"SV_FMT"'.", SV_ARG(abs_sv));

    // Already loaded?
    Package* existing = hashmap_get(&compiler->packages, &abs_sv);
    if (existing != NULL) {
        REPORT_INFO(&compiler->rc, DIAG_DRIVER_PROJECT, "package '" SV_FMT "' is already loaded", SV_ARG(package_name));
        string_destroy(&abs);
        return existing;
    }

    // Placeholder to prevent reentrant duplication during recursion
    hashmap_insert(&compiler->packages, &abs, NULL);

    Vector entries = { 0 };
    if (compiler_get_entries_in_dir(&entries, abs_sv, &compiler->rc) != DIR_LIST_OK) {
        return NULL;
    }

    Package* package = NULL;
    for (u32 i = 0; i < entries.size; ++i) {
        DirEntry* e = vector_at(entries, i);
        StringView base = path_get_basename(string_get_view(e->fullpath));

        // skip hidden directories
        if (e->is_dir && string_view_has_prefix_sv(base, STR_LIT("."))) {
            continue;
        }

        // skip non-source files
        if (!e->is_dir && !string_view_has_suffix_sv(base, STR_LIT(VANE_LANG_EXT))) {
            continue;
        }

        // process subdirectories recursively
        if (e->is_dir) {
            Package* sub = compiler_load_package(compiler, string_get_view(e->fullpath), is_core);
            if (sub == NULL) {
                REPORT_NOTE(&compiler->rc, DIAG_DRIVER_PROJECT, "skipping '"SV_FMT"' (no package created)", SV_ARG(base));
                continue;
            }

            // lazily create package for this directory if needed
            if (package == NULL) {
                package = package_create(abs_sv, compiler);
            }

            package_add_subpackage(package, sub);

            REPORT_INFO(&compiler->rc, DIAG_DRIVER_PROJECT, "subpackage '" SV_FMT "' added to package '" SV_FMT "'", SV_ARG(base), SV_ARG(package_name));
        }
        // process source files
        else {
            if (package == NULL) {
                package = package_create(abs_sv, compiler);
            }

            SourceFile* source_file = source_file_create(string_get_view(e->fullpath), &compiler->rc);

            hashmap_insert(&compiler->source_files, &e->fullpath, &source_file);
            package_add_source_file(package, source_file);

            vector_push_back(&compiler->source_files_queue, &source_file);

            REPORT_INFO(&compiler->rc, DIAG_DRIVER_PROJECT, "added source file '" SV_FMT"'.", SV_ARG(base));

            e->fullpath = STRING_EMPTY;
        }
    }

    vector_destroy(&entries);

    if (package != NULL) {
        package->is_core = is_core;
        hashmap_insert(&compiler->packages, &abs, &package);
        REPORT_INFO(&compiler->rc, DIAG_DRIVER_PROJECT, "registered package '" SV_FMT"'.", SV_ARG(package_name));
    }

    return package;
}

Package* compiler_resolve_import(Compiler* compiler, SourceFile* source_file, const ImportEntry* e) {
    assert(compiler != NULL && source_file != NULL && e != NULL);
    assert(source_file->package != NULL);

    String import_path = STRING_EMPTY;

    switch (e->base) {
    case IMPORT_BASE_COLLECTION: {
        StringView root = compiler_get_collection_path(compiler, e->collection_name);
        if (is_string_view_empty(root)) {
            REPORT_ERROR(&compiler->rc, DIAG_DRIVER_IMPORTS, "could not resolve collection '"SV_FMT"'.", SV_ARG(e->collection_name));
            return NULL;
        }

        String tmp = path_join_sv(root, e->package_path);
        import_path = path_get_normalized(string_get_view(tmp));
        string_destroy(&tmp);
    } break;

    case IMPORT_BASE_PROJECT_ROOT: {
        if (e->package_path.len == 0) {
            REPORT_ERROR(&compiler->rc, DIAG_DRIVER_IMPORTS, "root import must have a path, e.g. \":grid\".");
            return NULL;
        }

        StringView root_dir = string_get_view(compiler->build_options->project_path);
        if (is_string_view_empty(root_dir)) {
            REPORT_ERROR(&compiler->rc, DIAG_DRIVER_PROJECT, "project root not set.");
            return NULL;
        }

        String tmp = path_join_sv(root_dir, e->package_path);
        import_path = path_get_normalized(string_get_view(tmp));
        string_destroy(&tmp);
    } break;

    case IMPORT_BASE_RELATIVE: {
        StringView base = source_file->package->path;
        String tmp = path_join_sv(base, e->package_path);
        import_path = path_get_normalized(string_get_view(tmp));
        string_destroy(&tmp);
    } break;
    default:
        unreachable();
        break;
    }

    Package* pkg = compiler_load_package(compiler, string_get_view(import_path), false);
    string_destroy(&import_path);
    return pkg;
}

void compiler_dump_ast(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator package_it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&package_it, NULL, &package)) {
        if (package == NULL) continue;

        for (u32 i = 0; i < package->source_files.size; ++i) {
            const SourceFile* sf = vector_at(package->source_files, i);
            printf("[file: " SV_FMT "]\n", SV_ARG(sf->path));
            ast_print_simple(sf->ast, stdout);
        }
    }
}

void compiler_dump_ast_dot(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator package_it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&package_it, NULL, &package)) {
        if (package == NULL) continue;

        for (u32 i = 0; i < package->source_files.size; ++i) {
            const SourceFile* sf = vector_at(package->source_files, i);
            printf("[file: " SV_FMT "]\n", SV_ARG(sf->path));
            ast_print_dot(sf->ast, stdout);
        }
    }
}

static inline void indent(u32 level) {
    for (u32 i = 0; i < level; ++i) {
        printf("  ");
    }
}

static inline void print_loc(TokenLoc loc) {
    printf(SV_FMT ":%d:%d-%d:%d", SV_ARG(loc.path),
        loc.begin.line, loc.begin.column,
        loc.end.line, loc.end.column
    );
}

static inline void print_sv_quoted(StringView sv) {
    printf("\"" SV_FMT "\"", SV_ARG(sv));
}

static inline void compiler_dump_symbol_one(const Symbol* sym, u32 indent_lvl, bool compact) {
    if (compact) {
        indent(indent_lvl);
        printf("- { kind: %s, name: " SV_FMT, symbol_kind_get_name(sym->kind), SV_ARG(sym->name));
        if (sym->kind == SYMBOL_IMPORT && sym->as.import.target != NULL) {
            printf(", target: " SV_FMT, SV_ARG(sym->as.import.target->path));
        }
        if (sym->ast != NULL) {
            printf(", loc: ");
            print_loc(sym->ast->loc);
        }
        puts(" }");
        return;
    }

    // multi-line block mapping
    indent(indent_lvl);
    printf("- kind: %s\n", symbol_kind_get_name(sym->kind));

    indent(indent_lvl + 1);
    printf("name: " SV_FMT "\n", SV_ARG(sym->name));

    if (sym->kind == SYMBOL_IMPORT && sym->as.import.target != NULL) {
        indent(indent_lvl + 1);
        printf("target: " SV_FMT "\n", SV_ARG(sym->as.import.target->path));
    }

    if (sym->ast != NULL) {
        indent(indent_lvl + 1);
        printf("loc: ");
        print_loc(sym->ast->loc);
        putchar('\n');
    }
}

static void compiler_dump_scope(const Scope* scope, u32 indent_lvl, bool as_list_item) {
    if (scope == NULL) {
        indent(indent_lvl);
        puts("kind: unknown");
        return;
    }

    // Header
    if (as_list_item) {
        indent(indent_lvl);
        printf("- kind: %s\n", scope_kind_get_name(scope->kind));
    }
    else {
        indent(indent_lvl);
        printf("kind: %s\n", scope_kind_get_name(scope->kind));
    }

    u32 base = indent_lvl + (as_list_item ? 1u : 0u);

    // optional function name
    if (scope->kind == SCOPE_FUNCTION && scope->ast != NULL && scope->ast->symbol != NULL) {
        indent(base);
        printf("name: " SV_FMT "\n", SV_ARG(scope->ast->symbol->name));
    }

    // symbols
    {
        bool has_symbols = (scope->symbol_sets.size > 0);
        indent(base);
        puts("symbols:");
        if (!has_symbols) {
            indent(base + 1);
            puts("[]");
        }
        else {
            bool compact = (scope->kind == SCOPE_FUNCTION);

            HashmapIterator it = hashmap_get_it(&scope->symbol_sets);
            StringView key = STRING_VIEW_EMPTY;
            SymbolSet set = { 0 };

            while (hashmap_it_next(&it, &key, &set)) {
                for (u32 i = 0; i < SYMBOL_KIND_COUNT - 1; ++i) {
                    Symbol* symbol = set.by_kind[i];
                    if (symbol == NULL) {
                        continue;
                    }

                    compiler_dump_symbol_one(symbol, base + 1, compact);
                }
            }
        }
    }

    // child scopes
    {
        bool has_children = (scope->scopes.size > 0);
        indent(base);
        puts("scopes:");
        if (!has_children) {
            indent(base + 1);
            puts("[]");
        }
        else {
            for (u32 i = 0; i < scope->scopes.size; ++i) {
                Scope* child = vector_at(scope->scopes, i);
                compiler_dump_scope(child, base + 1, true);
            }
        }
    }
}

static void compiler_dump_package_scopes(const Package* package) {
    if (package->scope == NULL || (package->scope->scopes.size == 0 && package->scope->symbol_sets.size == 0)) {
        return;
    }

    printf("package: " SV_FMT "\n", SV_ARG(package->path));
    puts("scope:");
    compiler_dump_scope(package->scope, 1, false);
    putchar('\n');
}

void compiler_dump_symbols(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* pkg = NULL;

    while (hashmap_it_next(&it, NULL, &pkg)) {
        if (pkg == NULL || pkg->is_core) {
            continue;
        }
        compiler_dump_package_scopes(pkg);
    }
}

static void print_type_inline_alias(const Type* t);
static void print_type_inline_canonical(const Type* t);

/* Returns builtin name for comparison/printing; NULL if not a builtin we name directly. */
static const char* builtin_kind_name(const Type* t) {
    if (!t || t->kind != TYPE_BUILTIN) return NULL;
    switch (t->as.builtin.kind) {
    case TYPE_BUILTIN_VOID: return "void";
    case TYPE_BUILTIN_BOOL: return "bool";
    case TYPE_BUILTIN_U8:   return "u8";
    case TYPE_BUILTIN_I8:   return "i8";
    case TYPE_BUILTIN_U16:  return "u16";
    case TYPE_BUILTIN_I16:  return "i16";
    case TYPE_BUILTIN_U32:  return "u32";
    case TYPE_BUILTIN_I32:  return "i32";
    case TYPE_BUILTIN_U64:  return "u64";
    case TYPE_BUILTIN_I64:  return "i64";
    case TYPE_BUILTIN_ANY:  return "any";
    default: return NULL;
    }
}

/* Follow alias chain to a non-alias canonical type (but do not loop forever). */
static const Type* type_canonical(const Type* t) {
    const u32 kMaxDepth = 256;
    u32 depth = 0;
    while (t && t->kind == TYPE_ALIAS && depth++ < kMaxDepth) {
        t = t->as.alias.target;
    }
    return t;
}

/* Print function type canonical: params/ret are canonical too. */
static void print_fun_type_canonical(const Type* t) {
    putchar('(');
    for (u32 i = 0; i < t->as.fun.param_count; ++i) {
        if (i) printf(", ");
        print_type_inline_canonical(t->as.fun.params[i]);
    }
    printf(") -> ");
    print_type_inline_canonical(t->as.fun.ret);
}

/* Print function type alias-friendly: keep aliases on the LHS view. */
static void print_fun_type_alias(const Type* t) {
    putchar('(');
    for (u32 i = 0; i < t->as.fun.param_count; ++i) {
        if (i) printf(", ");
        print_type_inline_alias(t->as.fun.params[i]);
    }
    printf(") -> ");
    print_type_inline_alias(t->as.fun.ret);
}

/* Canonical printer: expands aliases entirely. */
static void print_type_inline_canonical(const Type* t) {
    if (!t) { printf("<null>"); return; }

    const Type* c = type_canonical(t);
    if (!c) { printf("<null>"); return; }

    switch (c->kind) {
    case TYPE_UNRESOLVED:
        printf("<unresolved " SV_FMT ">", SV_ARG(c->as.unresolved.name));
        return;

    case TYPE_BUILTIN: {
        const char* name = builtin_kind_name(c);
        if (name) { printf("%s", name); }
        else { printf("<builtin:%u>", (unsigned)c->as.builtin.kind); }
        return;
    }

    case TYPE_POINTER:
        putchar('^'); print_type_inline_canonical(c->as.pointer.base); return;

    case TYPE_ARRAY:
        printf("[%u]", (unsigned)c->as.array.size);
        print_type_inline_canonical(c->as.array.elem);
        return;

    case TYPE_SLICE:
        printf("[]");
        print_type_inline_canonical(c->as.slice.elem);
        return;

    case TYPE_FUNCTION:
        print_fun_type_canonical(c);
        return;

    case TYPE_ALIAS:
        // Canonical view should never end here, but handle gracefully:
        print_type_inline_canonical(c->as.alias.target);
        return;

    default:
        printf("<type kind:%u>", (unsigned)c->kind);
        return;
    }
}

static void print_type_inline_alias(const Type* t) {
    if (!t) { printf("<null>"); return; }

    switch (t->kind) {
    case TYPE_UNRESOLVED:
        printf("<unresolved " SV_FMT ">", SV_ARG(t->as.unresolved.name));
        return;

    case TYPE_BUILTIN: {
        const char* name = builtin_kind_name(t);
        if (name) { printf("%s", name); }
        else { printf("<builtin:%u>", (unsigned)t->as.builtin.kind); }
        return;
    }

    case TYPE_POINTER:
        putchar('^'); print_type_inline_alias(t->as.pointer.base); return;

    case TYPE_ARRAY:
        printf("[%u]", (unsigned)t->as.array.size);
        print_type_inline_alias(t->as.array.elem);
        return;

    case TYPE_SLICE:
        printf("[]");
        print_type_inline_alias(t->as.slice.elem);
        return;

    case TYPE_FUNCTION:
        print_fun_type_alias(t);
        return;

    case TYPE_ALIAS: {
        // Print alias name as-is.
        printf(SV_FMT, SV_ARG(t->as.alias.name));

        // If alias is effectively a self-alias (e.g., "u8 = u8"), suppress " (= ...)" noise.
        const Type* tgt = t->as.alias.target;
        const char* tgt_builtin = builtin_kind_name(type_canonical(tgt));

        // Compare alias name with canonical builtin name if any.
        bool self_like = false;
        if (tgt_builtin) {
            StringView alias_name = t->as.alias.name;
            if (string_view_eq_sv(alias_name, string_view_from_cstr(tgt_builtin))) {
                self_like = true;
            }
        }
        if (!self_like) {
            printf(" (= ");
            print_type_inline_canonical(tgt);   // de-alias RHS
            putchar(')');
        }
        return;
    }

    default:
        printf("<type kind:%u>", (unsigned)t->kind);
        return;
    }
}

/* Wrapper to match your existing calls */
static void print_type_inline(const Type* t) {
    print_type_inline_alias(t);
}

static void compiler_dump_scope_types(const Scope* scope, u32 indent_lvl, bool as_list_item) {
    if (scope == NULL) {
        indent(indent_lvl);
        puts("kind: unknown");
        return;
    }

    // Header
    if (as_list_item) {
        indent(indent_lvl);
        printf("- kind: %s\n", scope_kind_get_name(scope->kind));
    }
    else {
        indent(indent_lvl);
        printf("kind: %s\n", scope_kind_get_name(scope->kind));
    }

    const u32 base = indent_lvl + (as_list_item ? 1u : 0u);

    // optional function name
    if (scope->kind == SCOPE_FUNCTION && scope->ast != NULL && scope->ast->symbol) {
        indent(base);
        printf("name: " SV_FMT "\n", SV_ARG(scope->ast->symbol->name));
    }

    // print symbol types
    {
        bool has_symbols = (scope->symbol_sets.size > 0);
        indent(base);
        puts("symbol_types:");
        if (!has_symbols) {
            indent(base + 1);
            puts("[]");
        }
        else {
            HashmapIterator it = hashmap_get_it(&scope->symbol_sets);
            StringView key = STRING_VIEW_EMPTY;
            SymbolSet set = {0};

            while (hashmap_it_next(&it, &key, &set)) {
                for (u32 i = 0; i < SYMBOL_KIND_COUNT - 1; ++i) {
                    Symbol* symbol = set.by_kind[i];
                    if (symbol == NULL) {
                        continue;
                    }

                    indent(base + 1);
                    printf("- { kind: %s, name: " SV_FMT, symbol_kind_get_name(symbol->kind), SV_ARG(symbol->name));


                    if (symbol->ast != NULL) {
                        printf(", loc: ");
                        print_loc(symbol->ast->loc);
                    }

                    // type if available
                    printf(", type: ");
                    if (symbol->kind == SYMBOL_IMPORT) {
                        printf("<n/a>");
                    }
                    else if (symbol->as.typed.type) {
                        print_type_inline(symbol->as.typed.type);
                    }
                    else {
                        printf("<unset>");
                    }
                    puts(" }");
                }
            }
        }
    }

    // recurse into child scopes
    {
        bool has_scopes = (scope->scopes.size > 0);
        indent(base); puts("scopes:");
        if (!has_scopes) {
            indent(base + 1); puts("[]");
        }
        else {
            for (u32 i = 0; i < scope->scopes.size; ++i) {
                const Scope* child = vector_at(scope->scopes, i);
                compiler_dump_scope_types(child, base + 1, true);
            }
        }
    }
}

static void compiler_dump_package_types(const Package* package) {
    if (!package || !package->scope) return;
    printf("package: " SV_FMT "\n", SV_ARG(package->path));
    puts("scope:");
    compiler_dump_scope_types(package->scope, 1, false);
    putchar('\n');
}

void compiler_dump_types(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* pkg = NULL;

    while (hashmap_it_next(&it, NULL, &pkg)) {
        if (pkg == NULL || pkg->scope == NULL) {
            continue;
        }

        if (pkg->is_core) {
            continue;
        }
        compiler_dump_package_types(pkg);
    }
}

bool compiler_parse_source_files(Compiler* compiler) {
    assert(compiler != NULL);

    bool is_ok = true;

    while (compiler->source_files_queue.size > 0) {
        SourceFile* source_file = vector_at_back(compiler->source_files_queue);
        vector_pop_back(&compiler->source_files_queue);

        if (!source_file_parse_ast(source_file)) {
            is_ok = false;
        }
    }

    return is_ok;
}

static inline bool compiler_drain_parse_queue(Compiler* compiler, Vector* resolve_queue) {
    assert(compiler != NULL && resolve_queue != NULL);

    bool is_good = true;

    while (compiler->source_files_queue.size > 0) {
        SourceFile* source_file = vector_at_back(compiler->source_files_queue);
        vector_pop_back(&compiler->source_files_queue);

        if (!source_file_parse_ast(source_file)) {
            is_good = false;
        }

        if (source_file->ast != NULL && !source_file->imports_resolved) {
            vector_push_back(resolve_queue, &source_file);
        }
    }

    return is_good;
}

static inline bool compiler_drain_resolve_queue(Compiler* compiler, Vector* resolve_queue) {
    assert(compiler != NULL && resolve_queue != NULL);

    bool is_good = true;

    while (resolve_queue->size > 0) {
        SourceFile* source_file = vector_at_back(*resolve_queue);
        vector_pop_back(resolve_queue);

        if (source_file->imports_resolved || source_file->ast == NULL) {
            continue;
        }

        if (!source_file_resolve_imports(source_file, compiler)) {
            is_good = false;
        }

        source_file->imports_resolved = true;
    }

    return is_good;
}

bool compiler_resolve_imports(Compiler* compiler) {
    assert(compiler != NULL);

    bool is_good = true;

    Vector resolve_queue = vector_create(8, VECTOR_SPECS(SourceFile*, NULL));

    {
        HashmapIterator it = hashmap_get_it(&compiler->source_files);
        SourceFile* source_file = NULL;

        while (hashmap_it_next(&it, NULL, &source_file)) {
            if (source_file != NULL && source_file->ast != NULL && !source_file->imports_resolved) {
                vector_push_back(&resolve_queue, &source_file);
            }
        }
    }

    while (compiler->source_files_queue.size > 0 || resolve_queue.size > 0) {
        if (!compiler_drain_parse_queue(compiler, &resolve_queue)) {
            is_good = false;
        }
        if (!compiler_drain_resolve_queue(compiler, &resolve_queue)) {
            is_good = false;
        }
    }

    vector_destroy(&resolve_queue);
    return is_good;
}

bool compiler_resolve_symbol_decls(Compiler* compiler) {
    assert(compiler != NULL);

    bool status = true;

    HashmapIterator package_it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&package_it, NULL, &package)) {
        if (package == NULL) {
            continue;
        }

        if (!package_resolve_symbol_decls(package, compiler->prelude_scope)) {
            status = false;
        }
    }

    return status;
}

static inline bool compiler_populate_global_scope_with_symbols(Compiler* compiler) {
    assert(compiler != NULL);

#define ADD_BUILTIN_TYPE(NAME, BUILTIN_TYPE) do { \
        Symbol* sym = symbol_create(SYMBOL_TYPEALIAS, STR_LIT(NAME), NULL); \
        sym->as.typed.type = &compiler->ts.builtin_types[(BUILTIN_TYPE)]; \
        sym->as.typed.type_state = TYPE_STATE_RESOLVED; \
        scope_add_symbol(compiler->global_scope, sym); \
    } while (false)

    // Map of public names -> compiler builtin kinds
    ADD_BUILTIN_TYPE("void", TYPE_BUILTIN_VOID);
    ADD_BUILTIN_TYPE("bool", TYPE_BUILTIN_BOOL);
    ADD_BUILTIN_TYPE("any",  TYPE_BUILTIN_ANY);

    ADD_BUILTIN_TYPE("u8",  TYPE_BUILTIN_U8);
    ADD_BUILTIN_TYPE("i8",  TYPE_BUILTIN_I8);
    ADD_BUILTIN_TYPE("u16", TYPE_BUILTIN_U16);
    ADD_BUILTIN_TYPE("i16", TYPE_BUILTIN_I16);
    ADD_BUILTIN_TYPE("u32", TYPE_BUILTIN_U32);
    ADD_BUILTIN_TYPE("i32", TYPE_BUILTIN_I32);
    ADD_BUILTIN_TYPE("u64", TYPE_BUILTIN_U64);
    ADD_BUILTIN_TYPE("i64", TYPE_BUILTIN_I64);

#undef ADD_BUILTIN_TYPE
    return true;
}

bool compiler_bind_symbols(Compiler* compiler) {
    assert(compiler != NULL);

    bool status = true;

    HashmapIterator package_it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&package_it, NULL, &package)) {
        if (package == NULL) {
            continue;
        }

        if (!package_bind_symbols(package)) {
            status = false;
        }
    }

    return status;
}

bool compiler_resolve_types(Compiler* compiler) {
    assert(compiler != NULL);

    type_system_init(&compiler->ts, target_infos[TARGET_ARCH_X86_64]);

    bool status = true;

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&it, NULL, &package)) {
        if (package == NULL || package->scope == NULL) {
            continue;
        }
        if (!scope_resolve_types(package->scope, &compiler->ts, &compiler->rc)) {
            status = false;
        }
    }

    return status;
}

bool compiler_build_cfgs(Compiler* compiler) {
    assert(compiler != NULL);

    bool status = true;

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&it, NULL, &package)) {
        if (package == NULL) {
            continue;
        }
        if (!package_build_cfg(package)) {
            status = false;
        }
    }

    return status;
}