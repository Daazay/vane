#include "vane/compiler/source_file.h"

#include <stdlib.h>

#include "vane/utils/path.h"
#include "vane/compiler/compiler.h"
#include "vane/scanner/token_stream.h"
#include "vane/ast/ast_parser.h"

#include "vane/ast/ast_visitor/ast_visitor.h"

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
    source_file->scope = NULL;

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
        const u32 before_idx = ts.idx;

        ASTNode* ast = ast_parser_parse_package_entity(&ast_parser);

        switch (ast->kind) {
        case AST_NODE_ERROR: {
            is_good = false;
            ast_parser_sync_to_package(&ast_parser);
            ast_parser_one_step_guard(&ast_parser, before_idx);
            continue;
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
        entry->target = compiler_try_resolve_imported_package(compiler, source_file, entry->collection_name, entry->package_path);
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

static inline bool resolve_import_symbols(SourceFile* source_file) {
    assert(source_file != NULL);

    bool is_good = true;

    for (u32 i = 0; i < source_file->imports.size; ++i) {
        ImportEntry* import_entry = vector_at(source_file->imports, i);

        Symbol* symbol = scope_lookup_current(source_file->scope, import_entry->name);
        if (symbol == NULL) {
            symbol = symbol_create(SYMBOL_IMPORT, import_entry->name, import_entry->node);
            symbol->as.import.target = import_entry->target;
            scope_add_symbol(source_file->scope, symbol);
            continue;
        }
        // Do something if we import same thing twice or different things but with different identifiers
        is_good = false;
    }

    return is_good;
}

struct ResolveDeclSymbolCtx {
    ReportCollector* rc;
    Scope* scope;
    bool status;
};

static inline void resolve_symbol_decls_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    struct ResolveDeclSymbolCtx* ctx = data;

    if (node->symbol != NULL) {
        return;
    }

    switch (node->kind) {
    case AST_NODE_FUN_DECL: {
        ctx->scope = scope_create(SCOPE_FUNCTION, ctx->scope, node);
        node->scope = ctx->scope;
    } break;
    case AST_NODE_FUN_SIGN: {
        const ASTNode* id = node->as.fun_sign.id;
        StringView name = string_get_view(id->as.id.value);

        Symbol* symbol = scope_lookup(ctx->scope, name);
        if (symbol == NULL) {
            symbol = symbol_create(SYMBOL_FUNCTION, name, parent);
            parent->symbol = symbol;

            //
            assert(ctx->scope->kind == SCOPE_FUNCTION);
            scope_add_symbol(ctx->scope->parent->parent, symbol);

            symbol->scope = ctx->scope->parent;
            break;
        }

        REPORT_ERROR_LOC(ctx->rc, "sema", node->loc, "identifier '"SV_FMT"' is already in use.", SV_ARG(name));
        REPORT_NOTE_LOC(ctx->rc, "sema", symbol->ast->loc, "identifier '"SV_FMT"' was previously defined here.", SV_ARG(name));
        ctx->status = false;
    } break;
    case AST_NODE_FUN_PARAM: {
        const ASTNode* id = node->as.fun_param.id;
        StringView name = string_get_view(id->as.id.value);

        Symbol* symbol = scope_lookup(ctx->scope, name);
        if (symbol == NULL) {
            symbol = symbol_create(SYMBOL_PARAMETER, name, node);
            node->symbol = symbol;

            scope_add_symbol(ctx->scope, symbol);
            break;
        }

        REPORT_ERROR_LOC(ctx->rc, "sema", node->loc, "identifier '"SV_FMT"' is already in use.", SV_ARG(name));
        REPORT_NOTE_LOC(ctx->rc, "sema", symbol->ast->loc, "identifier '"SV_FMT"' was previously defined here.", SV_ARG(name));
        ctx->status = false;
    } break;
    case AST_NODE_STMT_TYPEALIAS_DECL: {
        const ASTNode* id = node->as.stmt_typealias_decl.id;
        StringView name = string_get_view(id->as.id.value);

        Symbol* symbol = scope_lookup(ctx->scope, name);
        if (symbol == NULL) {
            symbol = symbol_create(SYMBOL_TYPEALIAS, name, node);
            node->symbol = symbol;

            // add to package scope
            if (ctx->scope->kind == SCOPE_SOURCE_FILE) {
                scope_add_symbol(ctx->scope->parent, symbol);
            }
            else {
                scope_add_symbol(ctx->scope, symbol);
            }
            break;
        }

        REPORT_ERROR_LOC(ctx->rc, "sema", node->loc, "identifier '"SV_FMT"' is already in use.", SV_ARG(name));
        REPORT_NOTE_LOC(ctx->rc, "sema", symbol->ast->loc, "identifier '"SV_FMT"' was previously defined here.", SV_ARG(name));
        ctx->status = false;
    } break;
    case AST_NODE_STMT_VAR_ITEM: {
        const ASTNode* id = node->as.stmt_var_item.id;
        StringView name = string_get_view(id->as.id.value);

        Symbol* symbol = scope_lookup(ctx->scope, name);
        if (symbol == NULL) {
            symbol = symbol_create(SYMBOL_VARIABLE, name, node);
            node->symbol = symbol;

            scope_add_symbol(ctx->scope, symbol);
            break;
        }

        REPORT_ERROR_LOC(ctx->rc, "sema", node->loc, "identifier '"SV_FMT"' is already in use.", SV_ARG(name));
        REPORT_NOTE_LOC(ctx->rc, "sema", symbol->ast->loc, "identifier '"SV_FMT"' was previously defined here.", SV_ARG(name));
        ctx->status = false;
    } break;
    case AST_NODE_STMT_BLOCK: {
        ctx->scope = scope_create(SCOPE_BASIC, ctx->scope, node);
        node->scope = ctx->scope;
    } break;
    case AST_NODE_STMT_BRANCH: {
        ctx->scope = scope_create(SCOPE_BASIC, ctx->scope, node);
        node->scope = ctx->scope;
    } break;
    case AST_NODE_STMT_WHILE: {
        ctx->scope = scope_create(SCOPE_BASIC, ctx->scope, node);
        node->scope = ctx->scope;
    } break;
    case AST_NODE_STMT_DO: {
        ctx->scope = scope_create(SCOPE_BASIC, ctx->scope, node);
        node->scope = ctx->scope;
    } break;
    default: break;
    }
}

static inline void resolve_symbol_decls_post_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    struct ResolveDeclSymbolCtx* ctx = data;

    switch (node->kind) {
    case AST_NODE_FUN_DECL:
    case AST_NODE_STMT_BLOCK:
    case AST_NODE_STMT_BRANCH:
    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO: {
        ctx->scope = ctx->scope->parent;
    } break;
    default: break;
    }
}

bool source_file_resolve_symbol_decls(SourceFile* source_file) {
    assert(source_file != NULL);

    if (source_file->scope != NULL) {
        return true;
    }

    source_file->scope = scope_create(SCOPE_SOURCE_FILE, source_file->package->scope, source_file->ast);

    bool is_good = true;

    if (!resolve_import_symbols(source_file)) {
        is_good = false;
    }

    struct ResolveDeclSymbolCtx ctx = {
        .rc = source_file->rc,
        .scope = source_file->scope,
        .status = is_good,
    };

    ASTVisitor visitor = {
        .data = &ctx,
        .pre_fn = &resolve_symbol_decls_pre_fn,
        .post_fn = &resolve_symbol_decls_post_fn,
    };

    const Vector* nodes = &source_file->ast->as.source_file.entities;
    for (u32 i = 0; i < nodes->size; ++i) {
        ASTNode* node = vector_at(*nodes, i);

        if (node->kind == AST_NODE_IMPORT_DECL) {
            continue;
        }

        ast_visit_with(NULL, node, &visitor);
    }

    return ctx.status;
}

static inline bool is_member_child(const ASTNode* parent, const ASTNode* node) {
    return parent != NULL &&
        parent->kind == AST_NODE_EXPR_MEMBER &&
        parent->as.expr_member.member == node;
}

static inline bool is_member_object(const ASTNode* parent, const ASTNode* node) {
    return parent != NULL &&
        parent->kind == AST_NODE_EXPR_MEMBER &&
        parent->as.expr_member.object == node;
}

static inline void bind_symbols_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;
    struct ResolveDeclSymbolCtx* ctx = data;

    switch (node->kind) {
    case AST_NODE_EXPR_PLACE: {
        /* Skip only the MEMBER **member** side.
           - Object side (e.g. 'ovj' in 'ovj.r') should bind (or be reported later). */
        if (is_member_child(parent, node)) {
            break;
        }

        StringView name = string_get_view(node->as.expr_place.value);
        Symbol* sym = scope_lookup(ctx->scope, name);   // upward lookup
        if (sym != NULL) {
            node->symbol = sym;
        }
    } break;

    case AST_NODE_FUN_DECL:
    case AST_NODE_STMT_BLOCK:
    case AST_NODE_STMT_BRANCH:
    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO: {
        ctx->scope = node->scope;
    } break;

    default: break;
    }
}

static inline void bind_symbols_post_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;
    struct ResolveDeclSymbolCtx* ctx = data;

    switch (node->kind) {
    case AST_NODE_EXPR_MEMBER: {
        ASTNode* object = node->as.expr_member.object;
        ASTNode* member = node->as.expr_member.member;

        /* Handle import-qualified names: math.fib */
        if (object->symbol != NULL && object->symbol->kind == SYMBOL_IMPORT) {
            Package* target_pkg = object->symbol->as.import.target;
            if (target_pkg == NULL|| target_pkg->scope == NULL) {
                REPORT_ERROR_LOC(ctx->rc, "sema", object->loc, "unresolved import; package not available.");
                ctx->status = false;
                break;
            }

            StringView name = string_get_view(member->as.expr_place.value);
            Symbol* sym = scope_lookup_current(target_pkg->scope, name);
            if (sym == NULL) {
                REPORT_ERROR_LOC(ctx->rc, "sema", member->loc, "symbol '" SV_FMT "' not found in imported package.", SV_ARG(name));
                ctx->status = false;
                break;
            }

            member->symbol = sym;
            node->symbol = sym;
        }
        /* NOTE:
           For non-import members (obj.field), don�t bind here � that�s type-directed.
           The important part is that the OBJECT side has been handled (or error�d). */
    } break;

    case AST_NODE_EXPR_PLACE: {
        /* If still unbound, report � except for the **member** token in a MEMBER expr,
           which is resolved either by the import rule above or later by type system. */
        if (node->symbol == NULL && !is_member_child(parent, node)) {
            StringView name = string_get_view(node->as.expr_place.value);
            REPORT_ERROR_LOC(ctx->rc, "sema", node->loc, "unresolved identifier '" SV_FMT "'.", SV_ARG(name));
            ctx->status = false;
        }
    } break;

    case AST_NODE_FUN_DECL:
    case AST_NODE_STMT_BLOCK:
    case AST_NODE_STMT_BRANCH:
    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO:
        ctx->scope = ctx->scope->parent;
        break;
    default: break;
    }
}

bool source_file_bind_symbols(SourceFile* source_file) {
    assert(source_file != NULL);
    assert(source_file->scope != NULL);

    struct ResolveDeclSymbolCtx ctx = {
        .rc = source_file->rc,
        .scope = source_file->scope,
        .status = true,
    };

    ASTVisitor visitor = {
        .data = &ctx,
        .pre_fn = &bind_symbols_pre_fn,
        .post_fn = &bind_symbols_post_fn,
    };

    const Vector* nodes = &source_file->ast->as.source_file.entities;
    for (u32 i = 0; i < nodes->size; ++i) {
        ASTNode* node = vector_at(*nodes, i);
        ast_visit_with(NULL, node, &visitor);
    }

    return ctx.status;
}