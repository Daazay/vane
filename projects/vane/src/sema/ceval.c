#include "vane/sema/ceval.h"

#include "vane/utils/unicode.h"

static inline void ceval_diag_not_const(const CEvalCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL);
    REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA, ast->loc, "expression is not a constant expression.");
}

static inline void ceval_diag_div_zero(const CEvalCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL);
    REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA, ast->loc, "division by zero in constant expression.");
}

static inline void ceval_diag_overflow(const CEvalCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL);
    REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA, ast->loc, "integer overflow in constant expression.");
}

static inline void ceval_diag_must_be_modifiable(const CEvalCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL);
    REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA, ast->loc, "expression must be modifiable value.");
}

static inline CEvalResult ceval_make_bool(bool b) {
    CEvalResult r = { 0 };

    r.ok        = true;
    r.is_const  = true;
    r.is_signed = false;
    r.u         = b ? 1 : 0;

    return r;
}

static inline CEvalResult ceval_make_u64(u64 v) {
    CEvalResult r = { 0 };

    r.ok        = true;
    r.is_const  = true;
    r.is_signed = false;
    r.u         = v;

    return r;
}

static inline CEvalResult ceval_make_i64(i64 v) {
    CEvalResult r = { 0 };

    r.ok        = true;
    r.is_const  = true;
    r.is_signed = true;
    r.s         = v;

    return r;
}

static inline bool ceval_op_is_signed(bool a_signed, bool b_signed) {
    return a_signed || b_signed;
}

static inline bool ceval_will_div_by_zero(const CEvalResult* rhs) {
    assert(rhs != NULL);
    return rhs->is_const && rhs->u == 0;
}

static inline CEvalResult ceval_apply_unary(TokenKind op, CEvalResult v, const CEvalCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL);

    if (!v.ok) {
        return v;
    }
    if (!v.is_const) {
        return (CEvalResult) { .ok = true, .is_const = false };
    }

    switch (op) {
    case TOKEN_PLUS:        return v;
    case TOKEN_MINUS:       return ceval_make_i64(-(v.is_signed ? v.s : (i64)v.u));
    case TOKEN_TILDE:       return ceval_make_u64(~v.u);
    case TOKEN_EXCLAIM:     return ceval_make_bool(v.u == 0);

    case TOKEN_PLUS_PLUS:
    case TOKEN_MINUS_MINUS:
        ceval_diag_must_be_modifiable(ctx, ast);
        return (CEvalResult) { .ok = false };

    default:
        unreachable();
        return (CEvalResult) { .ok = false };
    };
}

static inline bool ceval_add_u64(u64 a, u64 b, u64* out) {
    *out = a + b;
    return *out >= a;
}

static inline bool ceval_sub_u64(u64 a, u64 b, u64* out) {
    *out = a - b;
    return a >= b;
}

static inline bool ceval_mul_u64(u64 a, u64 b, u64* out) {
    if (a == 0 || b == 0) {
        *out = 0;
        return true;
    }
    *out = a * b;
    return (*out / a) == b;
}

static inline CEvalResult ceval_apply_binary(TokenKind op, CEvalResult lhs, CEvalResult rhs, const CEvalCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL);

    if (!lhs.ok || !rhs.ok) {
        return (CEvalResult) { .ok = false, };
    }
    if (!lhs.is_const || !rhs.is_const) {
        return (CEvalResult) { .ok = true, .is_const = false, };
    }

    switch (op) {
    case TOKEN_PLUS: {
        if (ceval_op_is_signed(lhs.is_signed, rhs.is_signed)) {
            i64 r = lhs.s + rhs.s;
            return ceval_make_i64(r);
        }
        else {
            u64 r = 0;
            if (!ceval_add_u64(lhs.u, rhs.u, &r)) {
                ceval_diag_overflow(ctx, ast);
                return (CEvalResult) { .ok = false, };
            }
            return ceval_make_u64(r);
        }
    } break;

    case TOKEN_MINUS: {
        if (ceval_op_is_signed(lhs.is_signed, rhs.is_signed)) {
            i64 r = lhs.s - rhs.s;
            return ceval_make_i64(r);
        }
        else {
            u64 r = 0;
            if (!ceval_sub_u64(lhs.u, rhs.u, &r)) {
                ceval_diag_overflow(ctx, ast);
                return (CEvalResult) { .ok =  false, };
            }
            return ceval_make_u64(r);
        }
    } break;

    case TOKEN_STAR: {
        if (ceval_op_is_signed(lhs.is_signed, rhs.is_signed)) {
            i64 r = lhs.s * rhs.s;
            return ceval_make_i64(r);
        }
        else {
            u64 r = 0;
            if (!ceval_mul_u64(lhs.u, rhs.u, &r)) {
                ceval_diag_overflow(ctx, ast);
                return (CEvalResult) { .ok = false, };
            }
            return ceval_make_u64(r);
        }
    } break;

    case TOKEN_SLASH: {
        if (ceval_will_div_by_zero(&rhs)) {
            ceval_diag_div_zero(ctx, ast);
            return (CEvalResult) { .ok = false, };
        }
        if (ceval_op_is_signed(lhs.is_signed, rhs.is_signed)) {
            return ceval_make_i64(lhs.s / rhs.s);
        }
        else {
            return ceval_make_u64(lhs.u / rhs.u);
        }
    } break;

    case TOKEN_PERCENT: {
        if (ceval_will_div_by_zero(&rhs)) {
            ceval_diag_div_zero(ctx, ast);
            return (CEvalResult) { .ok = false, };
        }
        if (ceval_op_is_signed(lhs.is_signed, rhs.is_signed)) {
            return ceval_make_i64(lhs.s % rhs.s);
        }
        else {
            return ceval_make_u64(lhs.u % rhs.u);
        }
    } break;

    case TOKEN_LESS_LESS: {
        return ceval_make_u64(lhs.u << (u8)rhs.u);
    } break;

    case TOKEN_GREATER_GREATER: {
        return ceval_make_u64(lhs.u >> (u8)rhs.u);
    } break;

    case TOKEN_AMP: {
        return ceval_make_u64(lhs.u & rhs.u);
    } break;

    case TOKEN_PIPE: {
        return ceval_make_u64(lhs.u | rhs.u);
    } break;

    case TOKEN_CARET: {
        return ceval_make_u64(lhs.u ^ rhs.u);
    } break;

    case TOKEN_LESS: {
        return ceval_make_bool(ceval_op_is_signed(lhs.is_signed, rhs.is_signed) ? (lhs.s < rhs.s) : (lhs.u < rhs.u));
    } break;

    case TOKEN_LESS_EQUAL: {
        return ceval_make_bool(ceval_op_is_signed(lhs.is_signed, rhs.is_signed) ? (lhs.s <= rhs.s) : (lhs.u <= rhs.u));
    } break;

    case TOKEN_GREATER: {
        return ceval_make_bool(ceval_op_is_signed(lhs.is_signed, rhs.is_signed) ? (lhs.s > rhs.s) : (lhs.u > rhs.u));
    } break;

    case TOKEN_GREATER_EQUAL: {
        return ceval_make_bool(ceval_op_is_signed(lhs.is_signed, rhs.is_signed) ? (lhs.s >= rhs.s) : (lhs.u >= rhs.u));
    } break;

    case TOKEN_EQUAL_EQUAL: {
        return ceval_make_bool(lhs.u == rhs.u);
    } break;

    case TOKEN_EXCLAIM_EQUAL: {
        return ceval_make_bool(lhs.u != rhs.u);
    } break;

    case TOKEN_AMP_AMP: {
        return ceval_make_bool((lhs.u != 0) && (rhs.u != 0));
    } break;

    case TOKEN_PIPE_PIPE: {
        return ceval_make_bool((lhs.u != 0) || (rhs.u != 0));
    } break;

    case TOKEN_PLUS_EQUAL:
    case TOKEN_MINUS_EQUAL:
    case TOKEN_STAR_EQUAL:
    case TOKEN_SLASH_EQUAL:
    case TOKEN_PERCENT_EQUAL:
        ceval_diag_must_be_modifiable(ctx, ast);
        break;


    default:
        unreachable();
        break;
    }
    return (CEvalResult) { .ok = true, .is_const = false, };
}

static inline bool ceval_is_digit_for_base(char c, i32 base) {
    if ('0' <= c && c <= '9') {
        return (c - '0') < base;
    }
    if ('a' <= c && c <= 'f') {
        return (10 + (c - 'a')) < base;
    }
    if ('A' <= c && c <= 'F') {
        return (10 + (c - 'A')) < base;
    }
    return false;
}

static inline u32 ceval_digit_val(char c) {
    if ('0' <= c && c <= '9') {
        return (u32)(c - '0');
    }
    if ('a' <= c && c <= 'f') {
        return (u32)(10 + (c - 'a'));
    }
    if ('A' <= c && c <= 'F') {
        return (u32)(10 + (c - 'A'));
    }
    return U32_MAX;
}

static bool ceval_parse_u64_literal(StringView sv, i32 base_hint, u64* out) {
    assert(out != NULL);

    const char* p = (const char*)sv.data;
    const char* e = (const char*)sv.data + sv.len;

    int base = base_hint;
    // Handle optional 0x/0o/0b if present, and base not forced
    if (base == 0 && (e - p) >= 2 && p[0] == '0') {
        if (p[1] == 'x' || p[1] == 'X') {
            base = 16;
            p += 2;
        }
        else if (p[1] == 'b' || p[1] == 'B') {
            base = 2;
            p += 2;
        }
        else if (p[1] == 'o' || p[1] == 'O') {
            base = 8;
            p += 2;
        }
    }
    if (base == 0) {
        base = 10;
    }

    u64 acc = 0;
    for (; p < e; ++p) {
        char c = *p;
        if (c == '_') {
            continue;
        }
        if (!ceval_is_digit_for_base(c, base)) {
            return false;
        }
        u32 d = ceval_digit_val(c);
        // overflow check: acc*base + d <= UINT64_MAX
        if (acc > (U64_MAX - d) / (u64)base) {
            return false;
        }
        acc = acc * (u64)base + (u64)d;
    }
    *out = acc;
    return true;
}

static inline CEvalResult ceval_eval_leaf(const ASTNode* ast, const CEvalCtx* ctx) {
    assert(ast != NULL && ctx != NULL);

    if (ast->kind == AST_NODE_IDENTIFIER) {
        if (ctx->resolve_fn != NULL) {
            u64 u= 0;
            bool signed_ = false;

            if (ctx->resolve_fn(ast, &u, &signed_)) {
                return signed_ ? ceval_make_i64((i64)u) : ceval_make_u64(u);
            }
        }

        // not a constant identifier
        return (CEvalResult) { .ok = true, .is_const = false, };
    }

    if (ast->kind == AST_NODE_EXPR_LITERAL) {
        StringView sv = string_get_view(ast->as.expr_literal.value);
        TokenKind k = ast->as.expr_literal.kind;

        switch (k) {
        case TOKEN_LITERAL_STRING: {
            u64 len = string_view_count_runes(sv);
            return ceval_make_u64(len);
        } break;

        case TOKEN_LITERAL_CHAR: {
            assert(false && "not yet implemented");
        } break;

        case TOKEN_LITERAL_DEC:
        case TOKEN_LITERAL_HEX:
        case TOKEN_LITERAL_OCT:
        case TOKEN_LITERAL_BIN: {
            i32 base =
                (k == TOKEN_LITERAL_DEC) ? 10 :
                (k == TOKEN_LITERAL_HEX) ? 16 :
                (k == TOKEN_LITERAL_OCT) ? 8 :
                (k == TOKEN_LITERAL_BIN) ? 2 : 10;
            u64 v = 0;
            if (!ceval_parse_u64_literal(sv, base, &v)) {
                REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA, ast->loc, "invalid or overflowing integer literal.");
                return (CEvalResult) { .ok = false };
            }
            return ceval_make_u64(v);
        } break;
        case TOKEN_LITERAL_BOOL: {
            if (string_view_eq_sv(sv, STR_LIT("true"))) {
                return ceval_make_bool(true);
            }
            if (string_view_eq_sv(sv, STR_LIT("false"))) {
                return ceval_make_bool(true);
            }
            unreachable();
        } break;

        default:
            unreachable();
            return (CEvalResult) { .ok = false };
        }
    }
    return (CEvalResult) { .ok = true, .is_const = false };
}

static inline const ASTNode* unfold_ast_braces(const ASTNode* ast) {
    assert(ast != NULL);
    while (ast->kind == AST_NODE_EXPR_BRACES) {
        ast = ast->as.expr_braces.expr;
    }
    return ast;
}

static inline CEvalResult ceval_eval_recursive(const ASTNode* ast, const CEvalCtx* ctx) {
    assert(ast != NULL && ctx != NULL);

    ast = unfold_ast_braces(ast);

    if (ast->kind == AST_NODE_EXPR_LITERAL) {
        return ceval_eval_leaf(ast, ctx);
    }

    if (ast->kind == AST_NODE_EXPR_PREFIX_UNARY) {
        CEvalResult v = ceval_eval_recursive(ast->as.expr_prefix_unary.rhs, ctx);
        if (!v.ok) {
            return v;
        }

        return ceval_apply_unary(ast->as.expr_prefix_unary.op, v, ctx, ast);
    }
    if (ast->kind == AST_NODE_EXPR_POSTFIX_UNARY) {
        // post-inc/dec not allowed in constant expressions
        ceval_diag_must_be_modifiable(ctx, ast);
        return (CEvalResult) { .ok = false };
    }
    if (ast->kind == AST_NODE_EXPR_BINARY) {
        // short-circuit logical ops to avoid evaluating RHS unnecessarily
        if (ast->as.expr_binary.op == TOKEN_AMP_AMP) {
            CEvalResult L = ceval_eval_recursive(ast->as.expr_binary.lhs, ctx);
            if (!L.ok) {
                return L;
            }
            if (!L.is_const) {
                return (CEvalResult) { .ok = true, .is_const = false, };
            }
            if (L.u == 0) {
                return ceval_make_bool(false);
            }

            CEvalResult R = ceval_eval_recursive(ast->as.expr_binary.rhs, ctx);
            if (!R.ok) {
                return R;
            }
            if (!R.is_const) {
                return (CEvalResult) { .ok = true, .is_const = false, };
            }
            return ceval_make_bool(R.u != 0);
        }

        if (ast->as.expr_binary.op == TOKEN_PIPE_PIPE) {
            CEvalResult L = ceval_eval_recursive(ast->as.expr_binary.lhs, ctx);
            if (!L.ok) {
                return L;
            }
            if (!L.is_const) {
                return (CEvalResult) { .ok = true, .is_const = false, };
            }
            if (L.u != 0) {
                return ceval_make_bool(true);
            }

            CEvalResult R = ceval_eval_recursive(ast->as.expr_binary.rhs, ctx);
            if (!R.ok) {
                return R;
            }
            if (!R.is_const) {
                return (CEvalResult) { .ok = true, .is_const = false, };
            }
            return ceval_make_bool(R.u != 0);
        }

        CEvalResult L = ceval_eval_recursive(ast->as.expr_binary.lhs, ctx);
        if (!L.ok) {
            return L;
        }
        CEvalResult R = ceval_eval_recursive(ast->as.expr_binary.rhs, ctx);
        if (!R.ok) {
            return R;
        }
        return ceval_apply_binary(ast->as.expr_binary.op, L, R, ctx, ast);
    }

    // Unknown expr kind: not a constant
    return (CEvalResult) { .ok = true, .is_const = false };
}

CEvalResult ceval_eval_int(const ASTNode* expr, const CEvalCtx* ctx) {
    assert(expr != NULL && ctx != NULL);
    CEvalResult r = ceval_eval_recursive(expr, ctx);
    return r;
}

bool ceval_eval_array_size(const ASTNode* size_expr, const CEvalCtx* ctx, u64* out) {
    assert(ctx != NULL && out != NULL);
    assert(size_expr != NULL);

    CEvalResult r = ceval_eval_int(size_expr, ctx);
    if (!r.ok) {
        return false;
    }

    if (!r.is_const) {
        ceval_diag_not_const(ctx, size_expr);
        return false;
    }

    // disallow negative
    if (r.is_signed && r.s < 0) {
        REPORT_ERROR_LOC(ctx->rc, DIAG_SEMA, size_expr->loc, "array size must be non-negative.");
        return false;
    }

    *out = r.u;
    return true;
}

CEvalResult ceval_eval_strlen(const ASTNode* expr, const CEvalCtx* ctx) {
    assert(ctx != NULL);
    assert(expr != NULL && expr->kind == AST_NODE_EXPR_LITERAL && expr->as.expr_literal.kind == TOKEN_LITERAL_STRING);

    StringView sv = string_get_view(expr->as.expr_literal.value);

    //u64 len = string_view_count_runes(string_get_view(expr->as.expr_literal.value));
    //return ceval_make_u64(len);
    return ceval_make_u64(sv.len);
}