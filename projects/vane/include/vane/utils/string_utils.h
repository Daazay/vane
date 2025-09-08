#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/unicode.h"

// -- ASCII --

static inline bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static inline bool is_digit(char c) {
    return ('0' <= c && c <= '9');
}

static inline bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static inline bool is_alnum_(char c) {
    return is_alpha(c) || is_digit(c) || (c == '_');
}

static inline bool is_xdigit(char c) {
    return is_digit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

static inline bool is_odigit(char c) {
    return ('0' <= c && c <= '7');
}

static inline bool is_bdigit(char c) {
    return (c == '0') || (c == '1');
}

static inline bool is_hspace(char c) {
    return (c == ' ') || (c == '\t');
}

static inline bool is_vspace(char c) {
    return (c == '\n') || (c == '\r');
}

static inline bool is_space(char c) {
    return is_hspace(c) || is_vspace(c);
}

static inline bool is_punc(char c) {
    switch (c) {
    case '(': case ')':
    case '[': case ']':
    case '.': case ',':
    case ':': case ';':
    case '+': case '-':
    case '*': case '/':
    case '%': case '^':
    case '&': case '|':
    case '=': case '~': case '!':
    case '>': case '<':
        return true;
    default:
        return false;
    }
}

// -- Rune (Unicode) --

static inline bool is_alpha_rune(Rune r) {
    // ASCII letters
    if (('a' <= r && r <= 'z') || ('A' <= r && r <= 'Z')) {
        return true;
    }

    return false;
}

static inline bool is_digit_rune(Rune r) {
    // ASCII digits
    if ('0' <= r && r <= '9') {
        return true;
    }

    return false;
}

static inline bool is_alnum_rune(Rune r) {
    return is_alpha_rune(r) || is_digit_rune(r);
}

static inline bool is_alnum_rune_(Rune r) {
    return is_alnum_rune(r) || r == '_';
}

static inline bool is_xdigit_rune(Rune r) {
    return is_digit_rune(r) ||
        ('a' <= r && r <= 'f') ||
        ('A' <= r && r <= 'F');
}

static inline bool is_odigit_rune(Rune r) {
    return ('0' <= r && r <= '7');
}

static inline bool is_bdigit_rune(Rune r) {
    return r == '0' || r == '1';
}

static inline bool is_hspace_rune(Rune r) {
    return r == ' ' || r == '\t';
}

static inline bool is_vspace_rune(Rune r) {
    return r == '\n' || r == '\r';
}

static inline bool is_space_rune(Rune r) {
    return is_hspace_rune(r) || is_vspace_rune(r);
}

static inline bool is_punc_rune(Rune r) {
    switch (r) {
    case '(': case ')':
    case '[': case ']':
    case '.': case ',':
    case ':': case ';':
    case '+': case '-':
    case '*': case '/':
    case '%': case '^':
    case '&': case '|':
    case '=': case '~': case '!':
    case '>': case '<':
        return true;
    default:
        return false;
    }
}