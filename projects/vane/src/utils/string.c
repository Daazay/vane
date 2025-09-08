#include "vane/utils/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -- creation --

String string_from_cstr(const char* cstr) {
    if (cstr == NULL) {
        return STRING_EMPTY;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return STRING_EMPTY;
    }

    u8* data = malloc(len + 1);
    assert(data != NULL);

    memcpy(data, cstr, len);
    data[len] = '\0';

    return (String) { .data = data, .len = len, };
}

String string_from_bytes(const u8* data, u64 len) {
    if (data == NULL || len == 0) {
        return STRING_EMPTY;
    }

    u8* buf = malloc(len + 1);
    assert(buf != NULL);

    memcpy(buf, data, len);
    buf[len] = '\0';

    return (String) { .data = buf, .len = len, };
}

String string_from_fmt(const char* fmt, ...) {
    if (fmt == NULL) {
        return STRING_EMPTY;
    }

    va_list va;
    va_start(va, fmt);
    String result = string_from_fmt_va(fmt, va);
    va_end(va);

    return result;
}

String string_from_fmt_va(const char* fmt, va_list _va) {
    if (fmt == NULL) {
        return STRING_EMPTY;
    }

    va_list va;
    va_copy(va, _va);
    i32 len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);

    assert(len >= 0);

    if (len == 0) {
        return STRING_EMPTY;
    }

    u8* buf = malloc((u64)len + 1);
    assert(buf != NULL);

    va_copy(va, _va);
    vsnprintf((char*)buf, (u64)len + 1, fmt, va);
    va_end(va);

    return (String) { .data = buf, .len = (u64)len, };
}

String string_from_sv(StringView sv) {
    if (is_string_view_empty(sv)) {
        return STRING_EMPTY;
    }

    u8* buf = malloc(sv.len + 1);
    assert(buf != NULL);

    memcpy(buf, sv.data, sv.len);
    buf[sv.len] = '\0';

    return (String) { .data = buf, .len = sv.len, };
}

String string_clone(String s) {
    return string_from_sv(string_get_view(s));
}

// -- destruction --

void string_clear(String* s) {
    assert(s != NULL);

    if (s->data != NULL) {
        s->data[0] = '\0';
    }
    s->len = 0;
}

void string_destroy(String* s) {
    if (s == NULL) {
        return;
    }

    if (s->data != NULL) {
        free(s->data);
        s->data = NULL;
    }

    s->len = 0;
}

// -- utilities --

bool is_string_empty(String s) {
    return s.len == 0 || s.data == NULL;
}

StringView string_get_view(String s) {
    return string_view_create(s.data, s.len);
}