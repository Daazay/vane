#include "vane/compiler/source_file.h"

#include <stdlib.h>

#include "vane/utils/path.h"
#include "vane/compiler/compiler.h"
#include "vane/scanner/token_stream.h"
#include "vane/ast/ast_parser.h"

static inline FileLoadStatus source_file_load_content(StringView path, String* content, ReportCollector* rc) {
    assert(content != NULL);

    FileLoadStatus status = file_content_load(path, (u8**)&content->data, &content->len);

    switch (status) {
    case FILE_LOAD_OK: break;
    case FILE_LOAD_ERR_EMPTY_CONTENT:
        REPORT_NOTE(rc, "driver", "file content is empty: '" SV_FMT"'.", SV_ARG(path));
        break;
    case FILE_LOAD_ERR_INVALID_PATH:
        REPORT_ERROR(rc, "driver", "invalid path: '" SV_FMT"'.", SV_ARG(path));
        break;
    case FILE_LOAD_ERR_NOT_FOUND:
        REPORT_ERROR(rc, "driver", "path not found: '" SV_FMT"'.", SV_ARG(path));
        break;
    case FILE_LOAD_ERR_ACCESS_DENIED:
        REPORT_ERROR(rc, "driver", "access denied for path: '" SV_FMT"'.", SV_ARG(path));
        break;
    case FILE_LOAD_ERR_IS_DIR:
        REPORT_ERROR(rc, "driver", "path is not a directory: '" SV_FMT"'.", SV_ARG(path));
        break;
    case FILE_LOAD_ERR_OPEN:
        REPORT_ERROR(rc, "driver", "failed to open directory: '" SV_FMT"'.", SV_ARG(path));
        break;
    case FILE_LOAD_ERR_READ:
        REPORT_ERROR(rc, "driver", "failed to read directory: '" SV_FMT"'.", SV_ARG(path));
        break;
    default:
        unreachable();
        break;
    }

    return status;
}


SourceFile* source_file_create(StringView path, ReportCollector* rc) {
    String content = STRING_EMPTY;
    FileLoadStatus status = source_file_load_content(path, &content, rc);

    if (status != FILE_LOAD_OK) {
        return NULL;
    }

    SourceFile* source_file = malloc(sizeof(SourceFile));
    assert(source_file != NULL);

    source_file->ast = NULL;

    source_file->path = path;
    source_file->package = NULL;
    source_file->content = content;

    source_file->imports = vector_create(2, VECTOR_SPECS(ImportEntry, NULL));
    //source_file->imports = hashmap_create(2,
    //    HASHMAP_KEY_SPECS(StringView, &string_view_item_hash, &string_view_item_eq, NULL),
    //    HASHMAP_VALUE_SPECS(ImportEntry, NULL)
    //);

    source_file->rc = rc;

    return source_file;
}

void source_file_destroy(SourceFile* source_file) {
    if (source_file == NULL) {
        return;
    }

    string_destroy(&source_file->content);
    vector_destroy(&source_file->imports);
    ast_node_destroy(source_file->ast);

    free(source_file);
}

static inline void split_import_path(StringView path, StringView* collection_name, StringView* package_path) {
    u64 colon_pos = string_view_find_c(path, ':');

    if (colon_pos == (u64)NPOS) {
        if (collection_name != NULL) {
            *collection_name = STRING_VIEW_EMPTY;
        }
        if (package_path != NULL) {
            *package_path = path;
        }
        return;
    }

    if (colon_pos == 0) {
        if (collection_name != NULL) {
            *collection_name = STRING_VIEW_EMPTY;
        }
        if (package_path != NULL) {
            *package_path = string_view_subview(path, 1, path.len - 1);
        }
        return;
    }

    if (collection_name != NULL) {
        *collection_name = string_view_subview(path, 0, colon_pos);
    }
    if (package_path != NULL) {
        *package_path = string_view_subview(path, colon_pos + 1, path.len - colon_pos);
    }
}

bool source_file_parse_ast(SourceFile* source_file) {
    assert(source_file != NULL);

    bool is_good = true;

    TokenStream ts = token_stream_create(0, source_file->path, string_get_view(source_file->content), source_file->rc);
    ASTParser ast_parser = ast_parser_create(&ts);

    //
    source_file->ast = ast_node_create(AST_NODE_SOURCE_FILE, token_stream_peek_next(&ts)->loc);
    source_file->ast->as.source_file.entities = vector_create(8, VECTOR_SPECS(ASTNode*, &ast_node_destroy));
    //

    while (!is_token_stream_end(&ts)) {
        ASTNode* ast = ast_parser_parse_package_entity(&ast_parser);

        switch (ast->kind) {
        case AST_NODE_ERROR: {
            is_good = false;
            token_stream_move_forward(&ts);
        } break;
        case AST_NODE_IMPORT_DECL: {
            const ASTNode* path_node = ast->as.import_decl.path;
            StringView raw = string_get_view(path_node->as.expr_literal.value);

            ImportEntry entry = {
                .name = STRING_VIEW_EMPTY,
                .node = ast,
                .target = NULL,
                .collection_name = STRING_VIEW_EMPTY,
                .package_path = STRING_VIEW_EMPTY,
            };

            split_import_path(raw, &entry.collection_name, &entry.package_path);
            vector_push_back(&source_file->imports, &entry);
        } break;
        default: break;
        }

        vector_push_back(&source_file->ast->as.source_file.entities, &ast);
        source_file->ast->loc.end = ast->loc.end;
    }

    token_stream_destroy(&ts);
    return is_good;
}

static inline StringView derive_import_name(const ImportEntry* e) {
    if (e->node->as.import_decl.alias != NULL) {
        return string_get_view(e->node->as.import_decl.alias->as.id.value);
    }
    if (e->target != NULL) {
        return path_get_stem(e->target->path);
    }
    return path_get_stem(e->package_path);
}

bool source_file_resolve_imports(SourceFile* source_file, struct Compiler* compiler) {
    assert(source_file != NULL && compiler != NULL);

    bool is_ok = true;

    for (u32 i = 0; i < source_file->imports.size; ++i) {
        ImportEntry* entry = vector_at(source_file->imports, i);
        assert(entry->node != NULL);

        // Resolve target
        entry->target = compiler_try_resolve_imported_package(compiler, entry->collection_name, entry->package_path);
        if (entry->target == NULL) {
            const ASTNode* path_node = entry->node->as.import_decl.path;
            REPORT_ERROR_LOC(&compiler->rc, "driver", path_node->loc, "cannot resolve package by path '" SV_FMT "'.", SV_ARG(path_node->as.expr_literal.value));
        }

        // Compute visible name now that target may be known
        entry->name = derive_import_name(entry);

        for (u32 j = 0; j < i; ++j) {
            ImportEntry* prev = vector_at(source_file->imports, j);

            if (string_view_eq_sv(prev->name, entry->name)) {
                REPORT_ERROR_LOC(&compiler->rc, "driver", entry->node->loc,
                    "duplicate import name '" SV_FMT "' in this file.", SV_ARG(entry->name)
                );

                if (prev->node != NULL) {
                    REPORT_NOTE_LOC(&compiler->rc, "driver", prev->node->loc, "the first import with this name is here.");
                }

                is_ok = false;
            }
        }

        // Self-import warning
        if (source_file->package && entry->target == source_file->package) {
            REPORT_WARNING_LOC(&compiler->rc, "driver", entry->node->loc, "self-import of '" SV_FMT "' has no effect.", SV_ARG(entry->package_path));
            REPORT_NOTE_LOC(&compiler->rc, "driver", entry->node->loc, "remove the import or alias it if you intended a rename.");
        }
    }

    return is_ok;
}