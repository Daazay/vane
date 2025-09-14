#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/unicode.h"

struct String;
typedef struct StringView StringView;

#define STR_LIT(S)        (StringView) { .data = (const u8*)S, .len = sizeof(S) - 1, }
#define STRING_VIEW_EMPTY (StringView) { .data = NULL, .len = 0, }

#define SV_FMT "%.*s"
#define SV_ARG(SV) (i32)(SV).len, (const char*)(SV).data

struct StringView {
    const u8* data;
    u64 len;
};

// -- creation --

StringView string_view_create(const u8* data, u64 len);

StringView string_view_from_cstr(const char* cstr);

// -- comparison --

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

static inline bool string_view_item_eq(const void* item1, const void* item2) {
    assert(item1 != NULL && item2 != NULL);
    const StringView* sv1 = (const StringView*)item1;
    const StringView* sv2 = (const StringView*)item2;
    return string_view_eq_sv(*sv1, *sv2);
}

static inline u32 string_view_item_hash(const void* item) {
    assert(item != NULL);
    const StringView* sv = (const StringView*)item;
    return string_view_get_hash(*sv);
}

// -- unicode --

u64 string_view_count_runes(StringView sv);

Rune string_view_rune_at_byte(StringView sv, u64 byte_idx, u32* rune_len);

Rune string_view_rune_at(StringView sv, u64 idx, u64* byte_idx, u32* rune_len);

bool string_view_utf8_to_utf16_str(StringView sv, struct String* out);

bool string_view_utf16_to_utf8_str(StringView sv, struct String* out);

// -- trim --

StringView string_view_trim_start(StringView sv);

StringView string_view_trim_end(StringView sv);

StringView string_view_trim(StringView sv);

// -- search --

u64 string_view_find_c_with_offset(StringView sv, u64 offset, char c);

u64 string_view_find_rune_with_byte_offset(StringView sv, u64 byte_offset, Rune r);

u64 string_view_find_rune_with_rune_offset(StringView sv, u64 rune_offset, Rune r);

u64 string_view_find_cstr_with_offset(StringView sv, u64 offset, const char* cstr);

u64 string_view_find_bytes_with_offset(StringView sv, u64 offset, const u8* data, u64 len);

u64 string_view_find_sv_with_offset(StringView sv, u64 offset, StringView sv2);

//

u64 string_view_find_last_c_with_offset(StringView sv, u64 offset, char c);

u64 string_view_find_last_rune_with_byte_offset(StringView sv, u64 byte_offset, Rune r);

u64 string_view_find_last_rune_with_rune_offset(StringView sv, u64 rune_offset, Rune r);

u64 string_view_find_last_cstr_with_offset(StringView sv, u64 offset, const char* cstr);

u64 string_view_find_last_bytes_with_offset(StringView sv, u64 offset, const u8* data, u64 len);

u64 string_view_find_last_sv_with_offset(StringView sv, u64 offset, StringView sv2);

//

static inline u64 string_view_find_c(StringView sv, char c) {
    return string_view_find_c_with_offset(sv, 0, c);
}

static inline u64 string_view_find_rune(StringView sv, Rune r) {
    return string_view_find_rune_with_rune_offset(sv, 0, r);
}

static inline u64 string_view_find_rune_byte(StringView sv, Rune r) {
    return string_view_find_rune_with_byte_offset(sv, 0, r);
}

static inline u64 string_view_find_cstr(StringView sv, const char* cstr) {
    return string_view_find_cstr_with_offset(sv, 0, cstr);
}

static inline u64 string_view_find_bytes(StringView sv, const u8* data, u64 len) {
    return string_view_find_bytes_with_offset(sv, 0, data, len);
}

static inline u64 string_view_find_sv(StringView sv, StringView sv2) {
    return string_view_find_sv_with_offset(sv, 0, sv2);
}

//

static inline u64 string_view_find_last_c(StringView sv, char c) {
    return string_view_find_last_c_with_offset(sv, sv.len - 1, c);
}

static inline u64 string_view_find_last_rune(StringView sv, Rune r) {
    return string_view_find_last_rune_with_rune_offset(sv, string_view_count_runes(sv) - 1, r);
}

static inline u64 string_view_find_last_rune_byte(StringView sv, Rune r) {
    return string_view_find_last_rune_with_byte_offset(sv, 0, r);
}

static inline u64 string_view_find_last_cstr(StringView sv, const char* cstr) {
    return string_view_find_last_cstr_with_offset(sv, sv.len - 1, cstr);
}

static inline u64 string_view_find_last_bytes(StringView sv, const u8* data, u64 len) {
    return string_view_find_last_bytes_with_offset(sv, sv.len - 1, data, len);
}

static inline u64 string_view_find_last_sv(StringView sv, StringView sv2) {
    return string_view_find_last_sv_with_offset(sv, sv.len - 1, sv2);
}

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