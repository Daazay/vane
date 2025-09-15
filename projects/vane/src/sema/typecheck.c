#include "vane/sema/typecheck.h"

static inline bool is_both_types_bool(const Type* l, const Type* r) {
    return is_type_bool(l) && is_type_bool(r);
}

static inline bool is_both_types_ints(const Type* l, const Type* r) {
    return is_type_integer(l) && is_type_integer(r);
}

static inline Type* typecheck_choose_numeric_binop(TypeSystem* ts, const Type* l, const Type* r, ReportCollector* rc, TokenLoc loc, TokenKind op) {
    assert(ts != NULL && l != NULL && r != NULL && rc != NULL);

    l = type_unwrap(l);
    r = type_unwrap(r);

    if (l == NULL || r == NULL) {
        return NULL;
    }

    if (is_type_any(l) || is_type_any(r)) {
        return type_system_get_builtin(ts, STR_LIT("any"));
    }

    const bool l_uns = is_type_unsized_integer(l);
    const bool r_uns = is_type_unsized_integer(r);

    TypeBuiltinKind lk = TYPE_BUILTIN_UNKNOWN, rk = TYPE_BUILTIN_UNKNOWN;
    const bool l_sz = is_type_sized_integer(l, &lk);
    const bool r_sz = is_type_sized_integer(r, &rk);

    if ((l_uns || l_sz) && (r_uns || r_sz)) {
        // sized + unsized -> sized
        if (l_sz && r_uns) {
            return (Type*)l;
        }
        if (r_sz && l_uns) {
            return (Type*)r;
        }

        // both unsized -> unsized int
        if (l_uns && r_uns ) {
            return type_system_get_builtin(ts, STR_LIT("unsized int"));
        }

        // both sized -> wider; if equal width, prefer unsigned
        if (l_sz && r_sz) {
            const u32 lb = type_builtin_kind_get_size(lk);
            const u32 rb = type_builtin_kind_get_size(rk);

            if (lb > rb) {
                return (Type*)l;
            }
            if (rb > lb) {
                return (Type*)r;
            }

            if (is_type_builtin_kind_unsigned(lk)) {
                return (Type*)l;
            }
            if (is_type_builtin_kind_unsigned(rk)) {
                return (Type*)r;
            }
            // both signed, same width
            return (Type*)l;
        }
    }

    String ls = type_to_str(l);
    String rs = type_to_str(r);

    REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, loc, "operator '%s' is not defined for '" SV_FMT "' and '" SV_FMT "'.",
        token_kind_get_value(op), SV_ARG(ls), SV_ARG(rs)
    );

    string_destroy(&ls);
    string_destroy(&rs);
    return NULL;
}

static inline Type* typecheck_resolve_literal(ASTNode* expr, TypeSystem* ts) {
    assert(expr != NULL && expr->kind == AST_NODE_EXPR_LITERAL && ts != NULL);

    switch (expr->as.expr_literal.kind) {
    case TOKEN_LITERAL_BOOL: return type_system_get_builtin(ts, STR_LIT("bool"));
    case TOKEN_LITERAL_CHAR: return type_system_get_builtin(ts, STR_LIT("u8"));
    case TOKEN_LITERAL_DEC:
    case TOKEN_LITERAL_OCT:
    case TOKEN_LITERAL_HEX:
    case TOKEN_LITERAL_BIN:
        return type_system_get_builtin(ts, STR_LIT("unsized int"));
    case TOKEN_LITERAL_STRING: {
        // Strings as []u8 by default; you already convert literals to [len]u8
        // where needed (e.g., typed contexts) in variable/type resolution.
        Type* u8t = type_system_get_builtin(ts, STR_LIT("u8"));
        return type_system_get_slice_or_create(ts, u8t);
    }
    default:
        unreachable();
        return NULL;
    }
}

static inline Type* typecheck_resolve_expr_type_recursive(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc);

static inline Type* typecheck_resolve_unary(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && expr != NULL && ts != NULL && rc != NULL);

    const bool is_prefix = (expr->kind == AST_NODE_EXPR_PREFIX_UNARY);
    TokenKind op     = is_prefix ? expr->as.expr_prefix_unary.op  : expr->as.expr_postfix_unary.op;
    ASTNode* operand = is_prefix ? expr->as.expr_prefix_unary.rhs : expr->as.expr_postfix_unary.lhs;

    Type* ot = typecheck_resolve_expr_type_recursive(scope, operand, ts, rc);
    if (ot == NULL) {
        return NULL;
    }
    const Type* u = type_unwrap(ot);

    // ++ / -- : require integer; result is operand type
    if (op == TOKEN_PLUS_PLUS || op == TOKEN_MINUS_MINUS) {
        if (!is_type_integer(u) && !is_type_any(u)) {
            String s = type_to_str(u);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "operator requires integer operand, got '" SV_FMT "'.", SV_ARG(s));
            string_destroy(&s);
            return NULL;
        }
        return ot;
    }

    // logical not '!'
    if (op == TOKEN_EXCLAIM) {
        if (!is_type_bool(u) && !is_type_any(u)) {
            String s = type_to_str(u);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "logical '!' requires 'bool', got '" SV_FMT "'.", SV_ARG(s));
            string_destroy(&s);
            return NULL;
        }
        return type_system_get_builtin(ts, STR_LIT("bool"));
    }

    // unary + / -
    if (op == TOKEN_PLUS || op == TOKEN_MINUS) {
        if (!is_type_integer(u) && !is_type_any(u)) {
            String s = type_to_str(u);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "unary '%s' requires integer, got '" SV_FMT "'.", token_kind_get_value(op), SV_ARG(s));
            string_destroy(&s);
            return NULL;
        }
        return ot;
    }

    // bitwise not '~'
    if (op == TOKEN_TILDE) {
        if (!is_type_integer(u) && !is_type_any(u)) {
            String s = type_to_str(u);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "bitwise '~' requires integer, got '" SV_FMT "'.", SV_ARG(s));
            string_destroy(&s);
            return NULL;
        }
        return ot;
    }

    REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "unknown unary operator '%s'.", token_kind_get_value(op));
    return NULL;
}

static Type* typecheck_resolve_binary(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && expr != NULL && ts != NULL && rc != NULL);

    TokenKind op = expr->as.expr_binary.op;
    ASTNode* L   = expr->as.expr_binary.lhs;
    ASTNode* R   = expr->as.expr_binary.rhs;

    Type* lt = typecheck_resolve_expr_type_recursive(scope, L, ts, rc);
    Type* rt = typecheck_resolve_expr_type_recursive(scope, R, ts, rc);
    if (lt == NULL || rt == NULL) {
        return NULL;
    }

    const Type* l = type_unwrap(lt);
    const Type* r = type_unwrap(rt);

    // short-circuit: ANY dominates result for non-boolean-y operators.
    if (is_type_any(l) || is_type_any(r)) {
        if (op == TOKEN_AMP_AMP     || op == TOKEN_PIPE_PIPE     ||
            op == TOKEN_EQUAL_EQUAL || op == TOKEN_EXCLAIM_EQUAL ||
            op == TOKEN_LESS        || op == TOKEN_LESS_EQUAL    ||
            op == TOKEN_GREATER     || op == TOKEN_GREATER_EQUAL) {
            return type_system_get_builtin(ts, STR_LIT("bool"));
        }
        return type_system_get_builtin(ts, STR_LIT("any"));
    }

    // logical
    if (op == TOKEN_AMP_AMP || op == TOKEN_PIPE_PIPE) {
        if (!is_both_types_bool(l, r)) {
            String ls = type_to_str(l), rs = type_to_str(r);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "logical operator requires 'bool' and 'bool', got '" SV_FMT "' and '" SV_FMT "'.", SV_ARG(ls), SV_ARG(rs));
            string_destroy(&ls);
            string_destroy(&rs);
            return NULL;
        }
        return type_system_get_builtin(ts, STR_LIT("bool"));
    }

    // equality
    if (op == TOKEN_EQUAL_EQUAL || op == TOKEN_EXCLAIM_EQUAL) {
        const bool ints = is_both_types_ints(l, r);
        const bool ok = ints || type_eq_type(l, r) || is_type_any(l) || is_type_any(r);
        if (!ok) {
            String ls = type_to_str(l), rs = type_to_str(r);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "comparison requires compatible types, got '" SV_FMT "' and '" SV_FMT "'.", SV_ARG(ls), SV_ARG(rs));
            string_destroy(&ls);
            string_destroy(&rs);
            return NULL;
        }
        return type_system_get_builtin(ts, STR_LIT("bool"));
    }

    // relational
    if (op == TOKEN_LESS || op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL) {
        if (!is_both_types_ints(l, r)) {
            String ls = type_to_str(l), rs = type_to_str(r);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "relational operator requires integers, got '" SV_FMT "' and '" SV_FMT "'.", SV_ARG(ls), SV_ARG(rs));
            string_destroy(&ls);
            string_destroy(&rs);
            return NULL;
        }
        return type_system_get_builtin(ts, STR_LIT("bool"));
    }

    // arithmetic
    if (op == TOKEN_PLUS || op == TOKEN_MINUS || op == TOKEN_STAR ||
        op == TOKEN_SLASH || op == TOKEN_PERCENT) {
        return typecheck_choose_numeric_binop(ts, l, r, rc, expr->loc, op);
    }

    // bitwise
    if (op == TOKEN_AMP || op == TOKEN_PIPE || op == TOKEN_CARET ||
        op == TOKEN_LESS_LESS || op == TOKEN_GREATER_GREATER) {
        if (!is_both_types_ints(l, r)) {
            String ls = type_to_str(l), rs = type_to_str(r);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "bitwise operator requires integers, got '" SV_FMT "' and '" SV_FMT "'.", SV_ARG(ls), SV_ARG(rs));
            string_destroy(&ls);
            string_destroy(&rs);
            return NULL;
        }
        return typecheck_choose_numeric_binop(ts, l, r, rc, expr->loc, op);
    }

    // assignment-forms are expressions in your tree. We return LHS type.
    if (op == TOKEN_EQUAL || op == TOKEN_PLUS_EQUAL || op == TOKEN_MINUS_EQUAL ||
        op == TOKEN_STAR_EQUAL || op == TOKEN_SLASH_EQUAL || op == TOKEN_PERCENT_EQUAL) {
        return lt;
    }

    REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "unknown binary operator '%s'.", token_kind_get_value(op));
    return NULL;
}

static inline Type* typecheck_resolve_expr_type_recursive(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && rc != NULL);

    if (expr == NULL) {
        return NULL;
    }

    switch (expr->kind) {
    case AST_NODE_EXPR_LITERAL:
        return typecheck_resolve_literal(expr, ts);

    case AST_NODE_EXPR_BRACES:
        return typecheck_resolve_expr_type_recursive(scope, expr->as.expr_braces.expr, ts, rc);

    case AST_NODE_EXPR_PLACE: {
        assert(expr->symbol != NULL && "place must be bound in bind pass");
        Symbol* symbol = expr->symbol;
        if (!symbol_resolve_type(symbol, ts, rc)) {
            return NULL;
        }
        return symbol->as.typed.type;
    }

    case AST_NODE_EXPR_MEMBER: {
        assert(expr->symbol && "member should be bound during bind pass");
        Symbol* sym = expr->symbol;
        if (!symbol_resolve_type(sym, ts, rc)) return NULL;
        return sym->as.typed.type;
    }

    case AST_NODE_EXPR_CALL: {
        ASTNode* callee = expr->as.expr_call.callee;
        Type* ct = typecheck_resolve_expr_type_recursive(scope, callee, ts, rc);
        if (ct == NULL) {
            return NULL;
        }
        const Type* u = type_unwrap(ct);
        if (u->kind != TYPE_FUNCTION) {
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "call target is not a function.");
            return NULL;
        }
        return (Type*)u->as.fun.ret;
    }

    case AST_NODE_EXPR_INDEX: {
        ASTNode* base_expr = expr->as.expr_index.callee;
        ASTNode* idx_expr = expr->as.expr_index.index;
        assert(base_expr != NULL && idx_expr != NULL);

        // resolve base type
        Type* bt = typecheck_resolve_expr_type_recursive(scope, base_expr, ts, rc);
        if (bt == NULL) {
            return NULL;
        }

        const Type* b = type_unwrap(bt);

        // resolve index type and validate it's an integer-ish
        Type* it = typecheck_resolve_expr_type_recursive(scope, idx_expr, ts, rc);
        if (it == NULL) {
            return NULL;
        }

        const Type* iu = type_unwrap(it);
        if (!is_type_integer(iu) && !is_type_any(iu)) {
            String is = type_to_str(iu);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, idx_expr->loc, "index must be an integer, got '" SV_FMT "'.", SV_ARG(is));
            string_destroy(&is);
            return NULL;
        }

        // figure out the element type depending on container kind
        switch (b->kind) {
        case TYPE_ARRAY: return (Type*)b->as.array.elem;
        case TYPE_SLICE: return (Type*)b->as.slice.elem;
        case TYPE_POINTER:
            // allow ptr[index] as sugar for *(ptr + index)
            return (Type*)b->as.pointer.base;

        default: {
            String bs = type_to_str(b);
            REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, base_expr->loc, "type '" SV_FMT "' is not indexable.", SV_ARG(bs));
            string_destroy(&bs);
            return NULL;
        }
        }
    }

    case AST_NODE_EXPR_PREFIX_UNARY:
    case AST_NODE_EXPR_POSTFIX_UNARY:
        return typecheck_resolve_unary(scope, expr, ts, rc);

    case AST_NODE_EXPR_BINARY:
        return typecheck_resolve_binary(scope, expr, ts, rc);

    default:
        REPORT_ERROR_LOC(rc, DIAG_SEMA_TYPES, expr->loc, "cannot resolve type of expression.");
        return NULL;
    }
}

Type* typecheck_resolve_expr_type(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && rc != NULL);

    if (expr == NULL) {
        return NULL;
    }

    return typecheck_resolve_expr_type_recursive(scope, expr, ts, rc);
}