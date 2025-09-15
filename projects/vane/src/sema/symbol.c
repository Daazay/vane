#include "vane/sema/symbol.h"

#include <stdlib.h>

#include "vane/ast/ast_node.h"

#include "vane/sema/type.h"
#include "vane/sema/type_system.h"
#include "vane/sema/scope.h"
#include "vane/sema/ceval.h"
#include "vane/sema/typecheck.h"

#include "vane/diagnostic/diagnostic_tags.h"
#include "vane/scanner/token_kind.h"

const char* symbol_kind_get_name(SymbolKind kind) {
    switch (kind) {
    case SYMBOL_IMPORT:    return "import";
    case SYMBOL_TYPEALIAS: return "type";
    case SYMBOL_FUNCTION:  return "func";
    case SYMBOL_PARAMETER: return "param";
    case SYMBOL_VARIABLE:  return "var";
    default:
        unreachable();
        return NULL;
    }
}

Symbol* symbol_create(SymbolKind kind, StringView name, struct ASTNode* ast) {
    Symbol* symbol = malloc(sizeof(Symbol));
    assert(symbol != NULL);

    symbol->kind  = kind;
    symbol->name  = name;
    symbol->ast   = ast;
    symbol->scope = NULL;

    symbol->as.import.target    = NULL;
    symbol->as.typed.type       = NULL;
    symbol->as.typed.type_state = 0;

    return symbol;
}

void symbol_destroy(Symbol* symbol) {
    if (symbol == NULL) {
        return;
    }

    free(symbol);
}

static inline Type* symbol_try_get_type_from_ast(Scope* scope, ASTNode* ast, TypeSystem* ts, ReportCollector* rc);

static inline Type* symbol_try_get_type_from_typeref_custom(Scope* scope, ASTNode* ast, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && ast != NULL && rc != NULL);

    StringView id = string_get_view(ast->as.typeref_custom.value);

    // fast path
    Type* builtin = type_system_get_builtin(ts, id);
    if (builtin != NULL) {
        return builtin;
    }

    Symbol* symbol = scope_lookup_any(scope, id);
    if (symbol == NULL) {
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, ast->loc, "unknown type '"SV_FMT"'.", SV_ARG(id));
        return NULL;
    }
    if (!symbol_resolve_type(symbol, ts, rc)) {
        return NULL;
    }

    return symbol->as.typed.type;
}

static inline Type* symbol_try_get_type_from_typeref_qualified(Scope* scope, ASTNode* ast, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ast != NULL && ts != NULL && rc != NULL);

    if (ast->symbol == NULL) {
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, ast->loc, "unresolved qualified type.");
        return type_system_get_unresolved_or_create(ts, STR_LIT("unresolved"));
    }

    if (!symbol_resolve_type(ast->symbol, ts, rc)) {
        return type_system_get_unresolved_or_create(ts, ast->symbol->name);
    }
    return ast->symbol->as.typed.type;
}

static inline Type* symbol_try_get_type_from_typeref_ptr(Scope* scope, ASTNode* ast, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && ast != NULL && rc != NULL);

    Type* base = symbol_try_get_type_from_ast(scope, ast->as.typeref_ptr.typeref, ts, rc);
    return base != NULL ? type_system_get_pointer_or_create(ts, base) : NULL;
}

static inline Type* symbol_try_get_type_from_typeref_arr(Scope* scope, ASTNode* ast, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && ast != NULL && rc != NULL);

    Type* elem_type = symbol_try_get_type_from_ast(scope, ast->as.typeref_arr.typeref, ts, rc);
    if (elem_type == NULL) {
        return NULL;
    }

    // evaluate array length as a constant expression
    if (ast->as.typeref_arr.size_expr != NULL) {
        CEvalCtx ctx = { .rc = rc, .ts = ts, .scope = scope, .resolve_fn = NULL };
        u64 len = 0;
        if (!ceval_eval_array_size(ast->as.typeref_arr.size_expr, &ctx, &len)) {
            return NULL;
        }
        return type_system_get_array_or_create(ts, elem_type, (u32)len);
    }

    // no size -> slice
    return type_system_get_slice_or_create(ts, elem_type);
}

static inline Type* symbol_try_get_type_from_typeref_fun(Scope* scope, ASTNode* ast, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && ast != NULL && rc != NULL);

    const Vector* ast_params = &ast->as.typeref_fun.params;

    Type** param_types = NULL;
    u32 param_count = ast_params->size;

    if (param_count > 0) {
        param_types = malloc(sizeof(Type*) * param_count);
        assert(param_types != NULL);

        for (u32 i = 0; i < param_count; ++i) {
            ASTNode* ast_param = vector_at(*ast_params, i);
            ASTNode* ast_param_type = ast_param->as.fun_param.typeref;

            if (ast_param_type == NULL) {
                param_types[i] = type_system_get_builtin(ts, STR_LIT("any"));
            }
            else {
                param_types[i] = symbol_try_get_type_from_ast(scope, ast_param_type, ts, rc);
            }
        }
    }

    Type* rt = NULL;
    if (ast->as.typeref_fun.typeref == NULL) {
        rt = type_system_get_builtin(ts, STR_LIT("any"));
    }
    else {
        rt = symbol_try_get_type_from_ast(scope, ast->as.typeref_fun.typeref, ts, rc);
    }

    return type_system_get_fun_or_create(ts, param_types, param_count, rt);
}

static inline Type* symbol_try_get_type_from_stmt_var_item(Scope* scope, ASTNode* ast, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && ast != NULL && rc != NULL);
    assert(ast->kind == AST_NODE_STMT_VAR_ITEM);
    assert(ast->symbol != NULL && "variable item must already have a symbol");

    ASTNode* typeref = ast->as.stmt_var_item.typeref;
    ASTNode* expr = ast->as.stmt_var_item.expr;

    // 1) explicit type annotation
    if (typeref != NULL) {
        Type* t = symbol_try_get_type_from_ast(scope, typeref, ts, rc);
        if (t == NULL) {
            return NULL;
        }

        // if there is an initializer, ensure it's compatible
        if (expr != NULL) {
            Type* init_t = typecheck_resolve_expr_type(scope, expr, ts, rc);
            if (init_t != NULL && !is_type_compatible(t, init_t)) {
                String l_s = type_to_str(t);
                String r_s = type_to_str(init_t);

                REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "initializer type '" SV_FMT "' does not match variable type '" SV_FMT "'.", SV_ARG(r_s), SV_ARG(l_s));

                string_destroy(&l_s);
                string_destroy(&r_s);
                return NULL;
            }
        }
        return t;
    }

    // 2) no annotation → infer from initializer
    if (expr != NULL) {
        return typecheck_resolve_expr_type(scope, expr, ts, rc);
    }

    // 3) no annotation, no initializer → default to 'any' with a note
    REPORT_NOTE_LOC(rc, DIAG_SEMA_TYPES, ast->loc, "variable has no type annotation and no initializer; defaulting to 'any'.");
    return type_system_get_builtin(ts, STR_LIT("any"));
}

static inline Type* symbol_try_get_type_from_ast(Scope* scope, ASTNode* ast, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && ast != NULL && rc != NULL);

    switch (ast->kind) {
    case AST_NODE_TYPEREF_CUSTOM:    return symbol_try_get_type_from_typeref_custom(scope, ast, ts, rc);
    case AST_NODE_TYPEREF_QUALIFIED: return symbol_try_get_type_from_typeref_qualified(scope, ast, ts, rc);
    case AST_NODE_TYPEREF_PTR:       return symbol_try_get_type_from_typeref_ptr(scope, ast, ts, rc);
    case AST_NODE_TYPEREF_ARR:       return symbol_try_get_type_from_typeref_arr(scope, ast, ts, rc);
    case AST_NODE_TYPEREF_FUN:       return symbol_try_get_type_from_typeref_fun(scope, ast, ts, rc);
    case AST_NODE_STMT_VAR_ITEM:     return symbol_try_get_type_from_stmt_var_item(scope, ast, ts, rc);

    default:
        unreachable();
        return NULL;
    }
}

static inline bool symbol_resolve_typealias_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);
    assert(symbol->ast != NULL);

    ASTNode* ast_type = symbol->ast->as.stmt_typealias_decl.typeref;
    StringView name = string_get_view(symbol->ast->as.stmt_typealias_decl.id->as.id.value);

    Type* target_type = symbol_try_get_type_from_ast(symbol->scope, ast_type, ts, rc);
    if (target_type != NULL) {
        symbol->as.typed.type = type_system_get_alias_or_create(ts, name, target_type);
    }
    return true;
}

static inline bool symbol_resolve_function_param_type(ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(node != NULL && ts != NULL && rc != NULL);
    assert(node->symbol != NULL);

    Symbol* symbol = node->symbol;
    ASTNode* typeref = node->as.fun_param.typeref;

    Type* type = NULL;
    if (typeref == NULL) {
        type = type_system_get_builtin(ts, STR_LIT("any"));
    }
    else {
        type = symbol_try_get_type_from_ast(symbol->scope, typeref, ts, rc);
    }

    symbol->as.typed.type = type;
    return true;
}

static inline bool symbol_resolve_function_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);
    assert(symbol->ast != NULL);

    ASTNode* sign = symbol->ast->as.fun_decl.sign;
    Vector* params = &sign->as.fun_sign.params;

    Type** param_types = NULL;
    u32 param_count = params->size;

    if (param_count > 0) {
        param_types = malloc(sizeof(Type*) * param_count);
        assert(param_types != NULL);

        for (u32 i = 0; i < param_count; ++i) {
            ASTNode* param = vector_at(*params, i);
            symbol_resolve_function_param_type(param, ts, rc);
            param_types[i] = param->symbol->as.typed.type;
        }
    }

    Type* ret_type = NULL;
    ASTNode* ast_ret_type = sign->as.fun_sign.typeref;
    if (ast_ret_type == NULL) {
        ret_type = type_system_get_builtin(ts, STR_LIT("any"));
    }
    else {
        ret_type = symbol_try_get_type_from_ast(symbol->scope, ast_ret_type, ts, rc);
    }

    symbol->as.typed.type = type_system_get_fun_or_create(ts, param_types, param_count, ret_type);
    return true;
}

static inline bool symbol_resolve_variable_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    symbol->as.typed.type = symbol_try_get_type_from_stmt_var_item(symbol->scope, symbol->ast, ts, rc);
    return symbol->as.typed.type != NULL;
}

bool symbol_resolve_type(Symbol* symbol, struct TypeSystem* ts, struct ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    if (symbol->as.typed.type_state == TYPE_STATE_RESOLVED) {
        return true;
    }

    if (symbol->as.typed.type_state == TYPE_STATE_RESOLVING) {
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, symbol->ast ? symbol->ast->loc : (TokenLoc) { 0 },
            "cyclic type alias for '" SV_FMT "'.", SV_ARG(symbol->name)
        );
        symbol->as.typed.type = type_system_get_unresolved_or_create(ts, symbol->name);
        symbol->as.typed.type_state = TYPE_STATE_RESOLVED;
        return false;
    }

    symbol->as.typed.type_state = TYPE_STATE_RESOLVING;

    bool is_good = false;
    switch (symbol->kind) {
    case SYMBOL_IMPORT:    is_good = true; break;
    case SYMBOL_TYPEALIAS: is_good = symbol_resolve_typealias_type(symbol, ts, rc); break;
    case SYMBOL_FUNCTION:  is_good = symbol_resolve_function_type(symbol, ts, rc); break;
    case SYMBOL_PARAMETER: is_good = true; break; // resolved when visiting function signature
    case SYMBOL_VARIABLE:  is_good = symbol_resolve_variable_type(symbol, ts, rc); break;

    default: {
        unreachable();
        is_good = false;
        break;
    }
    }

    if (!is_good && symbol->as.typed.type == NULL) {
        symbol->as.typed.type = type_system_get_unresolved_or_create(ts, symbol->name);
    }

    symbol->as.typed.type_state = TYPE_STATE_RESOLVED;
    return is_good;
}