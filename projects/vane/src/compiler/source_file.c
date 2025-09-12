#include "vane/compiler/source_file.h"

#include <stdlib.h>

#include "vane/utils/path.h"
#include "vane/scanner/token_stream.h"
#include "vane/ast/ast_parser.h"
#include "vane/ast/ast_visitor/ast_visitor.h"
#include "vane/compiler/compiler.h"
#include "vane/sema/scope.h"

#include "vane/diagnostic/diagnostic_tags.h"

static inline FileLoadStatus source_file_load_content(StringView path, String* content, ReportCollector* rc) {
    assert(content != NULL);

    FileLoadStatus status = file_content_load(path, (u8**)&content->data, &content->len);
    switch (status) {
    case FILE_LOAD_OK: break;
    case FILE_LOAD_ERR_EMPTY_CONTENT: REPORT_NOTE(rc,  DIAG_DRIVER_FS, "file content is empty: '" SV_FMT"'.", SV_ARG(path)); break;
    case FILE_LOAD_ERR_INVALID_PATH:  REPORT_ERROR(rc, DIAG_DRIVER_FS, "invalid path: '" SV_FMT"'.", SV_ARG(path)); break;
    case FILE_LOAD_ERR_NOT_FOUND:     REPORT_ERROR(rc, DIAG_DRIVER_FS, "path not found: '" SV_FMT"'.", SV_ARG(path)); break;
    case FILE_LOAD_ERR_ACCESS_DENIED: REPORT_ERROR(rc, DIAG_DRIVER_FS, "access denied for path: '" SV_FMT"'.", SV_ARG(path)); break;
    case FILE_LOAD_ERR_IS_DIR:        REPORT_ERROR(rc, DIAG_DRIVER_FS, "path is not a directory: '" SV_FMT"'.", SV_ARG(path)); break;
    case FILE_LOAD_ERR_OPEN:          REPORT_ERROR(rc, DIAG_DRIVER_FS, "failed to open directory: '" SV_FMT"'.", SV_ARG(path)); break;
    case FILE_LOAD_ERR_READ:          REPORT_ERROR(rc, DIAG_DRIVER_FS, "failed to read directory: '" SV_FMT"'.", SV_ARG(path)); break;
    default: unreachable(); break;
    }

    return status;
}

SourceFile* source_file_create(StringView path, ReportCollector* rc) {
    String content = STRING_EMPTY;
    if (source_file_load_content(path, &content, rc) != FILE_LOAD_OK) {
        return NULL;
    }

    SourceFile* source_file = malloc(sizeof(SourceFile));
    assert(source_file != NULL);

    source_file->path    = path;
    source_file->content = content;
    source_file->package = NULL;
    source_file->scope   = NULL;
    source_file->ast     = NULL;

    source_file->imports = vector_create(
        SOURCE_FILE_DEFAULT_IMPORT_COUNT,
        VECTOR_SPECS(ImportEntry, NULL)
    );
    source_file->rc      = rc;
    source_file->imports_resolved = false;

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

static inline void split_import_path(StringView raw, ImportBaseKind* base, StringView* collection_name, StringView* package_path) {
    assert(base != NULL && collection_name != NULL && package_path != NULL);

    *collection_name = STRING_VIEW_EMPTY;
    *package_path    = STRING_VIEW_EMPTY;
    *base            = IMPORT_BASE_RELATIVE;

    if (is_string_view_empty(raw)) {
        return;
    }

    u64 colon_pos = string_view_find_c(raw, ':');
    if (colon_pos != (u64)NPOS) {
        // collectio prefix "collection_name:package"
        if (colon_pos > 0) {
            *base = IMPORT_BASE_COLLECTION;
            *collection_name = string_view_subview(raw, 0, colon_pos);
            *package_path = string_view_subview(raw, colon_pos + 1, raw.len - 1);
            return;
        }
        // leading colon -> project-root import
        *base = IMPORT_BASE_PROJECT_ROOT;
        *package_path = string_view_subview(raw, 1, raw.len - 1);
        return;
    }

    // Otherwise: relative to current package
    *base = IMPORT_BASE_RELATIVE;
    *package_path = raw;
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

bool source_file_parse_ast(SourceFile* source_file) {
    assert(source_file != NULL);

    bool is_good = true;

    TokenStream ts       = token_stream_create(0, source_file->path, string_get_view(source_file->content), source_file->rc);
    ASTParser ast_parser = ast_parser_create(&ts);

    source_file->ast = ast_node_create(AST_NODE_SOURCE_FILE, token_stream_peek_next(&ts)->loc);
    source_file->ast->as.source_file.entities = vector_create(
        SOURCE_FILE_DEFAULT_ENTITIES_COUNT,
        VECTOR_SPECS(ASTNode*, &ast_node_destroy)
    );

    while (!is_token_stream_end(&ts)) {
        const u32 before_idx = ts.idx;
        ASTNode* node = ast_parser_parse_package_entity(&ast_parser);

        vector_push_back(&source_file->ast->as.source_file.entities, &node);
        source_file->ast->loc.end = node->loc.end;

        if (node->kind == AST_NODE_ERROR) {
            is_good = false;
            ast_parser_sync_to_package(&ast_parser);
            ast_parser_one_step_guard(&ast_parser, before_idx);
            continue;
        }

        if (node->kind == AST_NODE_IMPORT_DECL) {
            const ASTNode* path_node = node->as.import_decl.path;
            StringView raw = string_get_view(path_node->as.expr_literal.value);

            ImportEntry entry = {
                .name = STRING_VIEW_EMPTY,
                .node = node,
                .target = NULL,
                .base            = IMPORT_BASE_RELATIVE,
                .collection_name = STRING_VIEW_EMPTY,
                .package_path    = STRING_VIEW_EMPTY,
            };

            split_import_path(raw, &entry.base, &entry.collection_name, &entry.package_path);
            vector_push_back(&source_file->imports, &entry);
        }
    }

    token_stream_destroy(&ts);
    return is_good;
}

bool source_file_resolve_imports(SourceFile* source_file, struct Compiler* compiler) {
    assert(source_file != NULL && compiler != NULL);

    bool is_ok = true;

    for (u32 i = 0; i < source_file->imports.size; ++i) {
        ImportEntry* entry = vector_at(source_file->imports, i);

        // Resolve target package
        entry->target = compiler_resolve_import(compiler, source_file, entry);
        if (entry->target == NULL) {
            const ASTNode* path_node = entry->node->as.import_decl.path;
            REPORT_ERROR_LOC(&compiler->rc, DIAG_DRIVER_IMPORTS, path_node->loc, "cannot resolve package by path '" SV_FMT "'.", SV_ARG(path_node->as.expr_literal.value));
            is_ok = false;
        }

        // Compute the visible name
        entry->name = derive_import_name(entry);

        // Duplicate visible name inside this file?
        for (u32 j = 0; j < i; ++j) {
            const ImportEntry* prev = vector_at(source_file->imports, j);

            StringView key_i = entry->node->as.import_decl.alias
                ? string_get_view(entry->node->as.import_decl.alias->as.id.value)
                : STRING_VIEW_EMPTY;

            StringView key_j = prev->node->as.import_decl.alias
                ? string_get_view(prev->node->as.import_decl.alias->as.id.value)
                : STRING_VIEW_EMPTY;

            if (!is_string_view_empty(key_i) && string_view_eq_sv(key_i, key_j)) {
                REPORT_ERROR_LOC(&compiler->rc, DIAG_SEMA_IMPORTS, entry->node->loc, "duplicate import alias '" SV_FMT "' in this file.", SV_ARG(key_i));
                REPORT_NOTE_LOC(&compiler->rc, DIAG_SEMA_IMPORTS, prev->node->loc, "the first import with this alias is here.");
                is_ok = false;
            }
        }

        // Self-import warning (has no effect)
        if (entry->target == source_file->package) {
            REPORT_WARNING_LOC(&compiler->rc, DIAG_SEMA_IMPORTS, entry->node->loc, "self-import of '" SV_FMT "' has no effect.", SV_ARG(entry->package_path));
            REPORT_NOTE_LOC(&compiler->rc, DIAG_SEMA_IMPORTS, entry->node->loc, "remove the import or alias it if you intended a rename.");
        }
    }

    return is_ok;
}

static inline void warn_shadow(ReportCollector* rc, const ASTNode* node, StringView name, const Symbol* outer) {
    assert(rc != NULL && node != NULL && outer != NULL);

    REPORT_WARNING_LOC(rc, DIAG_SEMA_SYMBOLS, node->loc, "declaration '"SV_FMT"' shadows a %s from an outer scope.", SV_ARG(name), symbol_kind_get_name(outer->kind));
    if (outer->ast != NULL) {
        REPORT_NOTE_LOC(rc, DIAG_SEMA_SYMBOLS, outer->ast->loc, "the shadowed declaration is here");
    }
}

static inline Symbol* lookup_same_ns_in_outer_scope(Scope* start, StringView name, SymbolNamespaceMask mask) {
    for (Scope* s = start->parent; s != NULL; s = s->parent) {
        Symbol* hit = scope_lookup_current(s, name, mask);
        if (hit != NULL) {
            return hit;
        }
    }

    return NULL;
}

static inline bool resolve_import_symbols(SourceFile* source_file) {
    assert(source_file != NULL);

    bool is_good = true;

    for (u32 i = 0; i < source_file->imports.size; ++i) {
        ImportEntry* e = vector_at(source_file->imports, i);

        if (e->target == NULL) {
            // Already reported earlier during resolve_imports; repeat here for locality.
            REPORT_ERROR_LOC(source_file->rc, "module", e->node->loc, "cannot resolve imported package.");
            is_good = false;
            continue;
        }

        const bool has_alias = (e->node->as.import_decl.alias != NULL);

        // Unqualified by default -> open import into this file scope
        if (!has_alias) {
            // Open inport into this file scope on bind stage
            //scope_add_open_import(source_file->scope, e->target->scope);
            continue;
        }

        // Qualified import with alias -> create SYMBOL_IMPORT in this file's scope
        StringView alias = string_get_view(e->node->as.import_decl.alias->as.id.value);

        // Fail on any name that already exists in THIS scope (any namespace)
        if (scope_lookup_current_any(source_file->scope, alias)) {
            REPORT_ERROR_LOC(source_file->rc, DIAG_SEMA_SYMBOLS, e->node->loc, "identifier '" SV_FMT "' is already used in this scope; cannot use as import alias.", SV_ARG(alias));
            is_good = false;
            continue;
        }

        // Warn when alias shadows any other declarations (any namespace)
        if (lookup_same_ns_in_outer_scope(source_file->scope, alias, SYMBOL_NS_VALUE | SYMBOL_NS_FUNC | SYMBOL_NS_TYPE | SYMBOL_NS_MODULE)) {
            REPORT_WARNING_LOC(source_file->rc, DIAG_SEMA_SYMBOLS, e->node->loc, "import alias '"SV_FMT"' shadows an outer declaration; consider renaming with 'as'.", SV_ARG(alias));
        }

        Symbol* mod = symbol_create(SYMBOL_IMPORT, alias, e->node);
        mod->as.import.target = e->target;
        scope_add_symbol(source_file->scope, mod);
    }

    return is_good;
}

struct ResolveDeclCtx {
    ReportCollector* rc;
    Scope* scope;
    bool status;
};

static inline void resolve_symbol_decls_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    struct ResolveDeclCtx* ctx = data;

    if (node->symbol != NULL) {
        return;
    }

    switch (node->kind) {
    case AST_NODE_FUN_DECL: {
        ctx->scope  = scope_create(SCOPE_FUNCTION, ctx->scope, node);
        node->scope = ctx->scope;
    } break;

    case AST_NODE_FUN_SIGN: {
        const ASTNode* id = node->as.fun_sign.id;
        StringView name   = string_get_view(id->as.id.value);
        const SymbolNamespaceMask mask = SYMBOL_NS_FUNC;

        Scope* fun_scope = ctx->scope;
        assert(fun_scope != NULL && fun_scope->kind == SCOPE_FUNCTION);
        Scope* source_file_scope = fun_scope->parent;
        assert(source_file_scope != NULL && source_file_scope->kind == SCOPE_SOURCE_FILE);
        Scope* package_scope = source_file_scope->parent;
        assert(package_scope != NULL && package_scope->kind == SCOPE_PACKAGE);

        // duplicate in pacakge?
        Symbol* prev = scope_lookup_current(package_scope, name, mask);
        if (prev != NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, node->loc, "function '"SV_FMT"' is already declared in this package.", SV_ARG(name));
            REPORT_NOTE_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, prev->ast->loc, "first declaration is here.");

            ctx->status = false;
            break;
        }

        // Shadow warning
        Symbol* outer = lookup_same_ns_in_outer_scope(package_scope, name, mask);
        if (outer != NULL) {
            warn_shadow(ctx->rc, node, name, outer);
        }

        Symbol* f = symbol_create(SYMBOL_FUNCTION, name, parent);
        parent->symbol = f;
        scope_add_symbol(package_scope, f);

        f->scope = ctx->scope;
    } break;

    case AST_NODE_FUN_PARAM: {
        const ASTNode* id = node->as.fun_param.id;
        StringView name   = string_get_view(id->as.id.value);
        const SymbolNamespaceMask mask = SYMBOL_NS_VALUE;

        // Duplicate in current function scope
        Symbol* prev = scope_lookup_current(ctx->scope, name, mask);
        if (prev != NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, node->loc, "parameter '"SV_FMT"' already exists in this scope.", SV_ARG(name));
            REPORT_NOTE_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, prev->ast->loc, "first declarated here");
            ctx->status = false;
            break;
        }

        // Shadow outer value
        Symbol* outer = lookup_same_ns_in_outer_scope(ctx->scope, name, mask);
        if (outer != NULL) {
            warn_shadow(ctx->rc, node, name, outer);
        }

        Symbol* p = symbol_create(SYMBOL_PARAMETER, name, node);
        node->symbol = p;
        scope_add_symbol(ctx->scope, p);
    } break;

    case AST_NODE_STMT_TYPEALIAS_DECL: {
        const ASTNode* id = node->as.stmt_typealias_decl.id;
        StringView name   = string_get_view(id->as.id.value);
        const SymbolNamespaceMask mask = SYMBOL_NS_TYPE;

        // top-level -> put in package scope; otherwise local scope
        Scope* target = ctx->scope;
        if (ctx->scope->kind == SCOPE_SOURCE_FILE) {
            Scope* package_scope = target->parent;
            assert(package_scope != NULL && package_scope->kind == SCOPE_PACKAGE);

            if (package_scope != NULL) {
                target = package_scope;
            }
        }

        Symbol* prev = scope_lookup_current(target, name, mask);
        if (prev != NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, node->loc, "type '"SV_FMT"' already exists in this scope.", SV_ARG(name));
            REPORT_NOTE_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, prev->ast->loc, "first declared here.");
            ctx->status = false;
            break;
        }

        Symbol* outer = lookup_same_ns_in_outer_scope(target, name, mask);
        if (outer != NULL) {
            warn_shadow(ctx->rc, node, name, outer);
        }

        Symbol* t = symbol_create(SYMBOL_TYPEALIAS, name, node);
        node->symbol = t;
        scope_add_symbol(target, t);
    } break;

    case AST_NODE_STMT_VAR_ITEM: {
        const ASTNode* id = node->as.stmt_var_item.id;
        StringView name   = string_get_view(id->as.id.value);
        const SymbolNamespaceMask mask = SYMBOL_NS_VALUE;

        Symbol* prev = scope_lookup_current(ctx->scope, name, mask);
        if (prev != NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, node->loc, "variable '"SV_FMT"' already exists in this scope.", SV_ARG(name));
            REPORT_NOTE_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, prev->ast->loc, "first declared here.");
            ctx->status = false;
            break;
        }

        Symbol* outer = lookup_same_ns_in_outer_scope(ctx->scope, name, mask);
        if (outer != NULL) {
            warn_shadow(ctx->rc, node, name, outer);
        }

        Symbol* v = symbol_create(SYMBOL_VARIABLE, name, node);
        node->symbol = v;
        scope_add_symbol(ctx->scope, v);
    } break;

    case AST_NODE_STMT_BLOCK:
    case AST_NODE_STMT_BRANCH:
    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO: {
        ctx->scope = scope_create(SCOPE_BASIC, ctx->scope, node);
        node->scope = ctx->scope;
    } break;

    default: break;
    }
}

static inline void resolve_symbol_decls_post_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;
    struct ResolveDeclCtx* ctx = data;

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

    // Create source-file scope chained to the package
    source_file->scope = scope_create(SCOPE_SOURCE_FILE, source_file->package->scope, source_file->ast);

    bool is_good = true;
    // Bring imports into this scope (alias imports or open-imports)
    if (!resolve_import_symbols(source_file)) {
        is_good = false;
    }

    struct ResolveDeclCtx ctx = {
        .rc     = source_file->rc,
        .scope  = source_file->scope,
        .status = is_good,
    };

    ASTVisitor v = {
        .data    = &ctx,
        .pre_fn  = &resolve_symbol_decls_pre_fn,
        .post_fn = &resolve_symbol_decls_post_fn,
    };

    const Vector* nodes = &source_file->ast->as.source_file.entities;
    for (u32 i = 0; i < nodes->size; ++i) {
        ASTNode* node = vector_at(*nodes, i);

        if (node->kind == AST_NODE_IMPORT_DECL) {
            continue;
        }

        ast_visit_with(NULL, node, &v);
    }

    return ctx.status;
}

static inline bool bind_unqualified_outer_scopes(SourceFile* source_file) {
    assert(source_file != NULL);

    bool is_good = true;

    for (u32 i = 0; i < source_file->imports.size; ++i) {
        ImportEntry* e = vector_at(source_file->imports, i);

        const bool has_alias = (e->node->as.import_decl.alias != NULL);

        if (has_alias) {
            continue;
        }

        if (e->target == NULL) {
            // already reported during module resolution
            continue;
        }

        if (e->target->scope == NULL) {
            REPORT_ERROR_LOC(source_file->rc, "module", e->node->loc, "imported package has no scope.");
            is_good = false;
            continue;
        }

        // avoid duplicates
        bool already_added = false;
        for (u32 k = 0; k < source_file->scope->open_imports.size; ++k) {
            Scope* prev = vector_at(source_file->scope->open_imports, k);
            if (prev == e->target->scope) {
                already_added = true;

                REPORT_NOTE_LOC(source_file->rc, DIAG_SEMA_IMPORTS, e->node->loc, "package '" SV_FMT "' is already opened in this file; duplicate import ignored.", SV_ARG(e->target->path));
                break;
            }
        }
        if (!already_added) {
            scope_add_open_import(source_file->scope, e->target->scope);
            REPORT_DEBUG(source_file->rc, DIAG_SEMA_IMPORTS, "opened import '" SV_FMT "' into file '" SV_FMT "'.", SV_ARG(e->target->path), SV_ARG(source_file->path));
        }
    }

    return is_good;
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

static inline bool is_callee_position(const ASTNode* parent, const ASTNode* node) {
    return parent != NULL &&
           parent->kind == AST_NODE_EXPR_CALL &&
           parent->as.expr_call.callee == node;
}

static inline SymbolNamespaceMask mask_for_place_use(const ASTNode* parent, const ASTNode* node) {
    // foo(...) -> must be a function
    if (is_callee_position(parent, node)) {
        return SYMBOL_NS_FUNC | SYMBOL_NS_VALUE;
    }
    // math.fib -> allow module alias (import) here; also allow values (obj.field style)
    if (is_member_object(parent, node)) {
        return SYMBOL_NS_MODULE | SYMBOL_NS_VALUE;
    }
    // generic expression position: a value, but let functions bind too if language allows taking function names
    return SYMBOL_NS_VALUE | SYMBOL_NS_FUNC;
}

static inline SymbolNamespaceMask mask_for_member_from_import(const ASTNode* parent_of_member_expr, const ASTNode* member_expr) {
    // math.fib(...) -> fib must be a function
    if (is_callee_position(parent_of_member_expr, member_expr)) {
        return SYMBOL_NS_FUNC;
    }
    // In expression (non-callee) contexts, prefer value/func; types/modules excluded for expr member
    return SYMBOL_NS_VALUE | SYMBOL_NS_FUNC;
}

struct BindCtx {
    ReportCollector* rc;
    Scope* scope;
    bool status;
};

static inline void bind_symbols_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;
    struct BindCtx* ctx = data;

    switch (node->kind) {
    case AST_NODE_EXPR_PLACE: {
        // Skip the "member" token of a member expr; object is processed.
        if (is_member_child(parent, node)) {
            break;
        }

        StringView name = string_get_view(node->as.expr_place.value);
        const SymbolNamespaceMask mask = mask_for_place_use(parent, node);

        Symbol* sym = scope_lookup(ctx->scope, name, mask);
        if (sym != NULL) {
            node->symbol = sym;
        }
    } break;

    case AST_NODE_TYPEREF_CUSTOM: {
        StringView name = string_get_view(node->as.typeref_custom.value);
        Symbol* sym = scope_lookup(ctx->scope, name, SYMBOL_NS_TYPE);
        if (sym == NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, node->loc, "unresolved type '" SV_FMT "'.", SV_ARG(name));
            ctx->status = false;
            break;
        }
        node->symbol = sym;
    } break;

    case AST_NODE_TYPEREF_QUALIFIED: {
        Vector* segs = &node->as.typeref_qualified.segments;

        if (segs->size > 2) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, node->loc, "nested qualified type is not supported yet; yse 'alias.Type'.");
            ctx->scope = false;
            break;
        }

        ASTNode* head = vector_at(*segs, 0);
        ASTNode* tail = vector_at(*segs, 1);

        StringView alias = string_get_view(head->as.id.value);
        StringView tname = string_get_view(tail->as.id.value);

        // head must be an import alias
        Symbol* mod = scope_lookup(ctx->scope, alias, SYMBOL_NS_MODULE);
        if (mod == NULL || mod->kind != SYMBOL_IMPORT ||
            mod->as.import.target == NULL ||
            mod->as.import.target->scope == NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_IMPORTS, head->loc, "expected import alias before '.', got '" SV_FMT "'.", SV_ARG(alias));
            ctx->status = false;
            break;
        }

        // type must exist in target package’s scope
        Symbol* ty = scope_lookup_current(mod->as.import.target->scope, tname, SYMBOL_NS_TYPE);
        if (ty == NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, tail->loc, "type '" SV_FMT "' not found in imported package.", SV_ARG(tname));
            ctx->status = false;
            break;
        }

        node->symbol = ty;
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
    struct BindCtx* ctx = data;

    switch (node->kind) {
    case AST_NODE_EXPR_MEMBER: {
        ASTNode* object = node->as.expr_member.object;
        ASTNode* member = node->as.expr_member.member;

        // Qualified via import alias:  math.fib
        if (object->symbol != NULL && object->symbol->kind == SYMBOL_IMPORT) {
            Package* pkg = object->symbol->as.import.target;
            if (pkg == NULL|| pkg->scope == NULL) {
                REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, object->loc, "unresolved import; package not available.");
                ctx->status = false;
                break;
            }

            StringView mname = string_get_view(member->as.expr_place.value);
            const SymbolNamespaceMask mask = mask_for_member_from_import(parent, node);

            Symbol* hit = scope_lookup_current(pkg->scope, mname, mask);
            if (hit == NULL) {
                if (mask == SYMBOL_NS_FUNC) {
                    REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, member->loc, "function '" SV_FMT "' not found in imported package.", SV_ARG(mname));
                }
                else {
                    REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, member->loc, "symbol '" SV_FMT "' not found in imported package.", SV_ARG(mname));
                }
            }

            member->symbol = hit;
            node->symbol = hit;
        }
        // Non-import member (obj.field) → leave for type-directed resolution later.
    } break;

    case AST_NODE_EXPR_PLACE: {
        if (node->symbol == NULL && !is_member_child(parent, node)) {
            const StringView name = string_get_view(node->as.expr_place.value);
            if (is_callee_position(parent, node)) {
                REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, node->loc, "unresolved function '" SV_FMT "'.", SV_ARG(name));
            }
            else {
                REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, node->loc, "unresolved identifier '" SV_FMT "'.", SV_ARG(name));
            }
            ctx->status = false;
        }
    } break;

    case AST_NODE_FUN_DECL:
    case AST_NODE_STMT_BLOCK:
    case AST_NODE_STMT_BRANCH:
    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO:
        ctx->scope = ctx->scope->parent; // leave scope
        break;

    default: break;
    }
}

bool source_file_bind_symbols(SourceFile* source_file) {
    assert(source_file != NULL);
    assert(source_file->scope != NULL);

    bool is_good = true;
    // Bring unqualified open imports
    if (!bind_unqualified_outer_scopes(source_file)) {
        is_good = false;
    }

    struct BindCtx ctx = {
        .rc     = source_file->rc,
        .scope  = source_file->scope,
        .status = is_good,
    };

    ASTVisitor v = {
        .data    = &ctx,
        .pre_fn  = &bind_symbols_pre_fn,
        .post_fn = &bind_symbols_post_fn,
    };

    const Vector* nodes = &source_file->ast->as.source_file.entities;
    for (u32 i = 0; i < nodes->size; ++i) {
        ASTNode* node = vector_at(*nodes, i);
        ast_visit_with(NULL, node, &v);
    }

    return ctx.status;
}