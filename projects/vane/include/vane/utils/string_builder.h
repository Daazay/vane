#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/unicode.h"
#include "vane/utils/string.h"
#include "vane/utils/string_view.h"

typedef struct StringBuilder StringBuilder;

#define STRING_BUILDER_DEFAULT_CAPACITY 32
#define STRING_BUILDER_CAPACITY_MULT    1.5

struct StringBuilder {
    u8* data;
    u64 len;
    u64 cap;
};

// -- creation --

StringBuilder string_builder_create(u64 init_cap);

// -- destruction --

void string_builder_clear(StringBuilder* sb);

void string_builder_destroy(StringBuilder* sb);

// -- utilities --

bool is_string_builder_empty(StringBuilder sb);

void string_builder_shrink_to_fit(StringBuilder* sb);

void string_builder_reserve(StringBuilder* sb, u64 additional_len);

void string_builder_resize(StringBuilder* sb, u64 new_len, char fill);

StringView string_builder_get_view(StringBuilder sb);

String string_builder_build(StringBuilder sb);

String string_builder_release(StringBuilder* sb);

// -- appenings --

void string_builder_append_c(StringBuilder* sb, char c);

void string_builder_append_rune(StringBuilder* sb, Rune r);

void string_builder_append_cstr(StringBuilder* sb, const char* cstr);

void string_builder_append_bytes(StringBuilder* sb, const u8* data, u64 len);

void string_builder_append_fmt(StringBuilder* sb, const char* fmt, ...);

void string_builder_append_fmt_va(StringBuilder* sb, const char* fmt, va_list va);

void string_builder_append_sv(StringBuilder* sb, StringView sv);

//

void string_builder_append_left_c(StringBuilder* sb, char c);

void string_builder_append_left_rune(StringBuilder* sb, Rune r);

void string_builder_append_left_cstr(StringBuilder* sb, const char* cstr);

void string_builder_append_left_bytes(StringBuilder* sb, const u8* data, u64 len);

void string_builder_append_left_fmt(StringBuilder* sb, const char* fmt, ...);

void string_builder_append_left_fmt_va(StringBuilder* sb, const char* fmt, va_list va);

void string_builder_append_left_sv(StringBuilder* sb, StringView sv);

// -- replacement --

bool string_builder_replace_c(StringBuilder* sb, char find, char replace);

bool string_builder_replace_rune(StringBuilder* sb, Rune find, Rune replace);

bool string_builder_replace_cstr(StringBuilder* sb, const char* find, const char* replace);

bool string_builder_replace_bytes(StringBuilder* sb, const u8* find, u64 find_len, const u8* replace, u64 replace_len);

bool string_builder_replace_sv(StringBuilder* sb, StringView find, StringView replace);

// -- erasement --

bool string_builder_erase_c(StringBuilder* sb, char c);

bool string_builder_erase_rune(StringBuilder* sb, Rune r);

bool string_builder_erase_cstr(StringBuilder* sb, const char* cstr);

bool string_builder_erase_bytes(StringBuilder* sb, const u8* data, u64 len);

bool string_builder_erase_sv(StringBuilder* sb, StringView sv);