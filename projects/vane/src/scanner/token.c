#include "vane/scanner/token.h"

Token token_create(TokenKind kind, TokenFlags flags, StringView value, TokenLoc loc) {
    return (Token) {
        .kind = kind,
        .flags = flags,
        .value = value,
        .loc = loc,
    };
}