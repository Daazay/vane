#include "vane/compiler/compiler.h"

#include <stdio.h>

#include "vane/utils/path.h"
#include "vane/utils/terminal.h"
#include "vane/utils/string_builder.h"

#include "vane/ast/ast_visitor/ast_dot_printer.h"
#include "vane/ast/ast_visitor/ast_simple_printer.h"

#include "vane/sema/scope.h"

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

    scope_destroy(compiler->global_scope);

    type_system_destroy(&compiler->ts);
    report_collector_destroy(&compiler->rc);
}

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

typedef enum CompilerPipelineStage CompilerPipelineStage;

enum CompilerPipelineStage {
    COMPILER_PIPE_PARSE_AST,
    COMPILER_PIPE_RESOLVE_IMPORTS,
    COMPILER_PIPE_RESOLVE_SYMBOLS,
    COMPILER_PIPE_BIND_SYMBOLS,
    COMPILER_PIPE_RESOLVE_TYPES,
};

static inline bool compiler_run_upto(Compiler* compiler, CompilerPipelineStage stage) {
    assert(compiler != NULL);

    Package* core_package = compiler_load_core_collection(compiler);
    (void)core_package;

    if (!compiler_parse_source_files(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_PARSE_AST) return !compiler_should_halt(compiler);

    if (!compiler_resolve_imports(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_RESOLVE_IMPORTS) return !compiler_should_halt(compiler);

    // Declare

    if (!compiler_resolve_symbol_decls(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_RESOLVE_SYMBOLS) return !compiler_should_halt(compiler);

    if (!compiler_bind_symbols(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_BIND_SYMBOLS) return !compiler_should_halt(compiler);

    if (!compiler_resolve_types(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_RESOLVE_TYPES) return !compiler_should_halt(compiler);

    return !compiler_should_halt(compiler);
}

bool compiler_run_command(Compiler* compiler) {
    assert(compiler != NULL);

    if (compiler->build_options->command == BUILD_COMMAND_HELP) {
        print_usage("vane");
        return true;
    }

    if (is_string_view_empty(string_get_view(compiler->build_options->root_path))) {
        REPORT_ERROR(&compiler->rc, "driver", "no root path provided.");
        return false;
    }

    // Load entry package
    Package* root = compiler_load_package(compiler, string_get_view(compiler->build_options->root_path));
    if (root == NULL) {
        REPORT_ERROR(&compiler->rc, "driver", "failed to load package at '"SV_FMT"'.", SV_ARG(compiler->build_options->root_path));
        return false;
    }

    bool status = true;
    switch (compiler->build_options->command) {
    case BUILD_COMMAND_PARSE_AST:
        status = compiler_run_upto(compiler, COMPILER_PIPE_PARSE_AST);
        break;

    case BUILD_COMMAND_BUILD:
        status = compiler_run_upto(compiler, COMPILER_PIPE_RESOLVE_TYPES);
        break;

    default:
        REPORT_ERROR(&compiler->rc, "driver", "unsupported command.");
        status = false;
        break;
    }

    if (!status) {
        return false;
    }

    if (compiler->build_options->dump_ast) {
        compiler_dump_ast(compiler);
    }
    if (compiler->build_options->dump_ast_dot) {
        compiler_dump_ast_dot(compiler);
    }
    if (compiler->build_options->dump_symbols) {
        compiler_dump_symbols(compiler);
    }
    if (compiler->build_options->dump_types) {
        compiler_dump_types(compiler);
    }
    return true;
}

StringView compiler_get_collection_path(Compiler* compiler, StringView collection_name) {
    assert(compiler != NULL);
    const StringView* path = hashmap_get(&compiler->build_options->collections, &collection_name);
    return path != NULL ? *path : STRING_VIEW_EMPTY;
}

Package* compiler_load_package(Compiler* compiler, StringView dirpath) {
    assert(compiler != NULL);

    // Resolve absolute path
    String abs_path = path_get_absolute(dirpath);
    StringView abs_path_sv = string_get_view(abs_path);

    StringView package_name = path_get_basename(abs_path_sv);

    REPORT_DEBUG(&compiler->rc, "driver", "discovering packages in '"SV_FMT"'.", SV_ARG(abs_path_sv));

    // Check if this path has already been processed
    Package* existing_package = hashmap_get(&compiler->packages, &abs_path_sv);
    if (existing_package != NULL) {
        REPORT_INFO(&compiler->rc, "driver", "package '" SV_FMT "' is already loaded", SV_ARG(package_name));
        string_destroy(&abs_path);
        return existing_package;
    }

    // Insert a placeholder to prevent duplicate scanning
    hashmap_insert(&compiler->packages, &abs_path, NULL);

    Vector entries = { 0 };
    DirListStatus status = directory_list(abs_path_sv, &entries, false);
    switch (status) {
    case DIR_LIST_OK: break;
    case DIR_LIST_ERR_INVALID_PATH:
        REPORT_ERROR(&compiler->rc, "driver", "invalid path: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_NOT_FOUND:
        REPORT_ERROR(&compiler->rc, "driver", "path not found: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_ACCESS_DENIED:
        REPORT_ERROR(&compiler->rc, "driver", "access denied for path: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_NOT_DIR:
        REPORT_ERROR(&compiler->rc, "driver", "path is not a directory: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_OPEN:
        REPORT_ERROR(&compiler->rc, "driver", "failed to open directory: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_READ:
        REPORT_ERROR(&compiler->rc, "driver", "failed to read directory: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_STAT:
        REPORT_ERROR(&compiler->rc, "driver", "failed to stat directory: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    }

    Package* package = NULL;

    for (u32 i = 0; i < entries.size; ++i) {
        DirEntry* entry = vector_at(entries, i);
        StringView basename = path_get_basename(string_get_view(entry->fullpath));

        // Skip hidden directories
        if (entry->is_dir && string_view_has_prefix_sv(basename, STR_LIT("."))) {
            continue;
        }

        // Skip files without the expected language extension
        if (!entry->is_dir && !string_view_has_suffix_sv(basename, STR_LIT(VANE_LANG_EXT))) {
            continue;
        }

        // Process subdirectories recursively
        if (entry->is_dir) {
            Package* subpackage = compiler_load_package(compiler, string_get_view(entry->fullpath));
            if (subpackage == NULL) {
                REPORT_NOTE(&compiler->rc, "driver", "skipping '"SV_FMT"' (no package created)", SV_ARG(basename));
                continue;
            }

            // Lazily create package for this directory if needed
            if (package == NULL) {
                package = package_create(abs_path_sv);
            }

            vector_push_back(&package->subpackages, &subpackage);
            subpackage->parent_package = package;

            REPORT_INFO(&compiler->rc, "driver", "subpackage '" SV_FMT "' added to package '" SV_FMT "'", SV_ARG(basename), SV_ARG(package_name));
        }
        // Process source files
        else {
            if (package == NULL) {
                package = package_create(abs_path_sv);
            }

            SourceFile* source_file = source_file_create(string_get_view(entry->fullpath), &compiler->rc);
            hashmap_insert(&compiler->source_files, &entry->fullpath, &source_file);

            if (source_file != NULL) {
                vector_push_back(&package->source_files, &source_file);
                source_file->package = package;
                REPORT_INFO(&compiler->rc, "driver", "added source file '" SV_FMT"'.", SV_ARG(basename));
            }

            // Clear entry path since ownership transferred
            entry->fullpath = STRING_EMPTY;
        }
    }

    vector_destroy(&entries);

    if (package != NULL) {
        hashmap_insert(&compiler->packages, &abs_path, &package);
        REPORT_INFO(&compiler->rc, "driver", "registered package '" SV_FMT"'.", SV_ARG(package_name));
    }

    return package;

}

Package* compiler_try_resolve_imported_package(Compiler* compiler, SourceFile* source_file, StringView collection_name, StringView package_path) {
    assert(compiler != NULL && source_file != NULL);

    String import_path = STRING_EMPTY;

    if (!is_string_view_empty(collection_name)) {
        StringView collection_path = compiler_get_collection_path(compiler, collection_name);
        if (is_string_view_empty(collection_path)) {
            REPORT_ERROR(&compiler->rc, "driver", "could not resolve collection with name '" SV_FMT"'.", SV_ARG(collection_name));
            return NULL;
        }

        import_path = path_join_sv(collection_path, package_path);
    }
    else {
        import_path = path_join_sv(source_file->package->path, package_path);
    }

    Package* package = compiler_load_package(compiler, string_get_view(import_path));
    string_destroy(&import_path);

    return package;
}

Package* compiler_load_core_collection(Compiler* compiler) {
    assert(compiler != NULL);

    StringView key = STR_LIT("vane_core");
    StringView core_path = compiler_get_collection_path(compiler, key);
    if (is_string_view_empty(core_path)) {
        REPORT_NOTE(&compiler->rc, "driver", "no 'vane_core' collection configured; skipping core prelude.");
        return NULL;
    }

    REPORT_INFO(&compiler->rc, "driver", "loading core collection from '"SV_FMT"'.", SV_ARG(core_path));

    Package* core_package = compiler_load_package(compiler, core_path);
    if (core_package == NULL) {
        REPORT_ERROR(&compiler->rc, "driver", "failed to load core collection from '"SV_FMT"'.", SV_ARG(core_path));
        return NULL;
    }

    core_package->is_core = true;
    return core_package;
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

    // print "kind" on same line as list dash if requested
    if (as_list_item) {
        indent(indent_lvl);
        printf("- kind: %s\n", scope_kind_get_name(scope->kind));
    }
    else {
        indent(indent_lvl);
        printf("kind: %s\n", scope_kind_get_name(scope->kind));
    }

    // when we printed "- kind: ..." we shift the base indentation by +1
    u32 base = indent_lvl + (as_list_item ? 1u : 0u);

    // optional scope name (functions)
    if (scope->kind == SCOPE_FUNCTION && scope->ast != NULL && scope->ast->symbol != NULL) {
        indent(base);
        printf("name: " SV_FMT "\n", SV_ARG(scope->ast->symbol->name));
    }

    // symbols
    {
        bool has_symbols = (scope->symbols.size > 0);
        indent(base);
        puts("symbols:");
        if (!has_symbols) {
            indent(base + 1);
            puts("[]");
        }
        else {
            // compact items inside function scopes (unchanged behavior)
            bool compact = (scope->kind == SCOPE_FUNCTION);

            HashmapIterator it = hashmap_get_it(&scope->symbols);
            StringView key = STRING_VIEW_EMPTY;
            Symbol* sym = NULL;
            while (hashmap_it_next(&it, &key, &sym)) {
                if (sym == NULL) continue; // keep this bugfix
                compiler_dump_symbol_one(sym, base + 1, compact);
            }
        }
    }

    // child scopes
    {
        bool has_scopes = (scope->scopes.size > 0);
        indent(base);
        puts("scopes:");
        if (!has_scopes) {
            indent(base + 1);
            puts("[]");
        }
        else {
            for (u32 i = 0; i < scope->scopes.size; ++i) {
                Scope* child = vector_at(scope->scopes, i);
                // print child with "- kind: ..." on the same line
                compiler_dump_scope(child, base + 1, true);
            }
        }
    }
}

static void compiler_dump_package_scopes(const Package* package) {
    if (package->scope == NULL || (package->scope->scopes.size == 0 && package->scope->symbols.size == 0)) {
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
        if (pkg == NULL) {
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

/* Alias-friendly printer: keep aliases where encountered, but when we show
   an alias’s target (the RHS after "(= "), print it CANONICALLY (fully expanded)
   to avoid chains like "int (= int (= i32))". */
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

// ---------- dump “types per symbol” the same way you dump symbols ----------

static void compiler_dump_scope_types(const Scope* scope, u32 indent_lvl, bool as_list_item) {
    if (!scope) {
        indent(indent_lvl); puts("kind: unknown"); return;
    }

    // put "kind" on the correct line (mirrors compiler_dump_scope)
    if (as_list_item) {
        indent(indent_lvl); printf("- kind: %s\n", scope_kind_get_name(scope->kind));
    }
    else {
        indent(indent_lvl); printf("kind: %s\n", scope_kind_get_name(scope->kind));
    }

    const u32 base = indent_lvl + (as_list_item ? 1u : 0u);

    // label (function name) if applicable
    if (scope->kind == SCOPE_FUNCTION && scope->ast && scope->ast->symbol) {
        indent(base); printf("name: " SV_FMT "\n", SV_ARG(scope->ast->symbol->name));
    }

    // print symbol types
    {
        bool has_symbols = (scope->symbols.size > 0);
        indent(base); puts("symbol_types:");
        if (!has_symbols) {
            indent(base + 1); puts("[]");
        }
        else {
            HashmapIterator it = hashmap_get_it(&scope->symbols);
            StringView key = STRING_VIEW_EMPTY;
            Symbol* sym = NULL;
            while (hashmap_it_next(&it, &key, &sym)) {
                if (!sym) continue;
                indent(base + 1);
                printf("- { kind: %s, name: " SV_FMT, symbol_kind_get_name(sym->kind), SV_ARG(sym->name));

                // location if available
                if (sym->ast) {
                    printf(", loc: ");
                    print_loc(sym->ast->loc);
                }

                // type if available
                printf(", type: ");
                if (sym->kind == SYMBOL_IMPORT) {
                    printf("<n/a>");
                }
                else if (sym->as.typed.type) {
                    print_type_inline(sym->as.typed.type);
                }
                else {
                    printf("<unset>");
                }
                puts(" }");
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
        if (!pkg || !pkg->scope) continue;
        compiler_dump_package_types(pkg);
    }
}

bool compiler_parse_source_files(Compiler* compiler) {
    assert(compiler != NULL);

    bool is_ok = true;

    HashmapIterator source_file_it = hashmap_get_it(&compiler->source_files);
    SourceFile* source_file = NULL;
    StringView source_file_path = STRING_VIEW_EMPTY;

    while (hashmap_it_next(&source_file_it, &source_file_path, &source_file)) {
        if (source_file == NULL) {
            continue;
        }

        if (!source_file_parse_ast(source_file)) {
            is_ok = false;
        }
    }

    return is_ok;
}

bool compiler_resolve_imports(Compiler* compiler) {
    assert(compiler != NULL);

    bool is_ok = true;

    HashmapIterator source_file_it = hashmap_get_it(&compiler->source_files);
    SourceFile* source_file = NULL;
    while (hashmap_it_next(&source_file_it, NULL, &source_file)) {
        if (source_file == NULL) {
            continue;
        }
        if (!source_file_resolve_imports(source_file, compiler)) {
            is_ok = false;
        }
    }

    return is_ok;
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

        if (!package_resolve_symbol_decls(package, compiler->global_scope)) {
            status = false;
        }
    }

    return status;
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