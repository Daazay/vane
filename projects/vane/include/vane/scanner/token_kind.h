#pragma once

#include "vane/utils/defines.h"

typedef enum TokenKind TokenKind;
typedef enum OpPrecedence OpPrecedence;
typedef enum OpAssociativity OpAssociativity;

enum TokenKind {
#define TOKEN(KIND, NAME, VALUE) TOKEN_##KIND,
#include "vane/scanner/token_kind.def"
};

enum OpPrecedence {
    OP_PREC_NONE = 0,
    OP_PREC_ASSIGNMENT,
    OP_PREC_LOGICAL_OR,
    OP_PREC_LOGICAL_AND,
    OP_PREC_BIN_OR,
    OP_PREC_BIN_XOR,
    OP_PREC_BIN_AND,
    OP_PREC_EQUALITY,
    OP_PREC_RELATIONAL,
    OP_PREC_SHIFT,
    OP_PREC_ADDITIVE,
    OP_PREC_MULTIPLICATIVE,
    OP_PREC_CALL,
    OP_PREC_INDEX,
    OP_PREC_MEMBER,
    OP_PREC_UNARY,
};

enum OpAssociativity {
    OP_ASSOC_LEFT = 0,
    OP_ASSOC_RIGHT,
};

const char* token_kind_get_name(TokenKind kind);

const char* token_kind_get_value(TokenKind kind);

bool is_token_kind_misc(TokenKind kind);

bool is_token_kind_punc(TokenKind kind);

bool is_token_kind_op(TokenKind kind);

bool is_token_kind_binop(TokenKind kind);

bool is_token_kind_prefix_unop(TokenKind kind);

bool is_token_kind_postfix_unop(TokenKind kind);

bool is_token_kind_unop(TokenKind kind);

bool is_token_kind_keyword(TokenKind kind);

bool is_token_kind_literal(TokenKind kind);

OpPrecedence get_token_kind_precedence(TokenKind kind);

OpAssociativity get_op_associativity(OpPrecedence prec);