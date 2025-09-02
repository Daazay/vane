#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/unicode.h"
#include "vane/utils/hash.h"

typedef struct StringView StringView;

#define STR_LIT(S)           (StringView) { .data = (const u8*)S, .len = ARR_SIZE(S) - 1, }
#define STRING_VIEW_EMPTY (StringView) { .data = NULL, .len = 0, }

struct StringView {
    const u8* data;
    u64 len;
};

// -- creation --

StringView string_view_create(const u8* data, u64 len);

StringView string_view_from_cstr(const char* cstr);

// --  comparison --

bool is_string_view_empty(StringView sv);

bool string_view_eq_cstr(StringView sv, const char* cstr);

bool string_view_eq_bytes(StringView sv, const u8* data, u64 len);

bool string_view_eq_sv(StringView sv1, StringView sv2);

// -- prefixes/suffixes --

bool string_view_has_prefix_cstr(StringView sv, const char* prefix);

bool string_view_has_prefix_bytes(StringView sv, const u8* prefix, u64 prefix_len);

bool string_view_has_prefix_sv(StringView sv, StringView prefix);

bool string_view_has_suffix_cstr(StringView sv, const char* suffix);

bool string_view_has_suffix_bytes(StringView sv, const u8* suffix, u64 suffix_len);

bool string_view_has_suffix_sv(StringView sv, StringView suffix);

// -- subview --

StringView string_view_subview(StringView sv, u64 offset, u64 len);

// -- utilities --

u32 string_view_get_hash(StringView sv);

// -- collection utilities --

static inline bool __string_view_eq_sv(const StringView* sv1, const StringView* sv2) {
    assert(sv1 != NULL && sv2 != NULL);
    return string_view_eq_sv(*sv1, *sv2);
}

static inline u32 __string_view_get_hash(const StringView* sv) {
    assert(sv != NULL);
    return string_view_get_hash(*sv);
}

// -- unicode --

u64 string_view_count_runes(StringView sv);

Rune string_view_rune_at_byte(StringView sv, u64 byte_idx, u32* rune_len);

Rune string_view_rune_at(StringView sv, u64 idx, u64* byte_idx, u32* rune_len);

// -- search --

u64 string_view_find_c(StringView sv, char c);

u64 string_view_find_rune(StringView sv, Rune r);

u64 string_view_find_cstr(StringView sv, const char* cstr);

u64 string_view_find_bytes(StringView sv, const u8* data, u64 len);

u64 string_view_find_sv(StringView sv1, StringView sv2);

u64 string_view_find_last_c(StringView sv, char c);

u64 string_view_find_last_rune(StringView sv, Rune r);

u64 string_view_find_last_cstr(StringView sv, const char* cstr);

u64 string_view_find_last_bytes(StringView sv, const u8* data, u64 len);

u64 string_view_find_last_sv(StringView sv1, StringView sv2);

// -- contains --

static inline bool string_view_contains_c(StringView sv, char c) {
    return string_view_find_c(sv, c) != (u64)NPOS;
}

static inline bool string_view_contains_rune(StringView sv, Rune r) {
    return string_view_find_rune(sv, r) != (u64)NPOS;
}

static inline bool string_view_contains_cstr(StringView sv, const char* cstr) {
    return string_view_find_cstr(sv, cstr) != (u64)NPOS;
}

static inline bool string_view_contains_bytes(StringView sv, const u8* data, u64 len) {
    return string_view_find_bytes(sv, data, len) != (u64)NPOS;
}

static inline bool string_view_contains_sv(StringView sv1, StringView sv2) {
    return string_view_find_sv(sv1, sv2) != (u64)NPOS;
}