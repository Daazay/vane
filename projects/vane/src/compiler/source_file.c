#include "vane/compiler/source_file.h"

#include <stdlib.h>

#include "vane/scanner/token_stream.h"
#include "vane/ast/ast_parser.h"

typedef struct ImportEntry ImportEntry;
struct ImportEntry {
    StringView name;
    const struct ASTNode* node;
    struct Package* target;
};


SourceFile* source_file_create(StringView path, ReportCollector* rc) {
    SourceFile* source_file = malloc(sizeof(SourceFile));
    assert(source_file != NULL);

    source_file->ast = NULL;

    source_file->path = path;
    source_file->package = NULL;
    source_file->content = STRING_EMPTY;

    source_file->imports = vector_create(4, VECTOR_SPECS(ImportEntry, NULL));

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

static inline FileLoadStatus source_file_load_content(SourceFile* source_file) {
    assert(source_file != NULL);

    FileLoadStatus status = file_content_load(source_file->path, &source_file->content.data, &source_file->content.len);

    switch (status) {
    case FILE_LOAD_OK: break;
    case FILE_LOAD_ERR_EMPTY_CONTENT:
        source_file->content = STRING_EMPTY;
        status = FILE_LOAD_OK;
        break;
    case FILE_LOAD_ERR_INVALID_PATH:
        REPORT_ERROR(source_file->rc, "driver", "invalid path: '" SV_FMT"'.", SV_ARG(source_file->path));
        break;
    case FILE_LOAD_ERR_NOT_FOUND:
        REPORT_ERROR(source_file->rc, "driver", "path not found: '" SV_FMT"'.", SV_ARG(source_file->path));
        break;
    case FILE_LOAD_ERR_ACCESS_DENIED:
        REPORT_ERROR(source_file->rc, "driver", "access denied for path: '" SV_FMT"'.", SV_ARG(source_file->path));
        break;
    case FILE_LOAD_ERR_IS_DIR:
        REPORT_ERROR(source_file->rc, "driver", "path is not a directory: '" SV_FMT"'.", SV_ARG(source_file->path));
        break;
    case FILE_LOAD_ERR_OPEN:
        REPORT_ERROR(source_file->rc, "driver", "failed to open directory: '" SV_FMT"'.", SV_ARG(source_file->path));
        break;
    case FILE_LOAD_ERR_READ:
        REPORT_ERROR(source_file->rc, "driver", "failed to read directory: '" SV_FMT"'.", SV_ARG(source_file->path));
        break;
    default:
        unreachable();
        break;
    }

    return status;
}

bool source_file_parse_ast(SourceFile* source_file) {
    assert(source_file != NULL);

    if (source_file_load_content(source_file) != FILE_LOAD_OK) {
        return false;
    }

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
            ImportEntry import = {
                .node = ast,
                .target = NULL,
            };
            vector_push_back(&source_file->imports, &import);
        } break;
        default: break;
        }

        vector_push_back(&source_file->ast->as.source_file.entities, &ast);

        source_file->ast->loc.end = ast->loc.end;
    }

    token_stream_destroy(&ts);

    return is_good;
}