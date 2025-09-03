#pragma once

#include <stdarg.h>

#include "vane/utils/defines.h"
#include "vane/utils/unicode.h"
#include "vane/utils/string_view.h"

typedef struct String String;

#define STRING_EMPTY (String) { .data = NULL, .cap = 0, .len = 0, }
#define STRING_DEFAULT_CAPACITY 16
#define STRING_CAPACITY_MULT    1.5

struct String {
    u8* data;
    u64 len;
    u64 cap;
};

// -- creation --

String string_create(u64 init_cap);

String string_from_cstr(const char* cstr);

String string_from_bytes(const u8* data, u64 len);

String string_from_fmt(const char* fmt, ...);

String string_from_fmt_va(const char* fmt, va_list va);

String string_from_sv(StringView sv);

String string_clone(String s);

// -- destruction --

void string_clear(String* s);

void string_destroy(String* s);

// -- appendings --

void string_append_c(String* s, char c);

void string_append_rune(String* s, Rune r);

void string_append_cstr(String* s, const char* cstr);

void string_append_bytes(String* s, const u8* data, u64 len);

void string_append_fmt(String* s, const char* fmt, ...);

void string_append_fmt_va(String* s, const char* fmt, va_list va);

void string_append_sv(String* s, StringView sv);

void string_append_str(String* s1, String s2);

void string_append_left_c(String* s, char c);

void string_append_left_rune(String* s, Rune r);

void string_append_left_cstr(String* s, const char* cstr);

void string_append_left_bytes(String* s, const u8* data, u64 len);

void string_append_left_fmt(String* s, const char* fmt, ...);

void string_append_left_fmt_va(String* s, const char* fmt, va_list va);

void string_append_left_sv(String* s, StringView sv);

void string_append_left_str(String* s1, String s2);

// -- utilities

void string_shrink_to_fit(String* s);

StringView string_get_view(String s);

void string_reserve(String* s, u64 additional_len);

void string_resize(String* s, u64 new_len, char fill);

// -- replacement --

bool string_replace_c(String* s, char find, char replace);

bool string_replace_rune(String* s, Rune find, Rune replace);

bool string_replace_cstr(String* s, const char* find, const char* replace);

bool string_replace_bytes(String* s, const u8* find, u64 find_len, const u8* replace, u64 replace_len);

bool string_replace_sv(String* s, StringView find, StringView replace);

bool string_replace_str(String* s, String find, String replace);

// -- comparison --

static inline bool is_string_empty(String s) {
    return is_string_view_empty(string_get_view(s));
}

static inline bool string_eq_cstr(String s, const char* cstr) {
    return string_view_eq_cstr(string_get_view(s), cstr);
}

static inline bool string_eq_bytes(String s, const u8* data, u64 len) {
    return string_view_eq_bytes(string_get_view(s), data, len);
}

static inline bool string_eq_sv(String s, StringView sv) {
    return string_view_eq_sv(string_get_view(s), sv);
}

static inline bool string_eq_str(String s1, String s2) {
    return string_view_eq_sv(string_get_view(s1), string_get_view(s2));
}

// -- prefixes/suffixes --

static inline bool string_has_prefix_cstr(String s, const char* prefix) {
    return string_view_has_prefix_cstr(string_get_view(s), prefix);
}

static inline bool string_has_prefix_bytes(String s, const u8* prefix, u64 prefix_len) {
    return string_view_has_prefix_bytes(string_get_view(s), prefix, prefix_len);
}

static inline bool string_has_prefix_sv(String s, StringView prefix) {
    return string_view_has_prefix_sv(string_get_view(s), prefix);
}

static inline bool string_has_prefix_str(String s, String prefix) {
    return string_view_has_prefix_sv(string_get_view(s), string_get_view(prefix));
}

static inline bool string_has_suffix_cstr(String s, const char* suffix) {
    return string_view_has_suffix_cstr(string_get_view(s), suffix);
}

static inline bool string_has_suffix_bytes(String s, const u8* suffix, u64 suffix_len) {
    return string_view_has_suffix_bytes(string_get_view(s), suffix, suffix_len);
}

static inline bool string_has_suffix_sv(String s, StringView suffix) {
    return string_view_has_suffix_sv(string_get_view(s), suffix);
}

static inline bool string_has_suffix_str(String s, String suffix) {
    return string_view_has_suffix_sv(string_get_view(s), string_get_view(suffix));
}

// -- subview/substr --

static inline StringView string_subview(String s, u64 offset, u64 len) {
    return string_view_subview(string_get_view(s), offset, len);
}

static inline String string_substr(String s, u64 offset, u64 len) {
    return string_from_sv(string_view_subview(string_get_view(s), offset, len));
}

// -- unicode --

static inline u64 string_count_runes(String s) {
    return string_view_count_runes(string_get_view(s));
}

static inline Rune string_rune_at_byte(String s, u64 byte_idx, u32* read_len) {
    return string_view_rune_at_byte(string_get_view(s), byte_idx, read_len);
}

static inline Rune string_rune_at(String s, u64 idx, u64* byte_idx, u32* read_len) {
    return string_view_rune_at(string_get_view(s), idx, byte_idx, read_len);
}

// -- utilities --

static inline u32 string_get_hash(String s) {
    return string_view_get_hash(string_get_view(s));
}

// -- collection utilities --

static inline bool __string_eq_str(const String* s1, const String* s2) {
    assert(s1 != NULL && s2 != NULL);
    return string_eq_str(*s1, *s2);
}

static inline u32 __string_get_hash(const String* s) {
    assert(s != NULL);
    return string_get_hash(*s);
}

// -- search --

static inline bool string_contains_c(String s, char c) {
    return string_view_contains_c(string_get_view(s), c);
}

static inline bool string_contains_rune(String s, Rune r) {
    return string_view_contains_rune(string_get_view(s), r);
}

static inline bool string_contains_cstr(String s, const char* cstr) {
    return string_view_contains_cstr(string_get_view(s), cstr);
}

static inline bool string_contains_bytes(String s, const u8* data, u64 len) {
    return string_view_contains_bytes(string_get_view(s), data, len);
}

static inline bool string_contains_sv(String s, StringView sv) {
    return string_view_contains_sv(string_get_view(s), sv);
}

static inline bool string_contains_str(String s1, String s2) {
    return string_view_contains_sv(string_get_view(s1), string_get_view(s2));
}

static inline u64 string_find_c(String s, char c) {
    return string_view_find_c(string_get_view(s), c);
}

static inline u64 string_find_rune(String s, Rune r) {
    return string_view_find_rune(string_get_view(s), r);
}

static inline u64 string_find_cstr(String s, const char* cstr) {
    return string_view_find_cstr(string_get_view(s), cstr);
}

static inline u64 string_find_bytes(String s, const u8* data, u64 len) {
    return string_view_find_bytes(string_get_view(s), data, len);
}

static inline u64 string_find_sv(String s, StringView sv) {
    return string_view_find_sv(string_get_view(s), sv);
}

static inline u64 string_find_str(String s1, String s2) {
    return string_view_find_sv(string_get_view(s1), string_get_view(s2));
}

static inline u64 string_find_last_c(String s, char c) {
    return string_view_find_last_c(string_get_view(s), c);
}

static inline u64 string_find_last_rune(String s, Rune r) {
    return string_view_find_last_rune(string_get_view(s), r);
}

static inline u64 string_find_last_cstr(String s, const char* cstr) {
    return string_view_find_last_cstr(string_get_view(s), cstr);
}

static inline u64 string_find_last_bytes(String s, const u8* data, u64 len) {
    return string_view_find_last_bytes(string_get_view(s), data, len);
}

static inline u64 string_find_last_sv(String s, StringView sv) {
    return string_view_find_last_sv(string_get_view(s), sv);
}

static inline u64 string_find_last_str(String s1, String s2) {
    return string_view_find_last_sv(string_get_view(s1), string_get_view(s2));
}