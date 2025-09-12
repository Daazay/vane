#include "vane/sema/symbol.h"

#include <stdlib.h>

#include "vane/ast/ast_node.h"

#include "vane/sema/type.h"
#include "vane/sema/type_system.h"
#include "vane/sema/scope.h"

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

    symbol->kind = kind;
    symbol->name = name;
    symbol->ast = ast;

    symbol->as.import.target = NULL;
    symbol->as.typed.type = NULL;
    symbol->as.typed.type_state = 0;

    return symbol;
}

void symbol_destroy(Symbol* symbol) {
    if (symbol == NULL) {
        return;
    }

    free(symbol);
}

static inline Type* try_to_get_type_from_ast(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc);

static inline Type* try_resolve_expr_type(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc);

static inline Type* try_resolve_type_for_expr_literal(ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(expr != NULL && ts != NULL && rc != NULL);

    switch (expr->as.expr_literal.kind) {
    case TOKEN_LITERAL_BOOL:   return type_system_get_builtin(ts, STR_LIT("bool"));
    case TOKEN_LITERAL_CHAR:   return type_system_get_builtin(ts, STR_LIT("u8"));
    // TODO: get int size from size of evaluated literal?
    case TOKEN_LITERAL_DEC:    return type_system_get_builtin(ts, STR_LIT("unsized int"));
    case TOKEN_LITERAL_OCT:    return type_system_get_builtin(ts, STR_LIT("unsized int"));
    case TOKEN_LITERAL_HEX:    return type_system_get_builtin(ts, STR_LIT("unsized int"));
    case TOKEN_LITERAL_BIN:    return type_system_get_builtin(ts, STR_LIT("unsized int"));
    case TOKEN_LITERAL_STRING:
        Type* u8_type = type_system_get_builtin(ts, STR_LIT("u8"));
        // TODO: count runes for literal and return sized array
        return type_system_get_array_or_create(ts, u8_type, 0);

    default:
        unreachable();
        return NULL;
    }
}

static inline Type* try_resolve_type_for_expr_place(ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(expr != NULL && ts != NULL && rc != NULL);
    assert(expr->symbol != NULL && "must be already bound at bind stage");

    Symbol* sym = expr->symbol;

    if (!symbol_resolve_type(sym, ts, rc)) {
        return NULL;
    }

    return sym->as.typed.type;
}

static inline Type* try_resolve_type_for_expr_call(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(expr != NULL && ts != NULL && rc != NULL);

    ASTNode* callee = expr->as.expr_call.callee;
    Type* callee_t = try_resolve_expr_type(scope, callee, ts, rc);

    if (callee_t == NULL || callee_t->kind != TYPE_FUNCTION) {
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "call target is not a function.");
        return NULL;
    }

    return (Type*)callee_t->as.fun.ret;
}

static inline Type* try_resolve_type_for_expr_member(ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(expr != NULL && ts != NULL && rc != NULL);
    assert(expr->symbol != NULL && "must be already bound at bind stage");

    if (!symbol_resolve_type(expr->symbol, ts, rc)) {
        return NULL;
    }

    return expr->symbol->as.typed.type;
}

static inline Type* try_resolve_type_for_expr_unary(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(expr != NULL && ts != NULL && rc != NULL);

    const bool is_prefix = (expr->kind == AST_NODE_EXPR_PREFIX_UNARY);
    TokenKind op = is_prefix ? expr->as.expr_prefix_unary.op
                             : expr->as.expr_postfix_unary.op;
    ASTNode*  operand = is_prefix ? expr->as.expr_prefix_unary.rhs
                                  : expr->as.expr_postfix_unary.lhs;

    Type* t = try_resolve_expr_type(scope, operand, ts, rc);
    const Type* u = type_unwrap(t);

    // '++', '--' : require integer; result is operand type
    if (op == TOKEN_PLUS_PLUS || op == TOKEN_MINUS_MINUS) {
        if (!is_type_integer(u) && !is_type_any(u)) {
            String type_s = type_to_str(u);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "operator required integer operand, got '"SV_FMT"'.", SV_ARG(type_s));
            string_destroy(&type_s);
            return NULL;
        }
        return t;
    }

    // Logical not
    if (op == TOKEN_EXCLAIM) {
        if (!is_type_bool(u) && !is_type_any(u)) {
            String type_s = type_to_str(u);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "logical '!' requires 'bool' operand, got '"SV_FMT"'.", SV_ARG(type_s));
            string_destroy(&type_s);
            return NULL;
        }
        return type_system_get_builtin(ts, STR_LIT("bool"));
    }

    // Unary '+', '/', '-'
    if (op == TOKEN_PLUS || op == TOKEN_MINUS) {
        if (!is_type_integer(u) && !is_type_any(u)) {
            String type_s = type_to_str(u);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "unary '%s' requires integer operand, got '"SV_FMT"'.",
                token_kind_get_value(op), SV_ARG(type_s));
            string_destroy(&type_s);
            return NULL;
        }
        return t;
    }

    // bitwise not '~'
    if (op == TOKEN_TILDE) {
        if (!is_type_integer(u) && !is_type_any(u)) {
            String type_s = type_to_str(u);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "bitwise '~' requires integer operand, got '"SV_FMT"'.", SV_ARG(type_s));
            string_destroy(&type_s);
            return NULL;
        }
        return t;
    }

    REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "unknown unary operator '%s'.", token_kind_get_value(op));
    return NULL;
}

static inline Type* choose_numeric_binop_result(TypeSystem* ts, const Type* l, const Type* r, ReportCollector* rc, TokenLoc loc, TokenKind op) {
    assert(ts != NULL && rc != NULL);

    l = type_unwrap(l);
    r = type_unwrap(r);

    if (l == NULL || r == NULL) {
        return NULL;
    }

    if (is_type_any(l) || is_type_any(r)) {
        return type_system_get_builtin(ts, STR_LIT("any"));
    }

    // both sized & same kind
    const bool l_uns = is_type_unsized_integer(l);
    const bool r_uns = is_type_unsized_integer(r);

    TypeBuiltinKind lk = TYPE_BUILTIN_UNKNOWN;
    TypeBuiltinKind rk = TYPE_BUILTIN_UNKNOWN;
    const bool l_sized = is_type_sized_integer(l, &lk);
    const bool r_sized = is_type_sized_integer(r, &rk);

    if ((l_uns || l_sized) && (r_uns || r_sized)) {
        // If one is sized and the other unsized -> pick the sized one
        if (l_sized && r_uns) {
            return (Type*)l;
        }
        if (r_sized && l_uns) {
            return (Type*)r;
        }

        // Both unsized -> keep unsized
        if (l_uns && r_uns) {
            return type_system_get_builtin(ts, STR_LIT("unsized int"));
        }

        // Both sized -> choose the wider; if equal width and different signedness, prefer unsigned
        if (l_sized && r_sized) {
            const u32 lb = type_builtin_kind_get_size(lk);
            const u32 rb = type_builtin_kind_get_size(rk);
            if (lb > rb) {
                return (Type*)l;
            }
            if (rb > lb) {
                return (Type*)r;
            }

            // same width, pick unsigned if any
            if (is_type_builtin_kind_unsigned(lk)) {
                return (Type*)l;
            }
            if (is_type_builtin_kind_unsigned(rk)) {
                return (Type*)r;
            }
            return (Type*)l; // both signed, same width
        }
    }

    // otherwise error
    String l_s = type_to_str(l);
    String r_s = type_to_str(r);
    REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, loc, "operator '%s' is not defined for '"SV_FMT"' and '"SV_FMT"'.", token_kind_get_value(op), l_s, r_s);
    string_destroy(&l_s);
    string_destroy(&r_s);
    return NULL;
}

static inline bool is_type_both_bool(const Type* l, const Type* r) {
    return is_type_bool(l) && is_type_bool(r);
}

static inline bool is_type_both_integers(const Type* l, const Type* r) {
    return is_type_integer(l) && is_type_integer(r);
}

static inline Type* try_resolve_type_for_expr_binary(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(ts != NULL && rc != NULL);

    TokenKind op  = expr->as.expr_binary.op;
    ASTNode*  LHS = expr->as.expr_binary.lhs;
    ASTNode*  RHS = expr->as.expr_binary.rhs;

    Type* LT = try_resolve_expr_type(scope, LHS, ts, rc);
    Type* RT = try_resolve_expr_type(scope, RHS, ts, rc);

    const Type* L = type_unwrap(LT);
    const Type* R = type_unwrap(RT);

    // Short-circuit on ANY to reduce noise
    if (is_type_any(L) || is_type_any(R)) {
        // Logical ops still return bool
        if (op == TOKEN_AMP_AMP     || op == TOKEN_PIPE_PIPE     ||
            op == TOKEN_EQUAL_EQUAL || op == TOKEN_EXCLAIM_EQUAL ||
            op == TOKEN_LESS        || op == TOKEN_LESS_EQUAL    ||
            op == TOKEN_GREATER     || op == TOKEN_GREATER_EQUAL)
            return type_system_get_builtin(ts, STR_LIT("bool"));

        return type_system_get_builtin(ts, STR_LIT("any"));
    }

    // logical '&&' and '||'
    if (op == TOKEN_AMP_AMP || op == TOKEN_PIPE_PIPE) {
        if (!is_type_both_bool(L, R)) {
            String l_s = type_to_str(L);
            String r_s = type_to_str(R);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "logical operator '%s' required 'bool' and 'bool', got '"SV_FMT" and '"SV_FMT"'.",
                token_kind_get_value(op), SV_ARG(l_s), SV_ARG(r_s)
            );
            string_destroy(&l_s);
            string_destroy(&r_s);
            return NULL;
        }
        return type_system_get_builtin(ts, STR_LIT("bool"));
    }

    // Equality '==', '!='  (allow same type or integer-ish compatible)
    if (op == TOKEN_EQUAL_EQUAL || op == TOKEN_EXCLAIM_EQUAL) {
        const bool ints = is_type_both_integers(L, R);
        const bool ok = ints || type_eq_type(L, R) || is_type_any(L) || is_type_any(R);
        if (!ok) {
            String l_s = type_to_str(L);
            String r_s = type_to_str(R);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "comparison requires compatible types, got '"SV_FMT" and '"SV_FMT"'.", SV_ARG(l_s), SV_ARG(r_s));
            string_destroy(&l_s);
            string_destroy(&r_s);
            return NULL;
        }
        return type_system_get_builtin(ts, STR_LIT("bool"));
    }

    // Relational '<', '<=', '>', '>=' -> integers only
    if (op == TOKEN_LESS || op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL) {
        if (!is_type_both_integers(L, R)) {
            String l_s = type_to_str(L);
            String r_s = type_to_str(R);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "relational operator required integets, got '"SV_FMT" and '"SV_FMT"'.", SV_ARG(l_s), SV_ARG(r_s));
            string_destroy(&l_s);
            string_destroy(&r_s);
            return NULL;
        }
        return type_system_get_builtin(ts, STR_LIT("bool"));
    }

    // Arithmetic '+', '-', '*', '/', '%'
    if (op == TOKEN_PLUS || op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_PERCENT) {
        return choose_numeric_binop_result(ts, L, R, rc, expr->loc, op);
    }

    // Bitwise '&', '|', '^', '<<', '>>'
    if (op == TOKEN_AMP || op == TOKEN_PIPE || op == TOKEN_CARET || op == TOKEN_LESS_LESS || op == TOKEN_GREATER_GREATER) {
        if (!is_type_both_integers(L, R)) {
            String l_s = type_to_str(L);
            String r_s = type_to_str(R);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "bitwise operator requires integers, got '"SV_FMT" and '"SV_FMT"'.", SV_ARG(l_s), SV_ARG(r_s));
            string_destroy(&l_s);
            string_destroy(&r_s);
            return NULL;
        }
        return choose_numeric_binop_result(ts, L, R, rc, expr->loc, op);
    }

    REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "unknown binary operator '%s'.", token_kind_get_value(op));
    return NULL;
}

static inline Type* try_resolve_expr_type(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(ts != NULL && rc != NULL);

    if (expr == NULL) {
        return NULL;
    }

    switch (expr->kind) {
    case AST_NODE_EXPR_LITERAL: return try_resolve_type_for_expr_literal(expr, ts, rc);
    case AST_NODE_EXPR_BRACES:  return try_resolve_expr_type(scope, expr->as.expr_braces.expr, ts, rc);
    case AST_NODE_EXPR_PLACE:   return try_resolve_type_for_expr_place(expr, ts, rc);
    case AST_NODE_EXPR_CALL:    return try_resolve_type_for_expr_call(scope, expr, ts, rc);
    case AST_NODE_EXPR_MEMBER:  return try_resolve_type_for_expr_member(expr, ts, rc);

    case AST_NODE_EXPR_PREFIX_UNARY:
    case AST_NODE_EXPR_POSTFIX_UNARY: return try_resolve_type_for_expr_unary(scope, expr, ts, rc);
    case AST_NODE_EXPR_BINARY:        return try_resolve_type_for_expr_binary(scope, expr, ts, rc);
    default:
        unreachable();
        return NULL;
    }
}

static inline Type* try_to_get_type_from_typeref_custom(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    StringView id = string_get_view(node->as.typeref_custom.value);

    // Fast path
    Type* builtin_type = type_system_get_builtin(ts, id);
    if (builtin_type != NULL) {
        return builtin_type;
    }

    Symbol* sym = scope_lookup_any(scope, id);
    if (sym == NULL) {
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, node->loc, "unknown type '" SV_FMT "'.", SV_ARG(id));
        return NULL;
    }
    if (!symbol_resolve_type(sym, ts, rc)) {
        return NULL;
    }

    return sym->as.typed.type;
}

static inline Type* try_to_get_type_from_typeref_qualified(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    if (node->symbol == NULL) {
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, node->loc, "unresolved qualified type.");
        return type_system_get_unresolved_or_create(ts, STR_LIT("<unresolved>"));
    }

    // Ensure the symbol�s type is resolved (handles aliases)
    if (!symbol_resolve_type(node->symbol, ts, rc)) {
        return type_system_get_unresolved_or_create(ts, node->symbol->name);
    }

    return node->symbol->as.typed.type;
}

static inline Type* try_to_get_type_from_typeref_ptr(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    Type* base = try_to_get_type_from_ast(scope, node->as.typeref_ptr.typeref, ts, rc);
    return type_system_get_pointer_or_create(ts, base);
}

static inline Type* try_to_get_type_from_typeref_arr(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    Type* elem_type = try_to_get_type_from_ast(scope, node->as.typeref_arr.typeref, ts, rc);
    if (node->as.typeref_arr.size_expr != NULL) {
        return type_system_get_array_or_create(ts, elem_type, 0);
    }
    return type_system_get_slice_or_create(ts, elem_type);
}

static inline Type* try_to_get_type_from_typeref_fun(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    const Vector* ast_params = &node->as.typeref_fun.params;

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
                param_types[i] = try_to_get_type_from_ast(scope, ast_param_type, ts, rc);
            }

            /* IMPORTANT: for a *type reference* like (x: T, y: U)-> R we do
               NOT have parameter symbols; those are not declarations.
               So do NOT touch ast_param->symbol here. */
        }
    }

    Type* rt = NULL;
    if (node->as.typeref_fun.typeref == NULL) {
        rt = type_system_get_builtin(ts, STR_LIT("any"));
    }
    else {
        rt = try_to_get_type_from_ast(scope, node->as.typeref_fun.typeref, ts, rc);
    }

    return type_system_get_fun_or_create(ts, param_types, param_count, rt);
}

static inline Type* try_to_get_type_from_stmt_var_item(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    ASTNode* typeref = node->as.stmt_var_item.typeref;
    ASTNode* expr = node->as.stmt_var_item.expr;

    Type* t = NULL;

    if (typeref != NULL) {
        // Explicit annotation wins
        t = try_to_get_type_from_ast(scope, typeref, ts, rc);

        if (expr != NULL) {
            Type* t2 = try_resolve_expr_type(scope, expr, ts, rc);
            if (t != NULL && t2 != NULL && !is_type_compatible(t, t2)) {
                String l_s = type_to_str(t);
                String r_s = type_to_str(t2);
                REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "initializer type '"SV_FMT"' does not match variable type '"SV_FMT"'.", SV_ARG(l_s), SV_ARG(r_s));
                string_destroy(&l_s);
                string_destroy(&r_s);
                return NULL;
            }
        }

        return t;
    }

    if (expr != NULL) {
        // No annotation -> infer from initializer
        return try_resolve_expr_type(scope, expr, ts, rc);
    }

    // No annotation, no initializer -> default to any
    REPORT_NOTE_LOC(rc, DIAG_SEMA_TYPES, node->loc, "variable has no type annotation and no initializer; defaulting to 'any'.");
    return type_system_get_builtin(ts, STR_LIT("any"));
}

static inline Type* try_to_get_type_from_ast(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    switch (node->kind) {
    case AST_NODE_TYPEREF_CUSTOM:    return try_to_get_type_from_typeref_custom(scope, node, ts, rc);
    case AST_NODE_TYPEREF_QUALIFIED: return try_to_get_type_from_typeref_qualified(scope, node, ts, rc);
    case AST_NODE_TYPEREF_PTR:       return try_to_get_type_from_typeref_ptr(scope, node, ts, rc);
    case AST_NODE_TYPEREF_ARR:       return try_to_get_type_from_typeref_arr(scope, node, ts, rc);
    case AST_NODE_TYPEREF_FUN:       return try_to_get_type_from_typeref_fun(scope, node, ts, rc);
    case AST_NODE_STMT_VAR_ITEM:     return try_to_get_type_from_stmt_var_item(scope, node, ts, rc);
    default:
        unreachable();
        return NULL;
    }
}

static inline bool symbol_resolve_typealias_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    ASTNode* ast_type = symbol->ast->as.stmt_typealias_decl.typeref;
    StringView name = string_get_view(symbol->ast->as.stmt_typealias_decl.id->as.id.value);

    Type* target_type = try_to_get_type_from_ast(symbol->scope, ast_type, ts, rc);
    symbol->as.typed.type = type_system_get_alias_or_create(ts, name, target_type);

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
        type = try_to_get_type_from_ast(symbol->scope, typeref, ts, rc);
    }

    symbol->as.typed.type = type;
    return true;
}

static inline bool symbol_resolve_function_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    ASTNode* sign = symbol->ast->as.fun_decl.sign;
    Vector* params = &sign->as.fun_sign.params;

    Type** param_types = NULL;
    u32 param_type_count = params->size;

    if (param_type_count > 0) {
        param_types = malloc(sizeof(Type*) * param_type_count);
        assert(param_types != NULL);

        for (u32 i = 0; i < param_type_count; ++i) {
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
        ret_type = try_to_get_type_from_ast(symbol->scope, ast_ret_type, ts, rc);
    }

    symbol->as.typed.type = type_system_get_fun_or_create(ts, param_types, param_type_count, ret_type);
    return true;
}

static inline bool symbol_resolve_variable_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    symbol->as.typed.type = try_to_get_type_from_stmt_var_item(symbol->scope, symbol->ast, ts, rc);
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
    case SYMBOL_PARAMETER: is_good = true; break; // resolved in function
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