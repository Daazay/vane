#include "vane/compiler/compiler.h"

#include <stdio.h>

#include "vane/utils/path.h"
#include "vane/utils/terminal.h"
#include "vane/utils/string_builder.h"

#include "vane/ast/ast_visitor/ast_dot_visitor.h"

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

    compiler.rc = report_collector_create(build_options->log_verbosity);

    return compiler;
}

void compiler_destroy(Compiler* compiler) {
    if (compiler == NULL) {
        return;
    }

    hashmap_destroy(&compiler->packages);
    hashmap_destroy(&compiler->source_files);

    report_collector_destroy(&compiler->rc);
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

Package* compiler_try_resolve_imported_package(Compiler* compiler, StringView collection_name, StringView package_path) {
    assert(compiler != NULL);

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
        import_path = path_join_sv(string_get_view(compiler->build_options->root_path), package_path);
    }

    Package* package = compiler_load_package(compiler, string_get_view(import_path));
    string_destroy(&import_path);

    return package;
}


typedef struct { u32 indent; } DumpAstTextCtx;

static void dump_ast_text_pre(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    DumpAstTextCtx* ctx = (DumpAstTextCtx*)data;
    for (u32 k = 0; k < ctx->indent; ++k) {
        printf("  ");
    }

    printf("%s [%d:%d-%d:%d]\n",
        ast_node_kind_get_name(node->kind),
        node->loc.begin.line, node->loc.begin.column,
        node->loc.end.line, node->loc.end.column
    );
    ctx->indent++;
}

static void dump_ast_text_post(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;
    (void)node;

    DumpAstTextCtx* ctx = (DumpAstTextCtx*)data;
    if (ctx->indent) {
        ctx->indent--;
    }
}

void compiler_dump_ast(const Compiler* compiler) {
    assert(compiler != NULL);

    HashmapIterator package_it = hashmap_get_it(&compiler->packages);
    Package* package = NULL;

    while (hashmap_it_next(&package_it, NULL, &package)) {
        if (package == NULL) continue;

        for (u32 i = 0; i < package->source_files.size; ++i) {
            const SourceFile* sf = vector_at(package->source_files, i);
            printf("[ast: " SV_FMT "]\n", SV_ARG(sf->path));

            DumpAstTextCtx ctx = { 0 };
            ASTVisitor v = {
                .pre_fn = &dump_ast_text_pre,
                .post_fn = &dump_ast_text_post,
                .data = &ctx,
            };
            ast_visit_with(NULL, sf->ast, &v);
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