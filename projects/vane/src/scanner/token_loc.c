#include "vane/scanner/token_loc.h"

TokenPos token_pos_create(i32 line, i32 column) {
    return (TokenPos) {
        .line = line,
        .column = column,
    };
}

TokenLoc token_loc_create(StringView path, TokenPos begin, TokenPos end) {
    return (TokenLoc) {
        .path = path,
        .begin = begin,
        .end = end,
    };
}