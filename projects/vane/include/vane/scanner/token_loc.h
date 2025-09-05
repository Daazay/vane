#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string_view.h"

typedef struct TokenLoc TokenLoc;
typedef struct TokenPos TokenPos;

struct TokenPos {
    i32 line;
    i32 column;
};

struct TokenLoc {
    StringView path;
    TokenPos begin;
    TokenPos end;
};

TokenPos token_pos_create(i32 line, i32 column);

TokenLoc token_loc_create(StringView path, TokenPos begin, TokenPos end);