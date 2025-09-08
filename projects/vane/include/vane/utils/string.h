#pragma once

#include <stdarg.h>

#include "vane/utils/defines.h"
#include "vane/utils/unicode.h"
#include "vane/utils/string_view.h"

typedef struct String String;

#define STRING_EMPTY (String) { .data = NULL, .len = 0, }

struct String {
    u8* data;
    u64 len;
};

// -- creation --

String string_from_cstr(const char* cstr);

String string_from_bytes(const u8* data, u64 len);

String string_from_fmt(const char* fmt, ...);

String string_from_fmt_va(const char* fmt, va_list va);

String string_from_sv(StringView sv);

String string_clone(String s);

// -- destruction --

void string_clear(String* s);

void string_destroy(String* s);

// -- utilities --

bool is_string_empty(String s);

StringView string_get_view(String s);