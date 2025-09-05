#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string_view.h"

#include "vane/scanner/token_kind.h"
#include "vane/scanner/token_loc.h"

typedef struct Token Token;
typedef enum TokenFlags TokenFlags;

enum TokenFlags {
    TOKEN_FLAG_NONE = 0,

    TOKEN_FLAG_FIRST_IN_LINE = 1 << 0,

    // Whitespace flags
    TOKEN_FLAG_HAS_LEADING_WS  = 1 << 1,
    TOKEN_FLAG_HAS_TRAILING_WS = 1 << 2,
    TOKEN_FLAG_HAS_AROUND_WS   = TOKEN_FLAG_HAS_TRAILING_WS | TOKEN_FLAG_HAS_LEADING_WS,

    // Line break flags
    TOKEN_FLAG_HAS_LEADING_LBR  = 1 << 3,
    TOKEN_FLAG_HAS_TRAILING_LBR = 1 << 4,
    TOKEN_FLAG_HAS_AROUND_LBR   = TOKEN_FLAG_HAS_TRAILING_LBR | TOKEN_FLAG_HAS_LEADING_LBR,

    // Combined
    TOKEN_FLAG_HAS_LEADING_WS_OR_LBR  = TOKEN_FLAG_HAS_LEADING_WS | TOKEN_FLAG_HAS_LEADING_LBR,
    TOKEN_FLAG_HAS_TRAILING_WS_OR_LBR = TOKEN_FLAG_HAS_TRAILING_WS | TOKEN_FLAG_HAS_TRAILING_LBR,
    TOKEN_FLAG_HAS_AROUND_WS_OR_LBR   = TOKEN_FLAG_HAS_AROUND_WS  | TOKEN_FLAG_HAS_AROUND_LBR,
};

struct Token {
    TokenKind kind;
    TokenFlags flags;
    StringView value;
    TokenLoc loc;
};

Token token_create(TokenKind kind, TokenFlags flags, StringView value, TokenLoc loc);