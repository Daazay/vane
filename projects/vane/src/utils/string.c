#include "vane/utils/string.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// -- creation --

String string_create(u64 init_cap) {
    u64 cap = (init_cap > 0)
        ? init_cap
        : STRING_DEFAULT_CAPACITY;

    byte* data = malloc(cap + 1);
    assert(data != NULL);
    data[0] = '\0';

    return (String) { .data = data, .len = 0, .cap = cap, };
}

String string_from_cstr(const char* cstr) {
    if (cstr == NULL || *cstr == '\0') {
        return string_create(0);
    }

    const u64 len = strlen(cstr);

    String result = string_create(len);
    memcpy(result.data, cstr, len);

    result.data[len] = '\0';
    result.len = len;

    return result;
}

String string_from_bytes(const u8* bytes, u64 len) {
    if (bytes == NULL || len == 0) {
        return string_create(0);
    }

    String result = string_create(len);
    memcpy(result.data, bytes, len);

    result.data[len] = '\0';
    result.len = len;

    return result;
}

String string_from_sv(const StringView* sv) {
    assert(sv != NULL);

    if (is_string_view_empty(sv)) {
        return string_create(0);
    }

    String result = string_create(sv->len);
    memcpy(result.data, sv->data, sv->len);

    result.data[sv->len] = '\0';
    result.len = sv->len;

    return result;
}

String string_from_fmt(const char* format, ...) {
    assert(format != NULL);

    va_list va;
    va_start(va, format);
    String result = string_from_fmt_va(format, va);
    va_end(va);

    return result;
}

String string_from_fmt_va(const char* format, va_list _va) {
    assert(format != NULL);

    va_list va;
    va_copy(va, _va);
    i32 len = vsnprintf(NULL, 0, format, va);
    va_end(va);

    assert(len >= 0);

    if (len == 0) {
        return string_create(0);
    }

    String result = string_create((u64)len);
    result.len = (u64)len;

    va_copy(va, _va);
    vsnprintf((char*)result.data, (u64)len + 1, format, va);
    va_end(va);

    return result;
}

String string_clone(const String* s) {
    assert(s != NULL);

    if (is_string_empty(s)) {
        return string_create(0);
    }

    String result = string_create(s->cap);
    memcpy(result.data, s->data, s->len + 1);
    result.len = s->len;

    return result;
}

// -- destruction --

void string_clear(String* s) {
    assert(s != NULL);

    s->len = 0;

    if (s->data != NULL) {
        s->data[0] = '\0';
    }
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

//

String string_concat_cstr(const char* cstr1, const char* cstr2) {
    if (cstr1 == NULL || *cstr1 == '\0') {
        return string_from_cstr(cstr2);
    }
    if (cstr2 == NULL || *cstr2 == '\0') {
        return string_from_cstr(cstr1);
    }

    const u64 len1 = strlen(cstr1);
    const u64 len2 = strlen(cstr2);

    const u64 reslen = len1 + len2;
    String result = string_create(reslen);
    memcpy(result.data, cstr1, len1);
    memcpy(result.data + len1, cstr2, len2);

    result.data[reslen] = '\0';
    result.len = reslen;

    return result;
}

String string_concat_bytes(const u8* data1, u64 len1, const u8* data2, u64 len2) {
    if (data1 == NULL || len1 == 0) {
        return string_from_bytes(data2, len2);
    }
    if (data2 == NULL || len2 == 0) {
        return string_from_bytes(data1, len1);
    }

    const u64 reslen = len1 + len2;
    String result = string_create(reslen);
    memcpy(result.data, data1, len1);
    memcpy(result.data + len1, data2, len2);

    result.data[reslen] = '\0';
    result.len = reslen;

    return result;
}

String string_concat_sv(const StringView* sv1, const StringView* sv2) {
    assert(sv1 != NULL && sv2 != NULL);
    return string_concat_bytes(sv1->data, sv1->len, sv2->data, sv2->len);
}

String string_concat_str(const String* s1, const String* s2) {
    assert(s1 != NULL && s2 != NULL);
    return string_concat_bytes(s1->data, s1->len, s2->data, s2->len);
}

//

static void string_ensure_capacity(String* s, u64 appended_len) {
    const u64 required = s->len + appended_len;
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

    u8* new_data = realloc(s->data, new_cap + 1);
    assert(new_data != NULL);

    s->data = new_data;
    s->cap = new_cap;
}

void string_append_c(String* s, char c) {
    assert(s != NULL);
    string_append_bytes(s, (const u8*)&c, 1);
}

void string_append_rune(String* s, Rune r) {
    assert(s != NULL);

    u8 buf[4] = {0};
    u64 len = utf8_encode_rune(r, buf, sizeof(buf));

    string_append_bytes(s, buf, len);
}

void string_append_cstr(String* s, const char* cstr) {
    assert(s != NULL);

    if (cstr == NULL || *cstr == '\0') {
        return;
    }

    const u64 len = strlen(cstr);
    string_append_bytes(s, (const u8*)cstr, len);
}

void string_append_bytes(String* s, const u8* bytes, u64 len) {
    assert(s != NULL);

    if (bytes == NULL || len == 0) {
        return;
    }

    string_ensure_capacity(s, len);

    memcpy(s->data + s->len, bytes, len);
    s->len += len;
    s->data[s->len] = '\0';
}

void string_append_sv(String* s, const StringView* sv) {
    assert(s != NULL && sv != NULL);
    string_append_bytes(s, sv->data, sv->len);
}

void string_append_str(String* s, const String* s2) {
    assert(s != NULL && s2 != NULL);
    string_append_bytes(s, s2->data, s2->len);
}

void string_append_left_c(String* s, char c) {
    assert(s != NULL);
    string_append_left_bytes(s, (const u8*)&c, 1);
}

void string_append_left_rune(String* s, Rune r) {
    assert(s != NULL);

    u8 buf[4] = { 0 };
    u64 len = utf8_encode_rune(r, buf, sizeof(buf));

    string_append_left_bytes(s, buf, len);
}

void string_append_left_cstr(String* s, const char* cstr) {
    assert(s != NULL);

    if (cstr == NULL || *cstr == '\0') {
        return;
    }

    const u64 len = strlen(cstr);
    string_append_left_bytes(s, (const u8*)cstr, len);
}

void string_append_left_bytes(String* s, const u8* bytes, u64 len) {
    assert(s != NULL);

    if (bytes == NULL || len == 0) {
        return;
    }

    string_ensure_capacity(s, len);

    memmove(s->data + len, s->data, s->len + 1);
    memcpy(s->data, bytes, len);

    s->len += len;
}

void string_append_left_sv(String* s, const StringView* sv) {
    assert(s != NULL && sv != NULL);
    string_append_left_bytes(s, sv->data, sv->len);
}

void string_append_left_str(String* s, const String* s2) {
    assert(s != NULL && s2 != NULL);
    string_append_left_bytes(s, s2->data, s2->len);
}

// -- utilities --

void string_shrink_to_fit(String* s) {
    assert(s != NULL);

    if (s->cap == s->len) {
        return;
    }

    u8* new_data = realloc(s->data, s->len + 1);
    assert(new_data != NULL);

    s->data = new_data;
    s->cap = s->len;
}

StringView string_get_view(const String* s) {
    assert(s != NULL);
    return string_view_create(s->data, s->len);
}

void string_reserve(String* s, u64 additional) {
    assert(s != NULL);
    string_ensure_capacity(s, additional);
}

void string_resize(String* s, u64 new_len, char fill) {
    assert(s != NULL);

    if (new_len < s->len) {
        s->len = new_len;
        s->data[s->len] = '\0';
    }
    else if (new_len > s->len) {
        string_ensure_capacity(s, new_len - s->len);
        memset(s->data + s->len, fill, new_len - s->len);
        s->len = new_len;
        s->data[s->len] = '\0';
    }
}

// -- replacement --

bool string_replace_c(String* s, char find, char replace) {
    assert(s != NULL);

    if (is_string_empty(s)) {
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

    if (is_string_empty(s)) {
        return false;
    }

    u8 find_buf[4] = {0};
    u8 replace_buf[4] = {0};

    u64 find_len = utf8_encode_rune(find, find_buf, sizeof(find_buf));
    u64 replace_len = utf8_encode_rune(replace, replace_buf, sizeof(replace_buf));

    if (find_len == 1 && replace_len == 1) {
        return string_replace_c(s, find_buf[0], replace_buf[0]);
    }

    return string_replace_bytes(s, find_buf, find_len, replace_buf, replace_len);
}

bool string_replace_cstr(String* s, const char* find, const char* replace) {
    assert(s != NULL);

    if (is_string_empty(s)) {
        return false;
    }

    if (find == NULL || *find == '\0') {
        return false;
    }

    const u64 find_len = strlen(find);
    u64 replace_len = 0;

    if (replace != NULL) {
        replace_len = strlen(replace);
    }

    if (find_len == 1 && replace_len == 1) {
        return string_replace_c(s, find[0], replace[0]);
    }

    return string_replace_bytes(s, (const u8*)find, find_len, (const u8*)replace, replace_len);
}

static inline u8* memfind(const u8* data, const u64 data_len, const u8* find, const u64 find_len) {
    if ((data == NULL) || (data_len == 0) ||
        (find == NULL) || (find_len == 0) ||
        data_len < find_len) {
        return NULL;
    }

    for (u64 i = 0; i < data_len - find_len + 1; ++i) {
        if (memcmp(data + i, find, find_len) == 0) {
            return (u8*)(data + i);
        }
    }

    return NULL;
}

bool string_replace_bytes(String* s, const u8* find, u64 find_len, const u8* replace, u64 replace_len) {
    assert(s != NULL);

    if (is_string_empty(s)) {
        return false;
    }

    if (find == NULL || find_len == 0) {
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

bool string_replace_sv(String* s, const StringView* find, const StringView* replace) {
    assert(s != NULL && find != NULL && replace != NULL);
    return string_replace_bytes(s, find->data, find->len, replace->data, replace->len);
}

bool string_replace_str(String* s, const String* find, const String* replace) {
    assert(s != NULL && find != NULL && replace != NULL);
    return string_replace_bytes(s, find->data, find->len, replace->data, replace->len);
}