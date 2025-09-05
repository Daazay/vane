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