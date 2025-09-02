#include "vane/utils/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -- creation --

String string_create(u64 init_cap) {
    u64 cap = (init_cap > 0)
        ? init_cap
        : STRING_DEFAULT_CAPACITY;

    u8* data = malloc(cap + 1);
    assert(data != NULL);
    data[0] = '\0';

    return (String) { .data = data, .len = 0, .cap = cap, };
}

String string_from_cstr(const char* cstr) {
    if (cstr == NULL) {
        return STRING_EMPTY;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return STRING_EMPTY;
    }

    String result = string_create(len);
    memcpy(result.data, cstr, len);

    result.data[len] = '\0';
    result.len = len;

    return result;
}

String string_from_bytes(const u8* data, u64 len) {
    if (data == NULL || len == 0) {
        return STRING_EMPTY;
    }

    String result = string_create(len);
    memcpy(result.data, data, len);

    result.data[len] = '\0';
    result.len = len;

    return result;
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

    String result = string_create((u64)len);
    result.len = (u64)len;

    va_copy(va, _va);
    vsnprintf((char*)result.data, (u64)len + 1, fmt, va);
    va_end(va);

    return result;
}

String string_from_sv(StringView sv) {
    if (is_string_view_empty(sv)) {
        return STRING_EMPTY;
    }

    String result = string_create(sv.len);
    memcpy(result.data, sv.data, sv.len);

    result.data[sv.len] = '\0';
    result.len = sv.len;

    return result;
}

String string_clone(String s) {
    if (is_string_empty(s)) {
        return STRING_EMPTY;
    }

    String result = string_create(s.len);
    memcpy(result.data, s.data, s.len);

    result.data[s.len] = '\0';
    result.len = s.len;

    return result;
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
    s->cap = 0;
}

// -- appendings --

void string_append_c(String* s, char c) {
    assert(s != NULL);

    string_reserve(s, 1);

    s->data[s->len++] = c;
    s->data[s->len]   = '\0';
}

void string_append_rune(String* s, Rune r) {
    assert(s != NULL);

    u8 buf[4] = {0};
    u64 len = encode_utf8_rune(r, buf, sizeof(buf));

    if (len == 0) {
        return;
    }

    string_reserve(s, len);

    memcpy(s->data + s->len, buf, len);

    s->len += len;
    s->data[s->len] = '\0';
}

void string_append_cstr(String* s, const char* cstr) {
    assert(s != NULL);

    if (cstr == NULL) {
        return;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return;
    }

    string_reserve(s, len);

    memcpy(s->data + s->len, cstr, len + 1);

    s->len += len;
}

void string_append_bytes(String* s, const u8* data, u64 len) {
    assert(s != NULL);

    if (data == NULL || len == 0) {
        return;
    }

    string_reserve(s, len);

    memcpy(s->data + s->len, data, len);

    s->len += len;
    s->data[s->len] = '\0';
}

void string_append_fmt(String* s, const char* fmt, ...) {
    assert(s != NULL);

    if (fmt == NULL) {
        return;
    }

    va_list va;
    va_start(va, fmt);
    string_append_fmt_va(s, fmt, va);
    va_end(va);
}

void string_append_fmt_va(String* s, const char* fmt, va_list _va) {
    assert(s != NULL);

    if (fmt == NULL) {
        return;
    }

    va_list va;
    va_copy(va, _va);
    i32 len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);

    assert(len >= 0);

    if (len == 0) {
        return;
    }

    string_reserve(s, (u64)len);

    va_copy(va, _va);
    vsnprintf((char*)s->data + s->len, (u64)len + 1, fmt, va);
    va_end(va);

    s->len += len;
}

void string_append_sv(String* s, StringView sv) {
    assert(s != NULL);

    if (is_string_view_empty(sv)) {
        return;
    }

    string_reserve(s, sv.len);

    memcpy(s->data + s->len, sv.data, sv.len);

    s->len += sv.len;
    s->data[s->len] = '\0';
}

void string_append_str(String* s1, String s2) {
    assert(s1 != NULL);

    if (is_string_empty(s2)) {
        return;
    }

    string_reserve(s1, s2.len);

    memcpy(s1->data + s1->len, s2.data, s2.len);

    s1->len += s2.len;
    s1->data[s1->len] = '\0';
}

void string_append_left_c(String* s, char c) {
    assert(s != NULL);

    string_reserve(s, 1);

    memmove(s->data + 1, s->data, s->len + 1);
    s->data[0] = c;
    s->len++;
}

void string_append_left_rune(String* s, Rune r) {
    assert(s != NULL);

    u8 buf[4] = { 0 };
    u64 len = encode_utf8_rune(r, buf, sizeof(buf));

    if (len == 0) {
        return;
    }

    string_reserve(s, len);

    memmove(s->data + len, s->data, s->len + 1);
    memcpy(s->data, buf, len);

    s->len += len;
}

void string_append_left_cstr(String* s, const char* cstr) {
    assert(s != NULL);

    if (cstr == NULL) {
        return;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return;
    }

    string_reserve(s, len);

    memmove(s->data + len, s->data, s->len + 1);
    memcpy(s->data, cstr, len);

    s->len += len;
}

void string_append_left_bytes(String* s, const u8* data, u64 len) {
    assert(s != NULL);

    if (data == NULL || len == 0) {
        return;
    }

    string_reserve(s, len);

    memmove(s->data + len, s->data, s->len + 1);
    memcpy(s->data, data, len);

    s->len += len;
}

void string_append_left_fmt(String* s, const char* fmt, ...) {
    assert(s != NULL);

    if (fmt == NULL) {
        return;
    }

    va_list va;
    va_start(va, fmt);
    string_append_left_fmt_va(s, fmt, va);
    va_end(va);
}

void string_append_left_fmt_va(String* s, const char* fmt, va_list _va) {
    assert(s != NULL);

    if (fmt == NULL) {
        return;
    }

    va_list va;
    va_copy(va, _va);
    i32 len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);

    assert(len >= 0);

    if (len == 0) {
        return;
    }

    string_reserve(s, (u64)len);

    char ch = s->data[0];
    memmove(s->data + len, s->data, s->len + 1);

    va_copy(va, _va);
    vsnprintf((char*)s->data, (u64)len + 1, fmt, va);
    va_end(va);

    s->data[len] = ch;
    s->len += len;
}

void string_append_left_sv(String* s, StringView sv) {
    assert(s != NULL);

    if (is_string_view_empty(sv)) {
        return;
    }

    string_reserve(s, sv.len);

    memmove(s->data + sv.len, s->data, s->len + 1);
    memcpy(s->data, sv.data, sv.len);

    s->len += sv.len;
}

void string_append_left_str(String* s1, String s2) {
    assert(s1 != NULL);

    if (is_string_empty(s2)) {
        return;
    }

    string_reserve(s1, s2.len);

    memmove(s1->data + s2.len, s1->data, s1->len + 1);
    memcpy(s1->data, s2.data, s2.len);

    s1->len += s2.len;
}

// -- utilities

void string_shrink_to_fit(String* s) {
    assert(s != NULL);

    if (s->data == NULL || s->len == 0 || s->len == s->cap) {
        return;
    }

    u8* data = realloc(s->data, s->len + 1);
    assert(data != NULL);

    s->data = data;
    s->cap = s->len;
}

StringView string_get_view(String s) {
    return string_view_create(s.data, s.len);
}

void string_reserve(String* s, u64 additional_len) {
    assert(s != NULL);

    const u64 required = s->len + additional_len;
    if (required <= s->cap) {
        return;
    }

    u64 new_cap = (s->cap > 0)
        ? s->cap
        : STRING_DEFAULT_CAPACITY;

    if (new_cap == 1) {
        new_cap = 2;
    }

    while (new_cap < required) {
        const u64 next_cap = (u64)((f64)new_cap * STRING_CAPACITY_MULT);
        assert(next_cap > new_cap && "Capacity growth overflow");
        new_cap = next_cap;
    }

    u8* data = realloc(s->data, new_cap + 1);
    assert(data != NULL);

    data[s->len] = '\0';

    s->data = data;
    s->cap = new_cap;
}

void string_resize(String* s, u64 new_len, char fill) {
    assert(s != NULL);

    if (new_len < s->len) {
        s->data[new_len] = '\0';
        s->len = new_len;
        string_shrink_to_fit(s);
    }
    else if (new_len > s->len) {
        string_reserve(s, new_len - s->len);
        memset(s->data + s->len, fill, new_len - s->len);
        s->len = new_len;
        s->data[s->len] = '\0';
    }
}

// -- replacement --

bool string_replace_c(String* s, char find, char replace) {
    assert(s != NULL);

    if (is_string_empty(*s)) {
        return false;
    }

    bool replaced = false;
    for (u64 i = 0; i < s->len; ++i) {
        if (s->data[i] == find) {
            s->data[i] = replace;
            replaced = true;
        }
    }

    return replaced;
}

bool string_replace_rune(String* s, Rune find, Rune replace) {
    assert(s != NULL);

    if (is_string_empty(*s)) {
        return false;
    }

    u8 find_buf[4]    = {0};
    u8 replace_buf[4] = {0};

    u64 find_len = encode_utf8_rune(find, find_buf, sizeof(find_buf));
    u64 replace_len = encode_utf8_rune(replace, replace_buf, sizeof(replace_buf));

    return string_replace_bytes(s, find_buf, find_len, replace_buf, replace_len);
}

bool string_replace_cstr(String* s, const char* find, const char* replace) {
    assert(s != NULL);

    if (is_string_empty(*s) || find == NULL || *find == '\0') {
        return false;
    }

    const u64 find_len = strlen(find);

    u64 replace_len = 0;
    if (replace != NULL) {
        replace_len = strlen(replace);
    }

    return string_replace_bytes(s, (const u8*)find, find_len, (const u8*)replace, replace_len);
}

static inline u8* memfind(const u8* data, const u64 len, const u8* needle, const u64 needle_len) {
    if ((data == NULL) || (len == 0) ||
        (needle == NULL) || (needle_len == 0) ||
        len < needle_len) {
        return NULL;
    }

    for (u64 i = 0; i < len - needle_len + 1; ++i) {
        if (memcmp(data + i, needle, needle_len) == 0) {
            return (u8*)(data + i);
        }
    }

    return NULL;
}

bool string_replace_bytes(String* s, const u8* find, u64 find_len, const u8* replace, u64 replace_len) {
    assert(s != NULL);

    if (is_string_empty(*s) || (find == NULL || find_len == 0)) {
        return false;
    }

    if (find_len == 1 && replace_len == 1) {
        return string_replace_c(s, find[0], replace[0]);
    }

    u64 count = 0;
    for (u64 i = 0; i <= s->len - find_len; ) {
        if (memcmp(s->data + i, find, find_len) == 0) {
            count++;
            i += find_len;
        }
        else {
            ++i;
        }
    }

    if (count == 0) {
        return false;
    }

    u64 new_len = s->len + count * (replace_len - find_len);


    u8* new_data = malloc(new_len + 1);
    assert(new_data != NULL);

    u8* src = s->data;
    u8* dst = new_data;
    u64 remaining = s->len;

    while (remaining >= find_len) {
        if (memcmp(src, find, find_len) == 0) {
            memcpy(dst, replace, replace_len);
            dst += replace_len;
            src += find_len;
            remaining -= find_len;
        }
        else {
            *dst++ = *src++;
            remaining--;
        }
    }

    if (remaining != 0) {
        memcpy(dst, src, remaining);
        dst += remaining;
    }
    *dst = '\0';

    free(s->data);
    s->data = new_data;
    s->len = new_len;
    s->cap = new_len;

    return true;
}

bool string_replace_sv(String* s, StringView find, StringView replace) {
    assert(s != NULL);
    return string_replace_bytes(s, find.data, find.len, replace.data, replace.len);
}

bool string_replace_str(String* s, String find, String replace) {
    assert(s != NULL);
    return string_replace_bytes(s, find.data, find.len, replace.data, replace.len);
}