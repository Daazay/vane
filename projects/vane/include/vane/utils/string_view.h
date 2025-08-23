#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/unicode.h"

typedef struct StringView StringView;

#define STRING_VIEW_EMPTY (StringView) { .data = NULL, .len = 0, }
#define STR_LIT(S)        (StringView) { .data = S, .len = ARR_SIZE(S) - 1, }

struct StringView {
    const u8* data;
    u64 len;
};

// -- creation --

StringView string_view_create(const u8* data, u64 len);

StringView string_view_from_cstr(const char* cstr);

// -- comparison --

bool is_string_view_empty(const StringView* sv);

bool string_view_eq_cstr(const StringView* sv, const char* cstr);

bool string_view_eq_bytes(const StringView* sv, const u8* data, u64 len);

bool string_view_eq_sv(const StringView* sv1, const StringView* sv2);

i64 string_view_cmp_cstr(const StringView* sv, const char* cstr);

i64 string_view_cmp_bytes(const StringView* sv1, const u8* data, u64 len);

i64 string_view_cmp_sv(const StringView* sv1, const StringView* sv2);

// -- prefixes/suffixes --

bool string_view_has_prefix_cstr(const StringView* sv, const char* prefix);

bool string_view_has_prefix_bytes(const StringView* sv, const u8* prefix, u64 prefix_len);

bool string_view_has_prefix_sv(const StringView* sv, const StringView* prefix);

bool string_view_has_suffix_cstr(const StringView* sv, const char* suffix);

bool string_view_has_suffix_bytes(const StringView* sv, const u8* suffix, u64 suffix_len);

bool string_view_has_suffix_sv(const StringView* sv, const StringView* suffix);

// -- subview --

StringView string_view_subview(const StringView* sv, u64 offset, u64 len);

// -- unicode --

u64 string_view_count_runes(const StringView* sv);

Rune string_view_rune_at_byte(const StringView* sv, u64 byte_index, u32* rune_len);

Rune string_view_rune_at(const StringView* sv, u64 rune_index, u64* byte_index, u32* rune_len);

// -- search --

bool string_view_contains_c(const StringView* sv, char c);

bool string_view_contains_rune(const StringView* sv, Rune r);

bool string_view_contains_cstr(const StringView* sv, const char* cstr);

bool string_view_contains_bytes(const StringView* sv, const u8* data, u64 len);

bool string_view_contains_sv(const StringView* sv1, const StringView* sv2);

u64 string_view_find_c(const StringView* sv, char c);

u64 string_view_find_rune(const StringView* sv, Rune r);

u64 string_view_find_cstr(const StringView* sv, const char* cstr);

u64 string_view_find_bytes(const StringView* sv, const u8* data, u64 len);

u64 string_view_find_sv(const StringView* sv1, const StringView* sv2);

u64 string_view_find_last_c(const StringView* sv, char c);

u64 string_view_find_last_rune(const StringView* sv, Rune r);

u64 string_view_find_last_cstr(const StringView* sv, const char* cstr);

u64 string_view_find_last_bytes(const StringView* sv, const u8* data, u64 len);

u64 string_view_find_last_sv(const StringView* sv1, const StringView* sv2);