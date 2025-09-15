#include "vane/compiler/source_file.h"

#include <stdlib.h>

#include "vane/utils/path.h"
#include "vane/scanner/token_stream.h"
#include "vane/ast/ast_parser.h"
#include "vane/ast/ast_visitor.h"
#include "vane/compiler/compiler.h"
#include "vane/sema/scope.h"
#include "vane/sema/typecheck.h"
#include "vane/cfg/cfg_function.h"

static inline FileLoadStatus source_file_load_content(StringView path, String* content, ReportCollector* rc) {
    assert(content != NULL);

    FileLoadStatus status = file_content_load(path, (u8**)&content->data, &content->len);
    switch (status) {
    case FILE_LOAD_OK: break;
    case FILE_LOAD_ERR_EMPTY_CONTENT: REPORT_NOTE(rc, DIAG_DRIVER_FS, "file content is empty: '" SV_FMT"'.", SV_ARG(path)); break;
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

static inline void source_file_split_import_path(StringView raw, ImportBaseKind* base, StringView* collection_name, StringView* package_path) {
    assert(base != NULL && collection_name != NULL && package_path != NULL);

    *collection_name = STRING_VIEW_EMPTY;
    *package_path = STRING_VIEW_EMPTY;
    *base = IMPORT_BASE_RELATIVE;

    if (is_string_view_empty(raw)) {
        return;
    }

    const u64 colon = string_view_find_c(raw, ':');
    if (colon == (u64)NPOS) {
        *base = IMPORT_BASE_RELATIVE;
        *package_path = raw;
        return;
    }

    if (colon == 0) {
        *base = IMPORT_BASE_PROJECT_ROOT;
        *package_path = string_view_subview(raw, 1, raw.len - 1);
        return;
    }

    // "collection:package/subpackage"
    *base = IMPORT_BASE_COLLECTION;
    *collection_name = string_view_subview(raw, 0, colon);
    *package_path = string_view_subview(raw, colon + 1, raw.len - (colon + 1));
}

static inline StringView source_file_derive_import_name(const ImportEntry* e) {
    assert(e != NULL);

    if (e->node->as.import_decl.alias != NULL) {
        return string_get_view(e->node->as.import_decl.alias->as.id.value);
    }
    if (e->target != NULL) {
        return path_get_stem(e->target->path);
    }
    return path_get_stem(e->package_path);
}

static inline bool source_file_resolve_import_symbols(SourceFile* source_file) {
    assert(source_file != NULL);

    bool is_good = true;

    for (u32 i = 0; i < source_file->imports.size; ++i) {
        ImportEntry* e = vector_at(source_file->imports, i);

        if (e->target == NULL) {
            REPORT_ERROR_LOC(source_file->rc, DIAG_SEMA_IMPORTS, e->node->loc, "unresolved import '"SV_FMT"'.", SV_ARG(e->package_path));
            is_good = false;
            continue;
        }

        const bool has_alias = (e->node->as.import_decl.alias != NULL);

        // unqualified by default
        if (!has_alias) {
            continue;
        }

        StringView alias = string_get_view(e->node->as.import_decl.alias->as.id.value);

        if (scope_lookup_current(source_file->scope, alias, SYMBOL_NS_MODULE) != NULL) {
            REPORT_ERROR_LOC(source_file->rc, DIAG_SEMA_IMPORTS, e->node->loc, "import name '"SV_FMT"' already exists in this scope.", SV_ARG(alias));
            is_good = false;
            continue;
        }

        // Warn when alias shadows any other declarations (any namespace)
        const Symbol* outer_symbol = scope_lookup(source_file->scope->parent, alias, SYMBOL_NS_VALUE | SYMBOL_NS_FUNC | SYMBOL_NS_TYPE | SYMBOL_NS_MODULE);
        if (outer_symbol != NULL) {
            REPORT_WARNING_LOC(source_file->rc, DIAG_SEMA_SYMBOLS, e->node->loc, "import alias '"SV_FMT"' shadows an outer declaration.", SV_ARG(alias));
            REPORT_NOTE_LOC(source_file->rc, DIAG_SEMA_SYMBOLS, outer_symbol->ast->loc, "previous declared here");
        }

        Symbol* symbol = symbol_create(SYMBOL_IMPORT, alias, e->node);
        symbol->as.import.target = e->target;
        scope_add_symbol(source_file->scope, symbol);

        e->node->symbol = symbol;
        REPORT_NOTE_LOC(source_file->rc, DIAG_SEMA_IMPORTS, e->node->loc, "import aliased as '"SV_FMT"'.", SV_ARG(alias))
    }

    return is_good;
}

static inline bool source_file_bind_unqualified_outer_scopes(SourceFile* source_file) {
    assert(source_file != NULL && source_file->scope != NULL);
    assert(source_file->package != NULL && source_file->package->scope != NULL);

    bool is_good = true;

    for (u32 i = 0; i < source_file->imports.size; ++i) {
        ImportEntry* e = vector_at(source_file->imports, i);

        const bool has_alias = (e->node->as.import_decl.alias != NULL);
        if (has_alias) {
            continue;
        }

        if (e->target == NULL || e->target->scope == NULL) {
            continue;
        }

        if (e->target->scope == NULL) {
            REPORT_ERROR_LOC(source_file->rc, "module", e->node->loc, "imported package has no scope.");
            is_good = false;
            continue;
        }

        // avoid duplications
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

static inline void source_file_warn_shadow(ReportCollector* rc, const ASTNode* node, StringView name, const Symbol* outer) {
    assert(rc != NULL && node != NULL && outer != NULL);
    REPORT_NOTE_LOC(rc, DIAG_SEMA_SYMBOLS, node->loc, "shadows '"SV_FMT"' declared at previous scope.", SV_ARG(name));
    REPORT_NOTE_LOC(rc, DIAG_SEMA_SYMBOLS, outer->ast->loc, "outer declaration is here");
}

struct ResolveDeclCtx {
    ReportCollector* rc;
    Scope* scope;
    bool             status;
};

static inline void source_file_resolve_symbol_decls_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    struct ResolveDeclCtx* ctx = data;
    assert(ctx != NULL && ctx->scope != NULL);

    switch (node->kind) {

    case AST_NODE_FUN_DECL: {
        const ASTNode* sign = node->as.fun_decl.sign;
        StringView name = string_get_view(sign->as.fun_sign.id->as.id.value);

        Symbol* existing = scope_lookup_current(ctx->scope, name, SYMBOL_NS_FUNC);
        if (existing != NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, node->loc, "function '"SV_FMT"' already exists in this scope.", SV_ARG(name));
            REPORT_NOTE_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, existing->ast->loc, "first declaration is here.");
            ctx->status = false;
            break;
        }

        Symbol* outer = scope_lookup(ctx->scope, name, SYMBOL_NS_FUNC);
        if (outer != NULL) {
            source_file_warn_shadow(ctx->rc, node, name, outer);
        }

        Symbol* f = symbol_create(SYMBOL_FUNCTION, name, node);
        node->symbol = f;

        // if function declared at source file scope, we push it to parent (package) scope
        if (ctx->scope->kind == SCOPE_SOURCE_FILE) {
            scope_add_symbol(ctx->scope->parent, f);
        }
        else {
            scope_add_symbol(ctx->scope, f);
        }

        ctx->scope = scope_create(SCOPE_FUNCTION, ctx->scope, node, ctx->scope->package);
        node->scope = ctx->scope;
    } break;

    case AST_NODE_FUN_PARAM: {
        StringView name = string_get_view(node->as.fun_param.id->as.id.value);
        const SymbolNamespaceMask mask = SYMBOL_NS_VALUE;

        Symbol* prev = scope_lookup_current(ctx->scope, name, mask);
        if (prev != NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, node->loc, "parameter '"SV_FMT"' already exists in current scope.", SV_ARG(name));
            REPORT_NOTE_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, prev->ast->loc, "first declared here.");
            ctx->status = false;
            break;
        }

        Symbol* outer = scope_lookup(ctx->scope, name, mask);
        if (outer != NULL) {
            source_file_warn_shadow(ctx->rc, node, name, outer);
        }

        Symbol* p = symbol_create(SYMBOL_PARAMETER, name, node);
        node->symbol = p;
        scope_add_symbol(ctx->scope, p);
    } break;

    case AST_NODE_STMT_TYPEALIAS_DECL: {
        StringView name = string_get_view(node->as.stmt_typealias_decl.id->as.id.value);

        const SymbolNamespaceMask mask = SYMBOL_NS_TYPE;
        Symbol* prev = scope_lookup_current(ctx->scope, name, mask);
        if (prev != NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, node->loc, "type '"SV_FMT"' already exists in current scope.", SV_ARG(name));
            REPORT_NOTE_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, prev->ast->loc, "first declared here.");
            ctx->status = false;
            break;
        }

        Symbol* outer = scope_lookup(ctx->scope->parent, name, mask);
        if (outer != NULL) {
            source_file_warn_shadow(ctx->rc, node, name, outer);
        }

        Symbol* t = symbol_create(SYMBOL_TYPEALIAS, name, node);
        node->symbol = t;

        // if type declared at source file scope, we push it to parent (package) scope
        if (ctx->scope->kind == SCOPE_SOURCE_FILE) {
            scope_add_symbol(ctx->scope->parent, t);
        }
        else {
            scope_add_symbol(ctx->scope, t);
        }
    } break;

    case AST_NODE_STMT_VAR_ITEM: {
        StringView name = string_get_view(node->as.stmt_var_item.id->as.id.value);

        const SymbolNamespaceMask mask = SYMBOL_NS_VALUE;
        Symbol* prev = scope_lookup_current(ctx->scope, name, mask);
        if (prev != NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, node->loc, "variable '"SV_FMT"' already exists in current scope.", SV_ARG(name));
            REPORT_NOTE_LOC(ctx->rc, DIAG_SEMA_SYMBOLS, prev->ast->loc, "first declared here.");
            ctx->status = false;
            break;
        }

        Symbol* outer = scope_lookup(ctx->scope->parent, name, mask);
        if (outer != NULL) {
            source_file_warn_shadow(ctx->rc, node, name, outer);
        }

        Symbol* v = symbol_create(SYMBOL_VARIABLE, name, node);
        node->symbol = v;
        scope_add_symbol(ctx->scope, v);
    } break;

    case AST_NODE_STMT_BLOCK: {
        ctx->scope = scope_create(SCOPE_BASIC, ctx->scope, node, ctx->scope->package);
        node->scope = ctx->scope;
    } break;

    case AST_NODE_STMT_BRANCH: {
        ctx->scope = scope_create(SCOPE_BRANCH, ctx->scope, node, ctx->scope->package);
        node->scope = ctx->scope;
    } break;

    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO: {
        ctx->scope = scope_create(SCOPE_LOOP, ctx->scope, node, ctx->scope->package);
        node->scope = ctx->scope;
    } break;

    default: break;
    }
}

static inline void source_file_resolve_symbol_decls_post_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    struct ResolveDeclCtx* ctx = data;
    assert(ctx != NULL && ctx->scope != NULL);

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

static inline bool source_file_is_callee_position(const ASTNode* parent, const ASTNode* node) {
    return parent != NULL && parent->kind == AST_NODE_EXPR_CALL &&
        parent->as.expr_call.callee == node;
}

static inline bool source_file_is_member_child(const ASTNode* parent, const ASTNode* node) {
    return parent != NULL && parent->kind == AST_NODE_EXPR_MEMBER &&
        parent->as.expr_member.member == node;
}

static inline bool source_file_is_member_object(const ASTNode* parent, const ASTNode* node) {
    return parent != NULL && parent->kind == AST_NODE_EXPR_MEMBER &&
        parent->as.expr_member.object == node;
}

static inline SymbolNamespaceMask source_file_get_mask_for_place(const ASTNode* parent, const ASTNode* node) {
    if (source_file_is_callee_position(parent, node)) {
        return SYMBOL_NS_FUNC | SYMBOL_NS_VALUE;
    }
    if (source_file_is_member_object(parent, node)) {
        return SYMBOL_NS_MODULE | SYMBOL_NS_VALUE;
    }
    return SYMBOL_NS_VALUE | SYMBOL_NS_FUNC;
}

static inline SymbolNamespaceMask source_file_get_mask_for_member_from_import(const ASTNode* parent_of_member_expr, const ASTNode* member_expr) {
    if (source_file_is_callee_position(parent_of_member_expr, member_expr)) {
        return SYMBOL_NS_FUNC;
    }
    return SYMBOL_NS_VALUE | SYMBOL_NS_FUNC;
}

struct BindCtx {
    ReportCollector* rc;
    Scope* scope;
    bool             status;
};

static inline void source_file_bind_symbols_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    struct BindCtx* ctx = data;
    assert(ctx != NULL && ctx->scope != NULL);

    switch (node->kind) {
    case AST_NODE_EXPR_PLACE: {
        // Skip the "member" token of a member expr; object is processed.
        if (source_file_is_member_child(parent, node)) {
            break;
        }

        StringView name = string_get_view(node->as.expr_place.value);
        const SymbolNamespaceMask mask = source_file_get_mask_for_place(parent, node);

        Symbol* symbol = scope_lookup(ctx->scope, name, mask);
        if (symbol != NULL) {
            node->symbol = symbol;
        }
    } break;

    case AST_NODE_TYPEREF_CUSTOM: {
        StringView name = string_get_view(node->as.typeref_custom.value);

        Symbol* symbol = scope_lookup(ctx->scope, name, SYMBOL_NS_TYPE);
        if (symbol == NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, node->loc, "unresolved type '" SV_FMT "'.", SV_ARG(name));
            ctx->status = false;
            break;
        }
        node->symbol = symbol;
    } break;

    case AST_NODE_TYPEREF_QUALIFIED: {
        Vector* segments = &node->as.typeref_qualified.segments;

        if (segments->size != 2) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, node->loc, "nested qualified type is not supported yet; use 'alias.Type'.");
            ctx->status = false;
            break;
        }

        ASTNode* head = vector_at(*segments, 0);
        ASTNode* tail = vector_at(*segments, 1);

        StringView alias = string_get_view(head->as.id.value);
        StringView name = string_get_view(tail->as.id.value);

        Symbol* import_symbol = scope_lookup(ctx->scope, alias, SYMBOL_NS_MODULE);
        if (import_symbol == NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, node->loc, "unresolved import alias '"SV_FMT"'.", SV_ARG(alias));
            ctx->status = false;
            break;
        }

        Package* outer_package = import_symbol->as.import.target;
        if (outer_package == NULL || outer_package->scope == NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, node->loc, "imported package for alias '"SV_FMT"' is not available.", SV_ARG(alias));
            ctx->status = false;
            break;
        }

        Symbol* symbol = scope_lookup_current(outer_package->scope, name, SYMBOL_NS_TYPE);
        if (symbol == NULL) {
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, tail->loc, "type '"SV_FMT"' not found in imported package.", SV_ARG(name));
            ctx->status = false;
            break;
        }

        node->symbol = symbol;
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

static inline void source_file_bind_symbols_post_fn(ASTNode* parent, ASTNode* node, void* data) {
    struct BindCtx* ctx = data;
    assert(ctx != NULL && ctx->scope != NULL);

    switch (node->kind) {
    case AST_NODE_EXPR_MEMBER: {
        ASTNode* object = node->as.expr_member.object;
        ASTNode* member = node->as.expr_member.member;

        // import alias: math.fib
        if (object->symbol != NULL && object->symbol->kind == SYMBOL_IMPORT) {
            Package* pkg = object->symbol->as.import.target;

            if (pkg == NULL || pkg->scope == NULL) {
                REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, object->loc, "unresolved import; package not available.");
                ctx->status = false;
                break;
            }

            StringView mname = string_get_view(member->as.expr_place.value);
            const SymbolNamespaceMask mask = source_file_get_mask_for_member_from_import(parent, node);

            Symbol* hit = scope_lookup_current(pkg->scope, mname, mask);
            if (hit == NULL) {
                if (mask == SYMBOL_NS_FUNC) {
                    REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, member->loc, "function '"SV_FMT"' not found in imported package.", SV_ARG(mname));
                }
                else {
                    REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, member->loc, "symbol '"SV_FMT"' not found in imported package.", SV_ARG(mname));
                }
                ctx->status = false;
                break;
            }

            member->symbol = hit;
            node->symbol = hit;
        }
    } break;

    case AST_NODE_EXPR_PLACE: {
        if (node->symbol == NULL && !source_file_is_member_child(parent, node)) {
            const StringView name = string_get_view(node->as.expr_place.value);
            if (source_file_is_callee_position(parent, node)) {
                REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, node->loc, "unresolved function '"SV_FMT"'.", SV_ARG(name));
            }
            else {
                REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_BIND, node->loc, "unresolved identifier '"SV_FMT"'.", SV_ARG(name));
            }

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

static inline bool source_file_sema_is_lvalue_expr(const ASTNode* ast) {
    assert(ast != NULL);
    return ast->kind == AST_NODE_EXPR_PLACE  ||
           ast->kind == AST_NODE_EXPR_MEMBER ||
           ast->kind == AST_NODE_EXPR_INDEX;
}

static inline bool source_file_sema_is_assign_token(TokenKind kind) {
    switch (kind) {
    case TOKEN_EQUAL:
    case TOKEN_PLUS_EQUAL:
    case TOKEN_MINUS_EQUAL:
    case TOKEN_STAR_EQUAL:
    case TOKEN_SLASH_EQUAL:
    case TOKEN_PERCENT_EQUAL:
        return true;

    default:
        return false;
    }
}

static inline bool source_file_sema_is_compound_assign(TokenKind kind) {
    switch (kind) {
    case TOKEN_PLUS_EQUAL:
    case TOKEN_MINUS_EQUAL:
    case TOKEN_STAR_EQUAL:
    case TOKEN_SLASH_EQUAL:
    case TOKEN_PERCENT_EQUAL:
        return true;

    default:
        return false;
    }
}

static inline bool source_file_sema_check_condition_is_bool(ASTNode* cond, Scope* scope, TypeSystem* ts, ReportCollector* rc) {
    assert(cond != NULL && scope != NULL && ts != NULL && rc != NULL);

    Type* ct = typecheck_resolve_expr_type(scope, cond, ts, rc);
    if (ct == NULL) {
        return false;
    }

    const Type* u = type_unwrap(ct);
    if (!is_type_bool(u) && !is_type_any(u)) {
        String s = type_to_str(u);
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, cond->loc, "condition must be 'bool', got '" SV_FMT "'.", SV_ARG(s));
        string_destroy(&s);
        return false;
    }
    return true;
}

typedef struct SemaCtx SemaCtx;

struct SemaCtx {
    ReportCollector* rc;
    TypeSystem* ts;
    Scope* scope;             // current scope while walking
    const Type* current_ret_type;  // function return type when inside a function
    bool status;
};

static inline bool source_file_sema_validate_assignment(ASTNode* bin_expr, SemaCtx* ctx) {
    assert(bin_expr != NULL && ctx != NULL);
    assert(bin_expr->kind == AST_NODE_EXPR_BINARY);

    const TokenKind op = bin_expr->as.expr_binary.op;
    if (!source_file_sema_is_assign_token(op)) {
        return true; // not an assignment, nothing to validate here
    }

    ASTNode* L = bin_expr->as.expr_binary.lhs;
    ASTNode* R = bin_expr->as.expr_binary.rhs;
    assert(L != NULL && R != NULL);

    if (!source_file_sema_is_lvalue_expr(L)) {
        REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, L->loc, "left-hand side of assignment is not assignable.");
        ctx->status = false;
        return false;
    }

    Type* lt = typecheck_resolve_expr_type(ctx->scope, L, ctx->ts, ctx->rc);
    Type* rt = typecheck_resolve_expr_type(ctx->scope, R, ctx->ts, ctx->rc);

    if (lt == NULL || rt == NULL) {
        ctx->status = false;
        return false;
    }

    const Type* l = type_unwrap(lt);
    const Type* r = type_unwrap(rt);

    // for compound assignments, require integer arithmetic for now
    if (source_file_sema_is_compound_assign(op)) {
        if (!is_type_integer(l) || !is_type_integer(r)) {
            String ls = type_to_str(l), rs = type_to_str(r);
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, bin_expr->loc, "compound assignment requires integers, got '" SV_FMT "' and '" SV_FMT "'.", SV_ARG(ls), SV_ARG(rs));
            string_destroy(&ls);
            string_destroy(&rs);
            ctx->status = false;
            return false;
        }
    }

    // basic assignability (arrays must match exact size
    if (!is_type_compatible(l, r)) {
        String ls = type_to_str(l), rs = type_to_str(r);
        REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, bin_expr->loc, "cannot assign '" SV_FMT "' to '" SV_FMT "'.", SV_ARG(rs), SV_ARG(ls));
        string_destroy(&ls);
        string_destroy(&rs);
        ctx->status = false;
        return false;
    }

    return true;
}

static inline bool source_file_sema_validate_return(ASTNode* ret_stmt, SemaCtx* ctx) {
    assert(ret_stmt != NULL && ctx != NULL);

    // returning outside of a function; report a sensible error
    if (ctx->current_ret_type == NULL) {
        REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, ret_stmt->loc, "return statement is not inside a function.");
        ctx->status = false;
        return false;
    }

    ASTNode* value = ret_stmt->as.stmt_return.expr;
    const Type* expected = type_unwrap(ctx->current_ret_type);

    // returning from a void function
    if (value == NULL) {
        if (!is_type_builtin(expected, TYPE_BUILTIN_VOID) && !is_type_any(expected)) {
            String es = type_to_str(expected);
            REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, ret_stmt->loc, "returning no value from function returning '" SV_FMT "'.", SV_ARG(es));
            string_destroy(&es);
            ctx->status = false;
            return false;
        }
        return true;
    }

    // value return
    Type* vt = typecheck_resolve_expr_type(ctx->scope, value, ctx->ts, ctx->rc);
    if (vt == NULL) {
        ctx->status = false;
        return false;
    }

    const Type* v = type_unwrap(vt);

    if (!is_type_compatible(expected, v)) {
        String es = type_to_str(expected), vs = type_to_str(v);
        REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA_TYPES, value->loc, "returned type '" SV_FMT "' is not compatible with function return type '" SV_FMT "'.", SV_ARG(vs), SV_ARG(es));
        string_destroy(&es);
        string_destroy(&vs);
        ctx->status = false;
        return false;
    }

    return true;
}

static inline void source_file_validate_semantic_pre(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    SemaCtx* ctx = (SemaCtx*)data;
    assert(ctx != NULL);

    switch (node->kind) {
    case AST_NODE_FUN_DECL: {
        // enter function scope and capture return type
        assert(node->symbol != NULL);
        if (!symbol_resolve_type(node->symbol, ctx->ts, ctx->rc)) {
            ctx->status = false;
        }

        const Type* fnty = type_unwrap(node->symbol->as.typed.type);
        const Type* ret = NULL;
        if (fnty != NULL && fnty->kind == TYPE_FUNCTION) {
            ret = fnty->as.fun.ret;
        }

        // Push scope
        assert(node->scope != NULL);
        ctx->scope = node->scope;
        ctx->current_ret_type = ret;
    } break;

    case AST_NODE_STMT_BLOCK: {
        assert(node->scope != NULL);
        ctx->scope = node->scope;
    } break;

    case AST_NODE_STMT_EXPR: {
        // if it's an assignment binary, validate
        ASTNode* e = node->as.stmt_expr.expr; // adjust if field differs
        if (e != NULL && e->kind == AST_NODE_EXPR_BINARY) {
            if (source_file_sema_is_assign_token(e->as.expr_binary.op)) {
                source_file_sema_validate_assignment(e, ctx);
            }
        }
    } break;

    case AST_NODE_STMT_RETURN: {
        source_file_sema_validate_return(node, ctx);
    } break;

    case AST_NODE_STMT_BRANCH: {
        ASTNode* cond = node->as.stmt_branch.expr;
        if (cond != NULL) {
            source_file_sema_check_condition_is_bool(cond, ctx->scope, ctx->ts, ctx->rc);
        }

        assert(node->scope != NULL);
        ctx->scope = node->scope;
    } break;

    case AST_NODE_STMT_WHILE: {
        ASTNode* cond = node->as.stmt_while.expr;
        if (cond != NULL) {
            source_file_sema_check_condition_is_bool(cond, ctx->scope, ctx->ts, ctx->rc);
        }

        assert(node->scope != NULL);
        ctx->scope = node->scope;
    } break;

    case AST_NODE_STMT_DO: {
        ASTNode* cond = node->as.stmt_do.expr;
        if (cond != NULL) {
            source_file_sema_check_condition_is_bool(cond, ctx->scope, ctx->ts, ctx->rc);
        }

        assert(node->scope != NULL);
        ctx->scope = node->scope;
    } break;

    default: break;
    }
}

static inline void source_file_validate_semantic_post(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;
    SemaCtx* ctx = (SemaCtx*)data;
    assert(ctx != NULL);

    switch (node->kind) {
    case AST_NODE_FUN_DECL: {
        // Leave function scope
        assert(ctx->scope != NULL && ctx->scope->parent != NULL);
        ctx->scope = ctx->scope->parent;
        ctx->current_ret_type = NULL;
    } break;

    case AST_NODE_STMT_BLOCK:
    case AST_NODE_STMT_BRANCH:
    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO: {
        assert(ctx->scope != NULL && ctx->scope->parent != NULL);
        ctx->scope = ctx->scope->parent;
    } break;

    default: break;
    }
}

SourceFile* source_file_create(StringView path, ReportCollector* rc) {
    assert(rc != NULL);

    String content = STRING_EMPTY;
    if (source_file_load_content(path, &content, rc) != FILE_LOAD_OK) {
        return NULL;
    }

    SourceFile* source_file = malloc(sizeof(SourceFile));
    assert(source_file != NULL);

    source_file->path = path;
    source_file->content = content;

    source_file->package = NULL;
    source_file->scope = NULL;
    source_file->ast = NULL;

    source_file->imports = (Vector){ 0 };
    source_file->imports_resolved = false;

    source_file->cfg_by_fun = (Hashmap){ 0 };
    source_file->cfg_built = false;

    source_file->rc = rc;

    REPORT_INFO(source_file->rc, DIAG_DRIVER_FS, "loaded source: '"SV_FMT"'.", SV_ARG(path));

    return source_file;
}

void source_file_destroy(SourceFile* source_file) {
    if (source_file == NULL) {
        return;
    }

    string_destroy(&source_file->content);
    vector_destroy(&source_file->imports);
    ast_node_destroy(source_file->ast);

    hashmap_destroy(&source_file->cfg_by_fun);

    free(source_file);
}

bool source_file_parse_ast(SourceFile* source_file) {
    assert(source_file != NULL);

    REPORT_DEBUG(source_file->rc, DIAG_PARSER, "parsing: '"SV_FMT"'.", SV_ARG(source_file->path));

    TokenStream ts = token_stream_create(0, source_file->path, string_get_view(source_file->content), source_file->rc);
    ASTParser parser = ast_parser_create(&ts);

    source_file->ast = ast_node_create(AST_NODE_SOURCE_FILE, token_stream_peek_next(&ts)->loc);
    source_file->ast->as.source_file.entities = vector_create(
        SOURCE_FILE_DEFAULT_ENTITIES_COUNT,
        VECTOR_SPECS(ASTNode*, &ast_node_destroy)
    );

    bool is_good = true;

    while (!is_token_stream_end(&ts)) {
        const u32 before_idx = ts.idx;
        ASTNode* node = ast_parser_parse_package_entity(&parser);

        vector_push_back(&source_file->ast->as.source_file.entities, &node);
        source_file->ast->loc.end = node->loc.end;

        if (node->kind == AST_NODE_ERROR) {
            is_good = false;
            ast_parser_sync_to_package(&parser);
            ast_parser_one_step_guard(&parser, before_idx);
            continue;
        }

        if (node->kind == AST_NODE_IMPORT_DECL) {
            if (source_file->imports.raw == NULL) {
                source_file->imports = vector_create(
                    SOURCE_FILE_DEFAULT_IMPORT_COUNT,
                    VECTOR_SPECS(ImportEntry, NULL)
                );
            }

            const ASTNode* path_node = node->as.import_decl.path;
            StringView raw = string_get_view(path_node->as.expr_literal.value);

            ImportEntry entry = {
                .name = STRING_VIEW_EMPTY,
                .node = node,
                .target = NULL,
                .base = IMPORT_BASE_RELATIVE,
                .collection_name = STRING_VIEW_EMPTY,
                .package_path = STRING_VIEW_EMPTY,
            };

            source_file_split_import_path(raw, &entry.base, &entry.collection_name, &entry.package_path);
            vector_push_back(&source_file->imports, &entry);
        }
    }

    token_stream_destroy(&ts);

    REPORT_INFO(source_file->rc, DIAG_PARSER, "parsed: '"SV_FMT"' (entities=%d,imports=%d)",
        SV_ARG(source_file->path),
        source_file->ast->as.source_file.entities.size,
        source_file->imports.size
    );

    return is_good;
}

bool source_file_resolve_imports(SourceFile* source_file) {
    assert(source_file != NULL);

    if (source_file->imports_resolved) {
        REPORT_DEBUG(source_file->rc, DIAG_DRIVER_IMPORTS, "imports already resolved for '"SV_FMT"'.", SV_ARG(source_file->path));
        return true;
    }

    bool is_good = true;

    for (u32 i = 0; i < source_file->imports.size; ++i) {
        ImportEntry* e = vector_at(source_file->imports, i);

        // resolve target package
        e->target = compiler_resolve_import(source_file->package->compiler, source_file, e);
        if (e->target == NULL) {
            REPORT_ERROR_LOC(source_file->rc, DIAG_DRIVER_IMPORTS, e->node->loc, "failed to resolve import '"SV_FMT"'.", SV_ARG(e->package_path));
            is_good = false;
            continue;
        }

        // compute visible name (alias or stem)
        e->name = source_file_derive_import_name(e);
        REPORT_DEBUG(source_file->rc, DIAG_DRIVER_IMPORTS, "import '"SV_FMT"': base=%d, name='"SV_FMT"'", SV_ARG(e->package_path), (i32)e->base, SV_ARG(e->name));
    }

    source_file->imports_resolved = true;
    return is_good;
}

bool source_file_resolve_symbol_decls(SourceFile* source_file) {
    assert(source_file != NULL);

    if (source_file->scope != NULL) {
        return true;
    }

    // Create source-file scope chained to the package
    source_file->scope = scope_create(SCOPE_SOURCE_FILE, source_file->package->scope, source_file->ast, source_file->package);

    bool is_good = true;
    // Bring imports into this scope (alias imports or open-imports)
    if (!source_file_resolve_import_symbols(source_file)) {
        is_good = false;
    }

    struct ResolveDeclCtx ctx = {
        .rc = source_file->rc,
        .scope = source_file->scope,
        .status = is_good,
    };

    ASTVisitor v = {
        .data = &ctx,
        .pre_fn = &source_file_resolve_symbol_decls_pre_fn,
        .post_fn = &source_file_resolve_symbol_decls_post_fn,
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

bool source_file_bind_symbols(SourceFile* source_file) {
    assert(source_file != NULL);
    assert(source_file->scope != NULL);

    bool is_good = true;
    // Bring unqualified open imports
    if (!source_file_bind_unqualified_outer_scopes(source_file)) {
        is_good = false;
    }

    struct BindCtx ctx = {
        .rc = source_file->rc,
        .scope = source_file->scope,
        .status = is_good,
    };

    ASTVisitor v = {
        .data = &ctx,
        .pre_fn = &source_file_bind_symbols_pre_fn,
        .post_fn = &source_file_bind_symbols_post_fn,
    };

    const Vector* nodes = &source_file->ast->as.source_file.entities;
    for (u32 i = 0; i < nodes->size; ++i) {
        ASTNode* node = vector_at(*nodes, i);
        ast_visit_with(NULL, node, &v);
    }

    return ctx.status;
}

bool source_file_validate_semantics(SourceFile* source_file) {
    assert(source_file != NULL);
    assert(source_file->scope != NULL);
    assert(source_file->ast != NULL);


    SemaCtx ctx = {
        .rc = source_file->rc,
        .ts = &source_file->package->compiler->ts,
        .scope = source_file->scope,
        .current_ret_type = NULL,
        .status = true,
    };

    ASTVisitor v = {
        .data = &ctx,
        .pre_fn = &source_file_validate_semantic_pre,
        .post_fn = &source_file_validate_semantic_post,
    };

    const Vector* nodes = &source_file->ast->as.source_file.entities;
    for (u32 i = 0; i < nodes->size; ++i) {
        ASTNode* node = vector_at(*nodes, i);
        ast_visit_with(NULL, node, &v);
    }

    return ctx.status;
}

bool source_file_build_cfgs(SourceFile* source_file) {
    assert(source_file != NULL);
    assert(source_file->ast != NULL);

    if (source_file->cfg_built) {
        REPORT_DEBUG(source_file->rc, DIAG_SEMA_CFG, "CFGs already built: '"SV_FMT"'.", SV_ARG(source_file->path));
        return true;
    }

    source_file->cfg_by_fun = hashmap_create(
        SOURCE_FILE_DEFAULT_CFG_BY_FUN_SIZE,
        HASHMAP_KEY_SPECS(ASTNode*, &item_ptr_hash, &item_ptr_eq, NULL),
        HASHMAP_VALUE_SPECS(CFGFunction*, &cfg_function_destroy)
    );

    const Vector* entities = &source_file->ast->as.source_file.entities;
    for (u32 i = 0; i < entities->size; ++i) {
        ASTNode* node = vector_at(*entities, i);

        if (node->kind != AST_NODE_FUN_DECL) {
            continue;
        }

        CFGFunction* cfg = cfg_function_build(node, source_file->rc);
        hashmap_insert(&source_file->cfg_by_fun, &node, &cfg);

        StringView fun_name = string_get_view(node->as.fun_decl.sign->as.fun_sign.id->as.id.value);
        REPORT_NOTE_LOC(source_file->rc, DIAG_SEMA_CFG, node->loc, "CFG built for function '"SV_FMT"'.", SV_ARG(fun_name));
    }

    source_file->cfg_built = true;
    return true;
}