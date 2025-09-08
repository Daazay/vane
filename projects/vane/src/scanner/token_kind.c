#include "vane/scanner/token_kind.h"

const char* token_kind_get_name(TokenKind kind) {
    switch (kind) {
#define TOKEN(KIND, NAME, VALUE) case TOKEN_##KIND: return NAME;
#include "vane/scanner/token_kind.def"
    default:
        unreachable();
        return NULL;
    }
}

const char* token_kind_get_value(TokenKind kind) {
    switch (kind) {
#define TOKEN(KIND, NAME, VALUE) case TOKEN_##KIND: return VALUE;
#include "vane/scanner/token_kind.def"
    default:
        unreachable();
        return NULL;
    }
}

bool is_token_kind_misc(TokenKind kind) {
    switch (kind) {
#define TOKEN_MISC(KIND, NAME) case TOKEN_##KIND: return true;
#include "vane/scanner/token_kind.def"
    default: return false;
    }
}

bool is_token_kind_punc(TokenKind kind) {
    switch (kind) {
#define TOKEN_PUNC(KIND, NAME, VALUE) case TOKEN_##KIND: return true;
#include "vane/scanner/token_kind.def"
    default: return false;
    }
}

bool is_token_kind_op(TokenKind kind) {
    switch (kind) {
#define TOKEN_OP(KIND, NAME, VALUE, PREC) case TOKEN_##KIND: return true;
#include "vane/scanner/token_kind.def"
    default:
        return false;
    }
}

bool is_token_kind_binop(TokenKind kind) {
    switch (kind) {
#define TOKEN_BINOP(KIND, NAME, VALUE, PREC) case TOKEN_##KIND: return true;
#include "vane/scanner/token_kind.def"
    default:
        return false;
    }
}

bool is_token_kind_prefix_unop(TokenKind kind) {
    switch (kind) {
    case TOKEN_PLUS:  return true;
    case TOKEN_MINUS: return true;
    case TOKEN_CARET: return true;
    case TOKEN_AMP:   return true;
#define TOKEN_UNOP(KIND, NAME, VALUE, PREC) case TOKEN_##KIND: return true;
#include "vane/scanner/token_kind.def"
    default:
        return false;
    }
}

bool is_token_kind_postfix_unop(TokenKind kind) {
    switch (kind) {
    case TOKEN_PLUS_PLUS:   return true;
    case TOKEN_MINUS_MINUS: return true;
    case TOKEN_CARET:       return true;
    default: return false;
    }
}

bool is_token_kind_unop(TokenKind kind) {
    return is_token_kind_prefix_unop(kind) || is_token_kind_postfix_unop(kind);
}

bool is_token_kind_keyword(TokenKind kind) {
    switch (kind) {
#define TOKEN_KEYWORD(KIND, NAME) case TOKEN_KEYWORD_##KIND: return true;
#include "vane/scanner/token_kind.def"
    default: return false;
    }
}

bool is_token_kind_literal(TokenKind kind) {
    switch (kind) {
#define TOKEN_LITERAL(KIND, NAME) case TOKEN_LITERAL_##KIND: return true;
#include "vane/scanner/token_kind.def"
    default: return false;
    }
}

OpPrecedence get_token_kind_precedence(TokenKind kind) {
    switch (kind) {
#define TOKEN_OP(KIND, NAME, VAUE, PREC) case TOKEN_##KIND: return OP_PREC_##PREC;
#include "vane/scanner/token_kind.def"
    default: return OP_PREC_NONE;
    }
}

OpAssociativity get_op_associativity(OpPrecedence prec) {
    switch (prec) {
    case OP_PREC_ASSIGNMENT: return OP_ASSOC_RIGHT;
    case OP_PREC_UNARY:      return OP_ASSOC_RIGHT;
    default:                 return OP_ASSOC_LEFT;
    }
}