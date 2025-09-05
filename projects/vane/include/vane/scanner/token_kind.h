#pragma once

#include "vane/utils/defines.h"

typedef enum TokenKind TokenKind;

enum TokenKind {
#define TOKEN(KIND, NAME, VALUE) TOKEN_##KIND,
#include "vane/scanner/token_kind.def"
};

const char* token_kind_get_name(TokenKind kind);

const char* token_kind_get_value(TokenKind kind);

bool is_token_kind_misc(TokenKind kind);

bool is_token_kind_punc(TokenKind kind);

bool is_token_kind_keyword(TokenKind kind);

bool is_token_kind_literal(TokenKind kind);