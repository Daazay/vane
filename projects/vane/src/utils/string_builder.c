#include "vane/utils/string_builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// -- creation --

StringBuilder string_builder_create(u64 init_cap) {
    u64 cap = (init_cap > 0)
        ? init_cap
        : STRING_BUILDER_DEFAULT_CAPACITY;

    u8* data = malloc(cap + 1);
    assert(data != NULL);
    data[0] = '\0';

    return (StringBuilder) { .data = data, .len = 0, .cap = cap, };
}

// -- destruction --

void string_builder_clear(StringBuilder* sb) {
    assert(sb != NULL);

    if (sb->data != NULL) {
        sb->data[0] = '\0';
    }
    sb->len = 0;
}

void string_builder_destroy(StringBuilder* sb) {
    if (sb == NULL) {
        return;
    }

    if (sb->data != NULL) {
        free(sb->data);
        sb->data = NULL;
    }

    sb->len = 0;
    sb->cap = 0;
}

// -- utilities --

bool is_string_builder_empty(StringBuilder sb) {
    return sb.len == 0 || sb.data == 0;
}

void string_builder_shrink_to_fit(StringBuilder* sb) {
    assert(sb != NULL);

    if (sb->data == NULL || sb->len == 0 || sb->len == sb->cap) {
        return;
    }

    u8* data = realloc(sb->data, sb->len + 1);
    assert(data != NULL);

    sb->data = data;
    sb->cap = sb->len;
}

void string_builder_reserve(StringBuilder* sb, u64 additional_len) {
    assert(sb != NULL);

    const u64 required = sb->len + additional_len;
    if (required <= sb->cap) {
        return;
    }

    u64 new_cap = (sb->cap > 0)
        ? sb->cap
        : STRING_BUILDER_DEFAULT_CAPACITY;

    if (new_cap == 1) {
        new_cap = 2;
    }

    while (new_cap < required) {
        const u64 next_cap = (u64)((f64)new_cap * STRING_BUILDER_CAPACITY_MULT);
        assert(next_cap > new_cap && "Capacity growth overflow");
        new_cap = next_cap;
    }

    u8* data = realloc(sb->data, new_cap + 1);
    assert(data != NULL);

    data[sb->len] = '\0';

    sb->data = data;
    sb->cap = new_cap;
}

void string_builder_resize(StringBuilder* sb, u64 new_len, char fill) {
    assert(sb != NULL);

    if (new_len < sb->len) {
        sb->data[new_len] = '\0';
        sb->len = new_len;

        string_builder_shrink_to_fit(sb);
    }
    else if (new_len > sb->len) {
        string_builder_reserve(sb, new_len - sb->len);

        memset(sb->data + sb->len, fill, new_len - sb->len);
        sb->len = new_len;
        sb->data[sb->len] = '\0';
    }
}

StringView string_builder_get_view(StringBuilder sb) {
    return string_view_create(sb.data, sb.len);
}

String string_builder_build(StringBuilder sb) {
    if (is_string_builder_empty(sb)) {
        return STRING_EMPTY;
    }

    u8* data = malloc(sb.len + 1);
    assert(data != NULL);

    memcpy(data, sb.data, sb.len);

    data[sb.len] = '\0';

    return (String) { .data = data, .len = sb.len, };
}

String string_builder_release(StringBuilder* sb) {
    assert(sb != NULL);

    string_builder_shrink_to_fit(sb);

    String s = (String) {
        .data = sb->data,
        .len = sb->len,
    };

    sb->data = NULL;
    sb->cap = 0;
    sb->len = 0;

    return s;
}

// -- appenings --

void string_builder_append_c(StringBuilder* sb, char c) {
    assert(sb != NULL);

    string_builder_reserve(sb, 1);

    sb->data[sb->len++] = c;
    sb->data[sb->len]   = '\0';
}

void string_builder_append_rune(StringBuilder* sb, Rune r) {
    assert(sb != NULL);

    u8 buf[4] = { 0 };
    u64 len = encode_utf8_rune(r, buf, sizeof(buf));

    if (len == 0) {
        return;
    }

    string_builder_reserve(sb, len);

    memcpy(sb->data + sb->len, buf, len);

    sb->len += len;
    sb->data[sb->len] = '\0';
}

void string_builder_append_cstr(StringBuilder* sb, const char* cstr) {
    assert(sb != NULL);

    if (cstr == NULL) {
        return;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return;
    }

    string_builder_reserve(sb, len);

    memcpy(sb->data + sb->len, cstr, len + 1);

    sb->len += len;
}

void string_builder_append_bytes(StringBuilder* sb, const u8* data, u64 len) {
    assert(sb != NULL);

    if (data == NULL || len == 0) {
        return;
    }

    string_builder_reserve(sb, len);

    memcpy(sb->data + sb->len, data, len);

    sb->len += len;
    sb->data[sb->len] = '\0';
}

void string_builder_append_fmt(StringBuilder* sb, const char* fmt, ...) {
    assert(sb != NULL);

    if (fmt == NULL) {
        return;
    }

    va_list va;
    va_start(va, fmt);
    string_builder_append_fmt_va(sb, fmt, va);
    va_end(va);
}

void string_builder_append_fmt_va(StringBuilder* sb, const char* fmt, va_list _va) {
    assert(sb != NULL);

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

    string_builder_reserve(sb, (u64)len);

    va_copy(va, _va);
    vsnprintf((char*)sb->data + sb->len, (u64)len + 1, fmt, va);
    va_end(va);

    sb->len += len;
}

void string_builder_append_sv(StringBuilder* sb, StringView sv) {
    assert(sb != NULL);

    if (is_string_view_empty(sv)) {
        return;
    }

    string_builder_reserve(sb, sv.len);

    memcpy(sb->data + sb->len, sv.data, sv.len);

    sb->len += sv.len;
    sb->data[sb->len] = '\0';
}

//

void string_builder_append_left_c(StringBuilder* sb, char c) {
    assert(sb != NULL);

    string_builder_reserve(sb, 1);

    memmove(sb->data + 1, sb->data, sb->len + 1);
    sb->data[0] = c;
    sb->len++;
}

void string_builder_append_left_rune(StringBuilder* sb, Rune r) {
    assert(sb != NULL);

    u8 buf[4] = { 0 };
    u64 len = encode_utf8_rune(r, buf, sizeof(buf));

    if (len == 0) {
        return;
    }

    string_builder_reserve(sb, len);

    memmove(sb->data + len, sb->data, sb->len + 1);
    memcpy(sb->data, buf, len);

    sb->len += len;
}

void string_builder_append_left_cstr(StringBuilder* sb, const char* cstr) {
    assert(sb != NULL);

    if (cstr == NULL) {
        return;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return;
    }

    string_builder_reserve(sb, len);

    memmove(sb->data + len, sb->data, sb->len + 1);
    memcpy(sb->data, cstr, len);

    sb->len += len;
}

void string_builder_append_left_bytes(StringBuilder* sb, const u8* data, u64 len) {
    assert(sb != NULL);

    if (data == NULL || len == 0) {
        return;
    }

    string_builder_reserve(sb, len);

    memmove(sb->data + len, sb->data, sb->len + 1);
    memcpy(sb->data, data, len);

    sb->len += len;
}

void string_builder_append_left_fmt(StringBuilder* sb, const char* fmt, ...) {
    assert(sb != NULL);

    if (fmt == NULL) {
        return;
    }

    va_list va;
    va_start(va, fmt);
    string_builder_append_left_fmt_va(sb, fmt, va);
    va_end(va);
}

void string_builder_append_left_fmt_va(StringBuilder* sb, const char* fmt, va_list _va) {
    assert(sb != NULL);

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

    string_builder_reserve(sb, (u64)len);

    char ch = sb->data[0];
    memmove(sb->data + len, sb->data, sb->len + 1);

    va_copy(va, _va);
    vsnprintf((char*)sb->data, (u64)len + 1, fmt, va);
    va_end(va);

    sb->data[len] = ch;
    sb->len += len;
}

void string_builder_append_left_sv(StringBuilder* sb, StringView sv) {
    assert(sb != NULL);

    if (is_string_view_empty(sv)) {
        return;
    }

    string_builder_reserve(sb, sv.len);

    memmove(sb->data + sv.len, sb->data, sb->len + 1);
    memcpy(sb->data, sv.data, sv.len);

    sb->len += sv.len;
}

// -- replacement --

bool string_builder_replace_c(StringBuilder* sb, char find, char replace) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb)) {
        return false;
    }

    bool is_replaced = false;
    for (u64 i = 0; i < sb->len; ++i) {
        if (sb->data[i] == find) {
            sb->data[i] = replace;
            is_replaced = true;
        }
    }

    return is_replaced;
}

bool string_builder_replace_rune(StringBuilder* sb, Rune find, Rune replace) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb)) {
        return false;
    }

    u8 find_buf[4] = { 0 };
    u8 replace_buf[4] = { 0 };

    u64 find_len = encode_utf8_rune(find, find_buf, sizeof(find_buf));
    u64 replace_len = encode_utf8_rune(replace, replace_buf, sizeof(replace_buf));

    return string_builder_replace_bytes(sb, find_buf, find_len, replace_buf, replace_len);
}

bool string_builder_replace_cstr(StringBuilder* sb, const char* find, const char* replace) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb) || find == NULL || *find == '\0') {
        return false;
    }

    const u64 find_len = strlen(find);

    u64 replace_len = 0;
    if (replace != NULL) {
        replace_len = strlen(replace);
    }

    return string_builder_replace_bytes(sb, (const u8*)find, find_len, (const u8*)replace, replace_len);
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

bool string_builder_replace_bytes(StringBuilder* sb, const u8* find, u64 find_len, const u8* replace, u64 replace_len) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb) || (find == NULL || find_len == 0)) {
        return false;
    }

    if (find_len == 1 && replace_len == 1) {
        return string_builder_replace_c(sb, find[0], replace[0]);
    }

    if (replace_len == 0) {
        return string_builder_erase_bytes(sb, find, find_len);
    }

    u64 count = 0;
    for (u64 i = 0; i <= sb->len - find_len; ) {
        if (memcmp(sb->data + i, find, find_len) == 0) {
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

    u64 new_len = sb->len + count * (replace_len - find_len);


    u8* new_data = malloc(new_len + 1);
    assert(new_data != NULL);

    u8* src = sb->data;
    u8* dst = new_data;
    u64 remaining = sb->len;

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

    free(sb->data);
    sb->data = new_data;
    sb->len = new_len;
    sb->cap = new_len;

    return true;
}

bool string_builder_replace_sv(StringBuilder* sb, StringView find, StringView replace) {
    assert(sb != NULL);
    return string_builder_replace_bytes(sb, find.data, find.len, replace.data, replace.len);
}

// -- erasement --

bool string_builder_erase_c(StringBuilder* sb, char c) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb)) {
        return false;
    }

    u64 dst = 0;
    bool is_erased = false;

    for (u64 src = 0; src < sb->len; ++src) {
        if (sb->data[src] != c) {
            sb->data[dst++] = sb->data[src];
        }
        else {
            is_erased = true;
        }
    }

    sb->len = dst;
    sb->data[sb->len] = '\0';

    return is_erased;
}

bool string_builder_erase_rune(StringBuilder* sb, Rune r) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb)) {
        return false;
    }

    u8 buf[4] = { 0 };
    u64 rune_len = encode_utf8_rune(r, buf, sizeof(buf));
    if (rune_len == 0) {
        return false;
    }

    return string_builder_erase_bytes(sb, buf, rune_len);
}

bool string_builder_erase_cstr(StringBuilder* sb, const char* cstr) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb) || cstr == NULL || *cstr == '\0') {
        return false;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return false;
    }

    return string_builder_erase_bytes(sb, (const u8*)cstr, len);
}

bool string_builder_erase_bytes(StringBuilder* sb, const u8* data, u64 len) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb) || data == NULL || len == 0) {
        return false;
    }

    u64 dst = 0;
    u64 i = 0;
    bool is_erased = false;

    while (i <= sb->len - len) {
        if (memcmp(sb->data + i, data, len) == 0) {
            i += len;
            is_erased = true;
        }
        else {
            sb->data[dst++] = sb->data[i++];
        }
    }

    while (i < sb->len) {
        sb->data[dst++] = sb->data[i++];
    }

    sb->len = dst;
    sb->data[sb->len] = '\0';

    return is_erased;
}

bool string_builder_erase_sv(StringBuilder* sb, StringView sv) {
    assert(sb != NULL);

    if (is_string_builder_empty(*sb) || is_string_view_empty(sv)) {
        return false;
    }

    return string_builder_erase_bytes(sb, sv.data, sv.len);
}