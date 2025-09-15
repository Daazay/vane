#include "vane/compiler/compiler.h"

#include <stdio.h>

#include "vane/utils/path.h"
#include "vane/utils/terminal.h"
#include "vane/utils/string_builder.h"
#include "vane/utils/env.h"

#include "vane/ast/ast_visitor.h"

#include "vane/dump/ast_dump.h"
#include "vane/dump/scope_dump.h"
#include "vane/dump/types_dump.h"
#include "vane/dump/cfg_dump.h"
#include "vane/dump/call_graph_dump.h"

#include "vane/sema/scope.h"

typedef enum CompilerPipelineStage CompilerPipelineStage;

enum CompilerPipelineStage {
    COMPILER_PIPE_PARSE_AST = 0,
    COMPILER_PIPE_RESOLVE_IMPORTS,
    COMPILER_PIPE_RESOLVE_SYMBOLS,
    COMPILER_PIPE_BIND_SYMBOLS,
    COMPILER_PIPE_RESOLVE_TYPES,
    COMPILER_PIPE_BUILD_FUN_TYPE_INDEX,
    COMPILER_PIPE_RESOLVE_ENTRY,
    COMPILER_PIPE_VALIDATE_SEMANTIC,
    COMPILER_PIPE_BUILD_CALL_GRAPH,
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

static inline bool compiler_create_emit_directory_if_needed(BuildOptions* build_options, ReportCollector* rc) {
    assert(build_options != NULL && rc != NULL);

    if (!is_path_absolute(string_get_view(build_options->emit_dir))) {
        String abs = path_get_absolute(string_get_view(build_options->emit_dir));
        string_destroy(&build_options->emit_dir);
        build_options->emit_dir = abs;
    }

    if (is_path_exist(string_get_view(build_options->emit_dir))) {
        return true;
    }

    // attempt to create directory
    StringView emit_sv = string_get_view(build_options->emit_dir);
    DirCreateStatus status = directory_create_recursive(emit_sv);
    switch (status) {
    case DIR_CREATE_OK:
        REPORT_DEBUG(rc, DIAG_SEMA, "directory for dumps '" SV_FMT "' was successfully created.", SV_ARG(emit_sv));
        break;
    case DIR_CREATE_ERR_INVALID_PATH:
        REPORT_WARNING(rc, DIAG_SEMA, "failed to create directory for dumps '" SV_FMT "': invalid path.", SV_ARG(emit_sv));
        break;
    case DIR_CREATE_ERR_EXISTS:
        REPORT_DEBUG(rc, DIAG_SEMA, "directory for dumps '" SV_FMT "' already exists.", SV_ARG(emit_sv));
        break;
    case DIR_CREATE_ERR_ACCESS_DENIED:
        REPORT_WARNING(rc, DIAG_SEMA, "failed to create directory for dumps '" SV_FMT "': access denied.", SV_ARG(emit_sv));
        break;
    case DIR_CREATE_ERR_FAILED:
        REPORT_WARNING(rc, DIAG_SEMA, "failed to create directory for dumps '" SV_FMT "'.", SV_ARG(emit_sv));
        break;
    default:
        unreachable();
        break;
    }

    return status == DIR_CREATE_OK || status == DIR_CREATE_ERR_EXISTS;
}

static inline bool compiler_get_package_emit_dir(const Compiler* compiler, const Package* package, String* out_dir) {
    assert(compiler != NULL && package != NULL && out_dir != NULL);

    *out_dir = STRING_EMPTY;

    String rel = path_get_relative(string_get_view(compiler->build_options->project_path), package->path);
    StringView rel_sv = string_get_view(rel);

    if (is_string_view_empty(rel_sv)) {
        rel_sv = path_get_basename(package->path);
    }

    *out_dir = path_join_sv(string_get_view(compiler->build_options->emit_dir), rel_sv);
    StringView out_dir_sv = string_get_view(*out_dir);

    string_destroy(&rel);

    DirCreateStatus status = directory_create_recursive(out_dir_sv);
    switch (status) {
    case DIR_CREATE_OK:
        REPORT_DEBUG((ReportCollector*)&compiler->rc, DIAG_DRIVER_FS, "created directory '" SV_FMT "'.", SV_ARG(out_dir_sv));
        break;
    case DIR_CREATE_ERR_EXISTS:
        REPORT_INFO((ReportCollector*)&compiler->rc, DIAG_DRIVER_FS, "directory '" SV_FMT "' already exists.", SV_ARG(out_dir_sv));
        break;
    case DIR_CREATE_ERR_ACCESS_DENIED:
        REPORT_ERROR((ReportCollector*)&compiler->rc, DIAG_DRIVER_FS, "access denied creating '" SV_FMT "'.", SV_ARG(out_dir_sv));
        break;
    case DIR_CREATE_ERR_INVALID_PATH:
        REPORT_ERROR((ReportCollector*)&compiler->rc, DIAG_DRIVER_FS, "invalid path while creating '" SV_FMT "'.", SV_ARG(out_dir_sv));
        break;

    default:
        unreachable();
        break;
    }
    return status == DIR_CREATE_OK || status == DIR_CREATE_ERR_EXISTS;
}

static inline String compiler_build_package_emit_filepath(StringView emit_dir, StringView name, StringView ext) {
    StringBuilder sb = string_builder_create(name.len + ext.len);
    string_builder_append_sv(&sb, name);
    string_builder_append_sv(&sb, ext);

    StringView fname = string_builder_get_view(sb);
    String fp = path_join_sv(emit_dir, fname);
    string_builder_destroy(&sb);

    return fp;
}

static inline bool compiler_wants_console(const Compiler* c) {
    return c->build_options->emit_out_mode == EMIT_OUT_CONSOLE ||
           c->build_options->emit_out_mode == EMIT_OUT_BOTH;
}
static inline bool compiler_wants_files(const Compiler* c) {
    return c->build_options->emit_out_mode == EMIT_OUT_FILE ||
           c->build_options->emit_out_mode == EMIT_OUT_BOTH;
}

static inline void compiler_emit_requested(const Compiler* compiler) {
    assert(compiler != NULL && compiler->build_options != NULL);

    // Only prepare directory if we will write files.
    if (compiler_wants_files(compiler) && compiler->build_options->emit_mask != EMIT_FLAG_NONE) {
        if (!compiler_create_emit_directory_if_needed(compiler->build_options, (ReportCollector*)&compiler->rc)) {
            REPORT_DEBUG((ReportCollector*)&compiler->rc, DIAG_DRIVER_PROJECT, "failed to prepare dump directory. File dumps will be skipped.");
        }
    }

    if (IS_FLAG_SET(compiler->build_options->emit_mask, EMIT_FLAG_AST_TXT)) {
        compiler_emit_ast_txt(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->emit_mask, EMIT_FLAG_AST_DOT)) {
        compiler_emit_ast_dot(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->emit_mask, EMIT_FLAG_SYMBOLS)) {
        compiler_emit_symbols(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->emit_mask, EMIT_FLAG_TYPES)) {
        compiler_emit_types(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->emit_mask, EMIT_FLAG_CFG_DOT)) {
        compiler_emit_cfg_dot(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->emit_mask, EMIT_FLAG_CALL_GRAPH_TXT)) {
        compiler_emit_call_graph_txt(compiler);
    }
    if (IS_FLAG_SET(compiler->build_options->emit_mask, EMIT_FLAG_CALL_GRAPH_DOT)) {
        compiler_emit_call_graph_dot(compiler);
    }
}

static inline StringView compiler_resolve_vane_root(Compiler* compiler) {
    assert(compiler != NULL && compiler->build_options != NULL);

    // CLI override: --collection vane-root=<path>
    {
        StringView key = STR_LIT("vane-root");
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

    compiler->prelude_scope = scope_create(SCOPE_PRELUDE, chain_tail, NULL, NULL);
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

    if (!compiler_build_fun_type_index(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_BUILD_FUN_TYPE_INDEX) return !compiler_should_halt(compiler);

    if (!compiler_resolve_entry_point(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_RESOLVE_ENTRY) return !compiler_should_halt(compiler);

    if (!compiler_validate_semantics(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_VALIDATE_SEMANTIC) return !compiler_should_halt(compiler);

    if (!compiler_build_call_graph(compiler)) return false;
    if (compiler_should_halt(compiler) || stage == COMPILER_PIPE_BUILD_CALL_GRAPH) return !compiler_should_halt(compiler);

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

    compiler.fun_by_type_hash   = (Hashmap) { 0 };

    compiler.entry_point = NULL;
    compiler.global_scope = scope_create(SCOPE_GLOBAL, NULL, NULL, NULL);

    compiler.ts = (TypeSystem) { 0 };

    compiler.call_graph = NULL;

    compiler.rc = report_collector_create(build_options->log_verbosity);

    return compiler;
}

void compiler_destroy(Compiler* compiler) {
    if (compiler == NULL) {
        return;
    }

    hashmap_destroy(&compiler->packages);
    hashmap_destroy(&compiler->source_files);
    hashmap_destroy(&compiler->fun_by_type_hash);
    vector_destroy(&compiler->source_files_queue);

    scope_destroy(compiler->global_scope);

    call_graph_destroy(compiler->call_graph);


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

    compiler_emit_requested(compiler);
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

static inline bool compiler_open_file_for_write(FileWriter* w, StringView path, bool overwrite_content, ReportCollector* rc) {
    assert(w != NULL && rc != NULL);

    FileWriteStatus status = file_writer_open(w, path, overwrite_content);
    switch (status) {
    case FILE_WRITE_OK:
        REPORT_DEBUG(rc, DIAG_SEMA, "successfully opened file '" SV_FMT "' for writing%s.",
            SV_ARG(path),
            overwrite_content ? " (overwriting existing content)" : ""
        );
        break;
    case FILE_WRITE_ERR_INVALID_PATH:
        REPORT_ERROR(rc, DIAG_SEMA, "cannot open file '" SV_FMT "' for writing: invalid path.",
            SV_ARG(path)
        );
        break;
    case FILE_WRITE_ERR_NOT_FOUND:
        REPORT_ERROR(rc, DIAG_SEMA, "cannot open file '" SV_FMT "' for writing: directory does not exist.",
            SV_ARG(path)
        );
        break;
    case FILE_WRITE_ERR_ACCESS_DENIED:
        REPORT_ERROR(rc, DIAG_SEMA, "cannot open '" SV_FMT "' for writing: path is a directory.",
            SV_ARG(path)
        );
        break;
    case FILE_WRITE_ERR_OPEN:
        REPORT_ERROR(rc, DIAG_SEMA, "failed to open '" SV_FMT "' for writing due to an unknown I/O error.",
            SV_ARG(path)
        );
        break;
    case FILE_WRITE_ERR_WRITE:
        REPORT_ERROR(rc, DIAG_SEMA, "failed to write to file '" SV_FMT "'.", SV_ARG(path));
        break;

    default:
        unreachable();
        break;
    }

    return status == FILE_WRITE_OK;
}

static inline String compiler_build_emit_filepath(const BuildOptions* build_options, StringView name, StringView ext) {
    assert(build_options != NULL);

    StringBuilder sb = string_builder_create(name.len + ext.len);
    string_builder_append_sv(&sb, name);
    string_builder_append_sv(&sb, ext);

    StringView filename = string_builder_get_view(sb);
    String filepath = path_join_sv(string_get_view(build_options->emit_dir), filename);

    string_builder_destroy(&sb);
    return filepath;
}

void compiler_emit_ast_txt(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator package_it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    FileWriter cw = { 0 };
    bool to_console = compiler_wants_console(compiler);
    if (to_console) {
        cw = file_writer_get_stdout();
    }

    while (hashmap_it_next(&package_it, NULL, &package)) {
        if (package == NULL || package->is_core) {
            continue;
        }

        if (to_console) {
            file_writer_write_format(&cw, "package: " SV_FMT "\n", SV_ARG(package->path));
        }

        // prepare per-package emit dir
        String package_out = STRING_EMPTY;
        const bool to_files = compiler_wants_files(compiler) && compiler_get_package_emit_dir(compiler, package, &package_out);

        //
        assert(string_view_contains_sv(string_get_view(package_out), STR_LIT("..")) && "TODO: handle path wich not in current project");

        for (u32 i = 0; i < package->source_files.size; ++i) {
            const SourceFile* source_file = vector_at(package->source_files, i);

            if (to_console) {
                file_writer_write_format(&cw, "file: " SV_FMT "\n", SV_ARG(source_file->path));
                dump_ast_text(&cw, source_file->ast);
                file_writer_write_eol(&cw);
            }

            // file output?
            if (to_files) {
                // keep original filename (stem) inside the per-package folder
                StringView stem = path_get_stem(source_file->path);
                String out_filepath = compiler_build_package_emit_filepath(string_get_view(package_out), stem, STR_LIT(".ast.txt"));

                FileWriter fw = { 0 };
                if (compiler_open_file_for_write(&fw, string_get_view(out_filepath), true, (ReportCollector*)&compiler->rc)) {
                    dump_ast_text(&fw, source_file->ast);
                    file_writer_close(&fw);
                }
                string_destroy(&out_filepath);
            }
        }

        string_destroy(&package_out);

        if (to_console) {
            file_writer_write_eol(&cw);
            file_writer_flush(&cw);
        }
    }
}

void compiler_emit_ast_dot(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator package_it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    FileWriter cw = { 0 };
    bool to_console = compiler_wants_console(compiler);
    if (to_console) {
        cw = file_writer_get_stdout();
    }

    while (hashmap_it_next(&package_it, NULL, &package)) {
        if (package == NULL || package->is_core) {
            continue;
        }

        if (to_console) {
            file_writer_write_format(&cw, "package: " SV_FMT "\n", SV_ARG(package->path));
        }

        // prepare per-package emit dir
        String package_out = STRING_EMPTY;
        const bool to_files = compiler_wants_files(compiler) && compiler_get_package_emit_dir(compiler, package, &package_out);

        assert(string_view_contains_sv(string_get_view(package_out), STR_LIT("..")) && "TODO: handle path wich not in current project");

        for (u32 i = 0; i < package->source_files.size; ++i) {
            const SourceFile* source_file = vector_at(package->source_files, i);

            if (to_console) {
                file_writer_write_format(&cw, "file: " SV_FMT "\n", SV_ARG(source_file->path));
                dump_ast_dot(&cw, source_file->ast);
                file_writer_write_eol(&cw);
            }

            // file output?
            if (to_files) {
                // keep original filename (stem) inside the per-package folder
                StringView stem = path_get_stem(source_file->path);
                String out_filepath = compiler_build_package_emit_filepath(string_get_view(package_out), stem, STR_LIT(".ast.dot"));

                FileWriter fw = { 0 };
                if (compiler_open_file_for_write(&fw, string_get_view(out_filepath), true, (ReportCollector*)&compiler->rc)) {
                    dump_ast_dot(&fw, source_file->ast);
                    file_writer_close(&fw);
                }
                string_destroy(&out_filepath);
            }
        }

        string_destroy(&package_out);

        if (to_console) {
            file_writer_write_eol(&cw);
            file_writer_flush(&cw);
        }
    }
}

static void compiler_emit_package_scopes(const Package* package) {
    if (package->scope == NULL || (package->scope->scopes.size == 0 && package->scope->symbol_sets.size == 0)) {
        return;
    }

    printf("package: " SV_FMT "\n", SV_ARG(package->path));
    puts("scope:");
    dump_scope(package->scope);
    putchar('\n');
}

void compiler_emit_symbols(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* pkg = NULL;

    while (hashmap_it_next(&it, NULL, &pkg)) {
        if (pkg == NULL || pkg->is_core) {
            continue;
        }
        compiler_emit_package_scopes(pkg);
    }
}

static void compiler_emit_package_types(const Package* package) {
    if (!package || !package->scope) return;
    printf("package: " SV_FMT "\n", SV_ARG(package->path));
    puts("scope:");
    dump_types(package->scope);
    putchar('\n');
}

void compiler_emit_types(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&it, NULL, &package)) {
        if (package == NULL || package->scope == NULL) {
            continue;
        }

        if (package->is_core) {
            continue;
        }
        compiler_emit_package_types(package);
    }
}

void compiler_emit_cfg_dot(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator package_it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    const bool to_console = compiler_wants_console(compiler);
    FileWriter cw = { 0 };
    if (to_console) cw = file_writer_get_stdout();

    while (hashmap_it_next(&package_it, NULL, &package)) {
        if (package == NULL || package->is_core) {
            continue;
        }

        if (to_console) {
            file_writer_write_format(&cw, "package: " SV_FMT "\n", SV_ARG(package->path));
        }

        String pkg_out = STRING_EMPTY;
        const bool to_files = compiler_wants_files(compiler) && compiler_get_package_emit_dir(compiler, package, &pkg_out);

        for (u32 i = 0; i < package->source_files.size; ++i) {
            const SourceFile* source_file = vector_at(package->source_files, i);

            HashmapIterator it = hashmap_get_it(&source_file->cfg_by_fun);
            ASTNode* fun_node = NULL;
            CFGFunction* fun_cfg = NULL;

            while (hashmap_it_next(&it, &fun_node, &fun_cfg)) {
                // Console: show file + function name
                if (to_console) {
                    StringView fun_name = string_get_view(fun_node->as.fun_decl.sign->as.fun_sign.id->as.id.value);
                    file_writer_write_format(&cw, "file: " SV_FMT "  fn: " SV_FMT "\n", SV_ARG(source_file->path), SV_ARG(fun_name));
                    dump_cfg_function_dot(&cw, fun_cfg);
                    file_writer_write_eol(&cw);
                }

                // Files: "<emit>/<pkg-rel>/<stem>.<fn>.cfg.dot"
                if (to_files) {
                    FileWriter fw = (FileWriter){ 0 };
                    StringView stem = path_get_stem(source_file->path);
                    StringView fun_name = string_get_view(fun_node->as.fun_decl.sign->as.fun_sign.id->as.id.value);

                    // build filename: "<stem>.<fun>.cfg.dot"
                    StringBuilder sb = string_builder_create(stem.len + 1 + fun_name.len + 8);
                    string_builder_append_sv(&sb, stem);
                    string_builder_append_c(&sb, '.');
                    string_builder_append_sv(&sb, fun_name);
                    StringView base = string_builder_get_view(sb);

                    String out_fp = compiler_build_package_emit_filepath(string_get_view(pkg_out), base, STR_LIT(".cfg.dot"));

                    if (compiler_open_file_for_write(&fw, string_get_view(out_fp), true, (ReportCollector*)&compiler->rc)) {
                        dump_cfg_function_dot(&fw, fun_cfg);
                        file_writer_close(&fw);
                    }

                    string_destroy(&out_fp);
                    string_builder_destroy(&sb);
                }
            }
        }

        string_destroy(&pkg_out);

        if (to_console) {
            file_writer_write_eol(&cw);
            file_writer_flush(&cw);
        }
    }
}

void compiler_emit_call_graph_txt(const Compiler* compiler) {
    assert(compiler != NULL);

    if (compiler_wants_console(compiler)) {
        FileWriter cw = { 0 };
        cw = file_writer_get_stdout();

        file_writer_write_format(&cw, "global call-graph: \n");
        dump_call_graph_txt(&cw, compiler->call_graph);
        file_writer_write_eol(&cw);
    }

    if (compiler_wants_files(compiler)) {
        String out_filepath = compiler_build_package_emit_filepath(string_get_view(compiler->build_options->emit_dir), STR_LIT("call-graph"), STR_LIT(".txt"));
        FileWriter fw = { 0 };
        if (compiler_open_file_for_write(&fw, string_get_view(out_filepath), true, (ReportCollector*)&compiler->rc)) {
            dump_call_graph_txt(&fw, compiler->call_graph);
            file_writer_close(&fw);
        }
        string_destroy(&out_filepath);
    }
}

void compiler_emit_call_graph_dot(const Compiler* compiler) {
    assert(compiler != NULL);

    if (compiler_wants_console(compiler)) {
        FileWriter cw = { 0 };
        cw = file_writer_get_stdout();

        file_writer_write_format(&cw, "global call-graph: \n");
        dump_call_graph_dot(&cw, compiler->call_graph);
        file_writer_write_eol(&cw);
    }

    if (compiler_wants_files(compiler)) {
        String out_filepath = compiler_build_package_emit_filepath(string_get_view(compiler->build_options->emit_dir), STR_LIT("call-graph"), STR_LIT(".dot"));
        FileWriter fw = { 0 };
        if (compiler_open_file_for_write(&fw, string_get_view(out_filepath), true, (ReportCollector*)&compiler->rc)) {
            dump_call_graph_dot(&fw, compiler->call_graph);
            file_writer_close(&fw);
        }
        string_destroy(&out_filepath);
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

        if (!source_file_resolve_imports(source_file)) {
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

static inline StringView compiler_get_entry_point_symbol_name(const Compiler* compiler) {
    assert(compiler != NULL);

    StringView sv = string_get_view(compiler->build_options->entry_symbol);
    return is_string_view_empty(sv) ? STR_LIT("main") : sv;
}

static inline bool compiler_is_entry_signature_ok(StringView entry_name, const Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);
    assert(symbol->kind == SYMBOL_FUNCTION);

    const Type* T = type_unwrap(symbol->as.typed.type);

    const u32 n     = T->as.fun.param_count;
    const Type* ret = T->as.fun.ret;

    bool params_ok = (n == 0);
    if (!params_ok && n == 1) {
        const Type* p0 = type_unwrap(T->as.fun.params[0]);
        if (p0 == NULL || p0->kind != TYPE_SLICE) {
            params_ok = false;
        }
        else {
            const Type* elem_type = type_unwrap(p0->as.slice.elem);
            params_ok = (elem_type != NULL && elem_type->kind == TYPE_SLICE && is_type_builtin(type_unwrap(elem_type->as.slice.elem), TYPE_BUILTIN_U8));
        }
    }

    bool ret_ok = is_type_builtin(ret, TYPE_BUILTIN_VOID);
    if (!ret_ok) {
        TypeBuiltinKind k = TYPE_BUILTIN_UNKNOWN;
        // allow any sized int (i8/u8/..)
        ret_ok = is_type_sized_integer(ret, &k);
    }

    if (!params_ok || !ret_ok) {
        String got_sig = type_to_str(T);
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, symbol->ast->loc,
            "invalid entry function signature for '"SV_FMT"'.\n"
            "    expected params: () or ([]string)\n"
            "    expected return: void or sized integer\n"
            "    but got:         "SV_FMT"\n",
            SV_ARG(entry_name), SV_ARG(got_sig)
        );
        string_destroy(&got_sig);
        return false;
    }
    return true;
}

static inline void compiler_init_fun_type_index(Compiler* compiler) {
    assert(compiler != NULL);

    compiler->fun_by_type_hash = hashmap_create(32,
        HASHMAP_KEY_SPECS(u32, &item_u32_hash, &item_u32_eq, NULL),
        HASHMAP_VALUE_SPECS(Vector, &vector_destroy)
    );
}

static inline void compiler_clear_fun_type_index(Compiler* compiler) {
    assert(compiler != NULL);
    hashmap_clear(&compiler->fun_by_type_hash);
}

typedef struct FunTypeIdxCtx FunTypeIdxCtx;

struct FunTypeIdxCtx {
    Compiler* compiler;
    Package*  package;
};

static inline void compiler_fun_type_index_pre(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    FunTypeIdxCtx* ctx = (FunTypeIdxCtx*)data;
    assert(ctx != NULL);

    if (node->kind != AST_NODE_FUN_DECL || node->symbol == NULL) {
        return;
    }

    Symbol* f = node->symbol;
    if (f->kind != SYMBOL_FUNCTION) {
        return;
    }

    // ensure function type is resolved.
    if (!symbol_resolve_type(f, &ctx->compiler->ts, &ctx->compiler->rc) || f->as.typed.type == NULL) {
        return;
    }

    const Type* T = f->as.typed.type;
    const u32 h   = type_get_hash(T);

    Vector* bucket = hashmap_get(&ctx->compiler->fun_by_type_hash, &h);
    if (bucket == NULL) {
        Vector new_bucket = vector_create(4, VECTOR_SPECS(Symbol*, NULL));
        hashmap_insert(&ctx->compiler->fun_by_type_hash, &h, &new_bucket);
        bucket = hashmap_get(&ctx->compiler->fun_by_type_hash, &h);
    }

    vector_push_back(bucket, &f);
}

bool compiler_build_fun_type_index(Compiler* compiler) {
    assert(compiler != NULL);

    compiler_clear_fun_type_index(compiler);
    compiler_init_fun_type_index(compiler);

    FunTypeIdxCtx ctx = {
        .compiler = compiler,
        .package = NULL,
    };

    ASTVisitor v = {
        .data    = &ctx,
        .pre_fn  = &compiler_fun_type_index_pre,
        .post_fn = NULL,
    };

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&it, NULL, &package)) {
        if (package == NULL) {
            continue;
        }

        ctx.package = package;

        for (u32 i = 0; i < package->source_files.size; ++i) {
            SourceFile* source_file = vector_at(package->source_files, i);

            const Vector* ents = &source_file->ast->as.source_file.entities;
            for (u32 k = 0; k < ents->size; ++k) {
                ASTNode* n = vector_at(*ents, k);
                ast_visit_with(NULL, n, &v);
            }
        }
    }

    return true;
}

bool compiler_resolve_entry_point(Compiler* compiler) {
    assert(compiler != NULL);

    if (compiler->entry_point != NULL) {
        return true;
    }

    StringView entry_name = compiler_get_entry_point_symbol_name(compiler);

    typedef struct { Package* package; Symbol* symbol; } Hit;
    Hit hits[32]            = { 0 };
    const u32 max_hit_count = ARR_SIZE(hits);
    u32 hit_count           = 0;

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&it, NULL, &package)) {
        if (package == NULL || package->scope == NULL) {
            continue;
        }

        Symbol* symbol = scope_lookup_current(package->scope, entry_name, SYMBOL_NS_FUNC);
        if (symbol == NULL) {
            continue;
        }

        // resolve / validate its type right away to filter bad signatures out
        if (!compiler_is_entry_signature_ok(entry_name, symbol, &compiler->ts, &compiler->rc)) {
            continue;
        }

        if (hit_count < max_hit_count) {
            hits[hit_count++] = (Hit) { .package = package, .symbol = symbol, };
        }
        else {
            REPORT_ERROR(&compiler->rc, DIAG_DRIVER_PROJECT, "too many candidate entry points named '"SV_FMT".'", SV_ARG(entry_name));
            return false;
        }
    }

    if (hit_count == 0) {
        REPORT_ERROR(&compiler->rc, DIAG_DRIVER_PROJECT, "entry point function '" SV_FMT "' was not found in any loaded package.", SV_ARG(entry_name));
        REPORT_NOTE(&compiler->rc, DIAG_DRIVER_PROJECT, "Create a function 'fun " SV_FMT "()' or pass a different name with --entry=<name>.", SV_ARG(entry_name));
        return false;
    }

    if (hit_count > 1) {
        REPORT_ERROR(&compiler->rc, DIAG_DRIVER_PROJECT, "multiple candidate entry points named '" SV_FMT "' found. Please disambiguate.", SV_ARG(entry_name));
        for (u32 i = 0; i < hit_count; ++i) {
            StringView p = hits[i].package->path;
            String sig = type_to_str(hits[i].symbol->as.typed.type);

            REPORT_NOTE(&compiler->rc, DIAG_DRIVER_PROJECT, "candidate in package '" SV_FMT "' with signature " SV_FMT, SV_ARG(p), SV_ARG(sig));
            string_destroy(&sig);
        }
        return false;
    }

    // Unique winner
    hits[0].package->entry_point = hits[0].symbol;
    compiler->entry_point = hits[0].package;
    REPORT_INFO(&compiler->rc, DIAG_DRIVER_PROJECT, "entry point set to '" SV_FMT "' in package '" SV_FMT "'.", SV_ARG(entry_name), SV_ARG(hits[0].package->path));
    return true;
}

bool compiler_build_call_graph(Compiler* compiler) {
    assert(compiler != NULL);

    if (compiler->call_graph != NULL) {
        return true;
    }

    bool status = true;

    CallGraph* graph = call_graph_create(NULL, &compiler->rc);

    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&it, NULL, &package)) {
        if (package == NULL) {
            continue;
        }
        if (package->call_graph == NULL) {
            if (!package_build_call_graph(package)) {
                status = false;
                continue;
            }
        }
        call_graph_merge_into(graph, package->call_graph);
    }

    compiler->call_graph = graph;
    return status;
}

bool compiler_validate_semantics(Compiler* compiler) {
    assert(compiler != NULL);

    bool status = true;
    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&it, NULL, &package)) {
        if (package == NULL) continue;
        if (!package_validate_semantics(package)) {
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