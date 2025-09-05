#pragma once

#include "vane/utils/defines.h"

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
    return (c == '(') || (c == ')') ||
           (c == '[') || (c == ']') ||
           (c == '.') || (c == ',') ||
           (c == ':') || (c == ';') ||
           (c == '+') || (c == '-') ||
           (c == '*') || (c == '/') ||
           (c == '%') || (c == '^') ||
           (c == '&') || (c == '|') ||
           (c == '=') || (c == '~') || (c == '!') ||
           (c == '>') || (c == '<');
}