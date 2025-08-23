#pragma once

#include <stdarg.h>

#include "vane/utils/defines.h"
#include "vane/utils/string_view.h"
#include "vane/utils/unicode.h"

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

String string_create(u64 cap);

String string_from_cstr(const char* cstr);

String string_from_bytes(const u8* bytes, u64 len);

String string_from_sv(const StringView* sv);

String string_from_fmt(const char* format, ...);

String string_from_fmt_va(const char* format, va_list va);

String string_clone(const String* s);

// -- destruction --

void string_clear(String* s);

void string_destroy(String* s);

//

String string_concat_cstr(const char* cstr1, const char* cstr2);

String string_concat_bytes(const u8* data1, u64 len1, const u8* data2, u64 len2);

String string_concat_sv(const StringView* sv1, const StringView* sv2);

String string_concat_str(const String* s1, const String* s2);

//

void string_append_c(String* s, char cstr);

void string_append_rune(String* s, Rune r);

void string_append_cstr(String* s, const char* cstr);

void string_append_bytes(String* s, const u8* bytes, u64 len);

void string_append_sv(String* s, const StringView* sv);

void string_append_str(String* s, const String* s2);

void string_append_left_c(String* s, char c);

void string_append_left_rune(String* s, Rune r);

void string_append_left_cstr(String* s, const char* cstr);

void string_append_left_bytes(String* s, const u8* bytes, u64 len);

void string_append_left_sv(String* s, const StringView* sv);

void string_append_left_str(String* s, const String* s2);

// -- utilities --

void string_shrink_to_fit(String* s);

StringView string_get_view(const String* s);

void string_reserve(String* s, u64 additional);

void string_resize(String* s, u64 new_len, char fill);

// -- replacement --

bool string_replace_c(String* s, char find, char replace);

bool string_replace_rune(String* s, Rune find, Rune replace);

bool string_replace_cstr(String* s, const char* find, const char* replace);

bool string_replace_bytes(String* s, const u8* find, u64 find_len, const u8* replace, u64 replace_len);

bool string_replace_sv(String* s, const StringView* find, const StringView* replace);

bool string_replace_str(String* s, const String* find, const String* replace);

// -- comparison --

static inline bool is_string_empty(const String* s) {
    return is_string_view_empty((const StringView*)s);
}

static inline bool string_eq_cstr(const String* s, const char* cstr) {
    return string_view_eq_cstr((const StringView*)s, cstr);
}

static inline bool string_eq_bytes(const String* s, const u8* data, u64 len) {
    return string_view_eq_bytes((const StringView*)s, data, len);
}

static inline bool string_eq_sv(const String* s, const StringView* sv) {
    return string_view_eq_sv((const StringView*)s, sv);
}

static inline bool string_eq_str(const String* s1, const String* s2) {
    return string_view_eq_sv((const StringView*)s1, (const StringView*)s2);
}

static inline i64 string_cmp_cstr(const String* s, const char* cstr) {
    return string_view_cmp_cstr((const StringView*)s, cstr);
}

static inline i64 string_cmp_bytes(const String* s, const u8* data, u64 len) {
    return string_view_cmp_bytes((const StringView*)s, data, len);
}

static inline i64 string_cmp_sv(const String* s, const StringView* sv) {
    return string_view_cmp_sv((const StringView*)s, sv);
}

static inline i64 string_cmp_str(const String* s1, const String* s2) {
    return string_view_cmp_sv((const StringView*)s1, (const StringView*)s2);
}

// -- prefixes/suffixes --

static inline bool string_has_prefix_cstr(const String* s, const char* prefix) {
    return string_view_has_prefix_cstr((const StringView*)s, prefix);
}

static inline bool string_has_prefix_bytes(const String* s, const u8* prefix, u64 prefix_len) {
    return string_view_has_prefix_bytes((const StringView*)s, prefix, prefix_len);
}

static inline bool string_has_prefix_sv(const String* s, const StringView* prefix) {
    return string_view_has_prefix_sv((const StringView*)s, prefix);
}

static inline bool string_has_prefix_str(const String* s, const String* prefix) {
    return string_view_has_prefix_sv((const StringView*)s, (const StringView*)prefix);
}

static inline bool string_has_suffix_cstr(const String* s, const char* suffix) {
    return string_view_has_suffix_cstr((const StringView*)s, suffix);
}

static inline bool string_has_suffix_bytes(const String* s, const u8* suffix, u64 suffix_len) {
    return string_view_has_suffix_bytes((const StringView*)s, suffix, suffix_len);
}

static inline bool string_has_suffix_sv(const String* s, const StringView* suffix) {
    return string_view_has_suffix_sv((const StringView*)s, suffix);
}

static inline bool string_has_suffix_str(const String* s, const String* suffix) {
    return string_view_has_suffix_sv((const StringView*)s, (const StringView*)suffix);
}
// -- subview --

static inline StringView string_subview(const String* s, u64 offset, u64 len) {
    return string_view_subview((const StringView*)s, offset, len);
}

static inline String string_substr(const String* s, u64 offset, u64 len) {
    StringView sub = string_view_subview((const StringView*)s, offset, len);
    return string_from_sv(&sub);
}

// -- unicode --

static inline u64 string_count_runes(const String* s) {
    return string_view_count_runes((const StringView*)s);
}

static inline Rune string_rune_at_byte(const String* s, u64 byte_index, u32* rune_len) {
    return string_view_rune_at_byte((const StringView*)s, byte_index, rune_len);
}

static inline Rune string_rune_at(const String* s, u64 rune_index, u64* byte_index, u32* rune_len) {
    return string_view_rune_at((const StringView*)s, rune_index, byte_index, rune_len);
}

// -- search --

static inline bool string_contains_c(const String* s, char c) {
    return string_view_contains_c((const StringView*)s, c);
}

static inline bool string_contains_rune(const String* s, Rune r) {
    return string_view_contains_rune((const StringView*)s, r);
}

static inline bool string_contains_cstr(const String* s, const char* cstr) {
    return string_view_contains_cstr((const StringView*)s, cstr);
}

static inline bool string_contains_bytes(const String* s, const u8* data, u64 len) {
    return string_view_contains_bytes((const StringView*)s, data, len);
}

static inline bool string_contains_sv(const String* s, const StringView* sv) {
    return string_view_contains_sv((const StringView*)s, sv);
}

static inline bool string_contains_str(const String* s1, const String* s2) {
    return string_view_contains_sv((const StringView*)s1, (const StringView*)s2);
}

static inline u64 string_find_c(const String* s, char c) {
    return string_view_find_c((const StringView*)s, c);
}

static inline u64 string_find_rune(const String* s, Rune r) {
    return string_view_find_rune((const StringView*)s, r);
}

static inline u64 string_find_cstr(const String* s, const char* cstr) {
    return string_view_find_cstr((const StringView*)s, cstr);
}

static inline u64 string_find_bytes(const String* s, const u8* data, u64 len) {
    return string_view_find_bytes((const StringView*)s, data, len);
}

static inline u64 string_find_sv(const String* s, const StringView* sv) {
    return string_view_find_sv((const StringView*)s, sv);
}

static inline u64 string_find_str(const String* s1, const String* s2) {
    return string_view_find_sv((const StringView*)s1, (const StringView*)s2);
}

static inline u64 string_find_last_c(const String* s, char c) {
    return string_view_find_last_c((const StringView*)s, c);
}

static inline u64 string_find_last_rune(const String* s, Rune r) {
    return string_view_find_last_rune((const StringView*)s, r);
}

static inline u64 string_find_last_cstr(const String* s, const char* cstr) {
    return string_view_find_last_cstr((const StringView*)s, cstr);
}

static inline u64 string_find_last_bytes(const String* s, const u8* data, u64 len) {
    return string_view_find_last_bytes((const StringView*)s, data, len);
}

static inline u64 string_find_last_sv(const String* s, const StringView* sv) {
    return string_view_find_last_sv((const StringView*)s, sv);
}

static inline u64 string_find_last_str(const String* s1, const String* s2) {
    return string_view_find_last_sv((const StringView*)s1, (const StringView*)s2);
}