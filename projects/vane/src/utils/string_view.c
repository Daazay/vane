#include "vane/utils/string_view.h"

#include <string.h>

// -- creation --

StringView string_view_create(const u8* data, u64 len) {
    return (StringView) { .data = data, .len = len, };
}

StringView string_view_from_cstr(const char* cstr) {
    if (cstr == NULL) {
        return STRING_VIEW_EMPTY;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return STRING_VIEW_EMPTY;
    }

    return (StringView) { .data = (const u8*)cstr, .len = len, };
}

// -- comparison --

bool is_string_view_empty(const StringView* sv) {
    assert(sv != NULL);
    return sv->data == NULL || sv->len == 0;
}

bool string_view_eq_cstr(const StringView* sv, const char* cstr) {
    assert(sv != NULL);

    if (cstr == NULL) {
        cstr = "";
    }

    const u64 len = strlen(cstr);

    if (sv->len != len) {
        return false;
    }

    if (len == 0) {
        return true;
    }

    if (sv->data == (const u8*)cstr) {
        return true;
    }

    return memcmp(sv->data, cstr, len) == 0;
}

bool string_view_eq_bytes(const StringView* sv, const u8* data, u64 len) {
    assert(sv != NULL);

    if (sv->len != len) {
        return false;
    }

    if (len == 0) {
        return true;
    }

    if (sv->data == data) {
        return true;
    }

    return memcmp(sv->data, data, len) == 0;
}

bool string_view_eq_sv(const StringView* sv1, const StringView* sv2) {
    assert(sv1 != NULL && sv2 != NULL);

    if (sv1->len != sv2->len) {
        return false;
    }

    if (sv1->len == 0) {
        return true;
    }

    if (sv1->data == sv2->data) {
        return true;
    }

    return memcmp(sv1->data, sv2->data, sv1->len) == 0;
}

i64 string_view_cmp_cstr(const StringView* sv, const char* cstr) {
    assert(sv != NULL);

    if (cstr == NULL) {
        cstr = "";
    }

    const u64 len = strlen(cstr);
    const u64 min_len = (sv->len < len)
        ? sv->len
        : len;

    i64 cmp = 0;
    if (min_len > 0 && sv->data != (const u8*)cstr) {
        cmp = (i64)memcmp(sv->data, cstr, min_len);
    }

    if (cmp != 0) {
        return (i64)cmp;
    }

    return (i64)sv->len - (i64)len;
}

i64 string_view_cmp_bytes(const StringView* sv, const u8* data, u64 len) {
    assert(sv != NULL);

    const u64 min_len = (sv->len < len)
        ? sv->len
        : len;

    i64 cmp = 0;
    if (min_len > 0 && sv->data != data) {
        cmp = (i64)memcmp(sv->data, data, min_len);
    }

    if (cmp != 0) {
        return (i64)cmp;
    }

    return (i64)sv->len - (i64)len;
}

i64 string_view_cmp_sv(const StringView* sv1, const StringView* sv2) {
    assert(sv1 != NULL && sv2 != NULL);

    const u64 min_len = (sv1->len < sv2->len)
        ? sv1->len
        : sv2->len;

    i64 cmp = 0;
    if (min_len > 0 && sv1->data != sv2->data) {
        cmp = (i64)memcmp(sv1->data, sv2->data, min_len);
    }

    if (cmp != 0) {
        return (i64)cmp;
    }

    return (i64)sv1->len - (i64)sv2->len;
}

// -- prefixes/suffixes --

bool string_view_has_prefix_cstr(const StringView* sv, const char* prefix) {
    assert(sv != NULL);

    if (prefix == NULL) {
        prefix = "";
    }

    const u64 prefix_len = strlen(prefix);
    if (prefix_len > sv->len) {
        return false;
    }

    if (prefix_len == 0) {
        return true;
    }

    if (sv->data == (const u8*)prefix) {
        return true;
    }

    return memcmp(sv->data, prefix, prefix_len) == 0;
}

bool string_view_has_prefix_bytes(const StringView* sv, const u8* prefix, u64 prefix_len) {
    assert(sv != NULL);

    if (prefix_len > sv->len) {
        return false;
    }

    if (prefix_len == 0) {
        return true;
    }

    if (sv->data == prefix) {
        return true;
    }

    return memcmp(sv->data, prefix, prefix_len) == 0;
}

bool string_view_has_prefix_sv(const StringView* sv, const StringView* prefix) {
    assert(sv != NULL && prefix != NULL);

    if (prefix->len > sv->len) {
        return false;
    }

    if (sv->len == 0) {
        return true;
    }

    if (sv->data == prefix->data) {
        return true;
    }

    return memcmp(sv->data, prefix->data, prefix->len) == 0;
}

bool string_view_has_suffix_cstr(const StringView* sv, const char* suffix) {
    assert(sv != NULL);

    if (suffix == NULL) {
        suffix = "";
    }

    const u64 suffix_len = strlen(suffix);
    if (suffix_len > sv->len) {
        return false;
    }

    if (suffix_len == 0) {
        return true;
    }

    const u8* data = sv->data + sv->len - suffix_len;
    if (data == (const u8*)suffix) {
        return true;
    }

    return memcmp(data, suffix, suffix_len) == 0;
}

bool string_view_has_suffix_bytes(const StringView* sv, const u8* suffix, u64 suffix_len) {
    assert(sv != NULL);

    if (suffix_len > sv->len) {
        return false;
    }

    if (suffix_len == 0) {
        return true;
    }

    const u8* data = sv->data + sv->len - suffix_len;
    if (data == suffix) {
        return true;
    }

    return memcmp(data, suffix, suffix_len) == 0;
}

bool string_view_has_suffix_sv(const StringView* sv, const StringView* suffix) {
    assert(sv != NULL && suffix != NULL);

    if (suffix->len > sv->len) {
        return false;
    }

    if (suffix->len == 0) {
        return true;
    }

    const u8* data = sv->data + sv->len - suffix->len;
    if (data == suffix->data) {
        return true;
    }

    return memcmp(data, suffix->data, suffix->len) == 0;
}

// -- subview --

StringView string_view_subview(const StringView* sv, u64 offset, u64 len) {
    assert(sv != NULL);

    if (offset >= sv->len) {
        return STRING_VIEW_EMPTY;
    }

    const u64 reslen = (offset + len > sv->len)
        ? sv->len - offset
        : len;

    return (StringView) { .data = sv->data + offset, .len = reslen, };
}

// -- unicode --

u64 string_view_count_runes(const StringView* sv) {
    assert(sv != NULL);
    return utf8_count_runes(sv->data, sv->len);
}

Rune string_view_rune_at_byte(const StringView* sv, u64 byte_index, u32* rune_len) {
    assert(sv != NULL);

    if (is_string_view_empty(sv)) {
        if (rune_len != NULL) {
            *rune_len = 0;
        }
        return RUNE_EOF;
    }

    u64 read_len = 0;
    Rune r = utf8_decode_rune(sv->data + byte_index, sv->len - byte_index, &read_len);

    if (rune_len != NULL) {
        *rune_len = (u32)read_len;
    }

    return r;
}

Rune string_view_rune_at(const StringView* sv, u64 rune_index, u64* byte_index, u32* rune_len) {
    assert(sv != NULL);

    u64 pos = 0;
    u64 count = 0;
    u64 read_len = 0;

    while (pos < sv->len) {
        Rune r = utf8_decode_rune(sv->data + pos, sv->len - pos, &read_len);

        if (count == rune_index) {
            if (byte_index != NULL) {
                *byte_index = pos;
            }
            if (rune_len != NULL) {
                *rune_len = (u32)read_len;
            }
            return r;
        }

        pos += read_len;
        ++count;
    }

    if (byte_index != NULL) {
        *byte_index = sv->len;
    }
    if (rune_len != NULL) {
        *rune_len = 0;
    }

    return RUNE_EOF;
}

// -- search --

bool string_view_contains_c(const StringView* sv, char c) {
    assert(sv != NULL);

    for (u64 i = 0; i < sv->len; ++i) {
        if (sv->data[i] == c) {
            return true;
        }
    }

    return false;
}

bool string_view_contains_rune(const StringView* sv, Rune r) {
    assert(sv != NULL);

    u8 buf[4] = {0};
    u64 len = utf8_encode_rune(r, buf, sizeof(buf));

    return string_view_contains_bytes(sv, buf, len);
}

bool string_view_contains_cstr(const StringView* sv, const char* cstr) {
    assert(sv != NULL);

    if (cstr == NULL) {
        return true;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return true;
    }

    if (sv->len < len) {
        return false;
    }

    for (u64 i = 0; i <= sv->len - len; ++i) {
        if (memcmp(sv->data + i, cstr, len) == 0) {
            return true;
        }
    }

    return false;
}

bool string_view_contains_bytes(const StringView* sv, const u8* data, u64 len) {
    assert(sv != NULL);

    if (data == NULL || len == 0) {
        return true;
    }

    if (sv->len < len) {
        return false;
    }

    for (u64 i = 0; i <= sv->len - len; ++i) {
        if (memcmp(sv->data + i, data, len) == 0) {
            return true;
        }
    }

    return false;
}

bool string_view_contains_sv(const StringView* sv1, const StringView* sv2) {
    assert(sv1 != NULL && sv2 != NULL);

    if (sv2->len == 0) {
        return true;
    }

    if (sv1->len < sv2->len) {
        return false;
    }

    for (u64 i = 0; i <= sv1->len - sv2->len; ++i) {
        if (memcmp(sv1->data + i, sv2->data, sv2->len) == 0) {
            return true;
        }
    }

    return false;
}

u64 string_view_find_c(const StringView* sv, char c) {
    assert(sv != NULL);

    for (u64 i = 0; i < sv->len; ++i) {
        if (sv->data[i] == c) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_rune(const StringView* sv, Rune r) {
    assert(sv != NULL);

    u8 buf[4] = { 0 };
    u64 len = utf8_encode_rune(r, buf, sizeof(buf));

    return string_view_find_bytes(sv, buf, len);
}

u64 string_view_find_cstr(const StringView* sv, const char* cstr) {
    assert(sv != NULL);

    if (cstr == NULL) {
        return 0;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return 0;
    }

    if (sv->len < len) {
        return (u64)NPOS;
    }

    for (u64 i = 0; i <= sv->len - len; ++i) {
        if (memcmp(sv->data + i, cstr, len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_bytes(const StringView* sv, const u8* data, u64 len) {
    assert(sv != NULL);

    if (data == NULL || len == 0) {
        return 0;
    }

    if (sv->len < len) {
        return (u64)NPOS;
    }

    for (u64 i = 0; i <= sv->len - len; ++i) {
        if (memcmp(sv->data + i, data, len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_sv(const StringView* sv1, const StringView* sv2) {
    assert(sv1 != NULL && sv2 != NULL);

    if (sv2->len == 0) {
        return 0;
    }

    if (sv1->len < sv2->len) {
        return (u64)NPOS;
    }

    for (u64 i = 0; i <= sv1->len - sv2->len; ++i) {
        if (memcmp(sv1->data + i, sv2->data, sv2->len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_last_c(const StringView* sv, char c) {
    assert(sv != NULL);

    for (u64 i = sv->len; i-- > 0; ) {
        if (sv->data[i] == c) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_last_rune(const StringView* sv, Rune r) {
    assert(sv != NULL);

    u8 buf[4] = { 0 };
    u64 len = utf8_encode_rune(r, buf, sizeof(buf));

    return string_view_find_last_bytes(sv, buf, len);
}

u64 string_view_find_last_cstr(const StringView* sv, const char* cstr) {
    assert(sv != NULL);

    if (cstr == NULL) {
        return 0;
    }

    const u64 cstr_len = strlen(cstr);

    if (cstr_len == 0) {
        return 0;
    }

    if (sv->len < cstr_len) {
        return (u64)NPOS;
    }

    for (u64 i = sv->len - cstr_len + 1; i-- > 0;) {
        if (memcmp(sv->data + i, cstr, cstr_len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_last_bytes(const StringView* sv, const u8* data, u64 len) {
    assert(sv != NULL);

    if (data == NULL || len == 0) {
        return 0;
    }

    if (sv->len < len) {
        return (u64)NPOS;
    }

    for (u64 i = sv->len - len + 1; i-- > 0;) {
        if (memcmp(sv->data + i, data, len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_last_sv(const StringView* sv1, const StringView* sv2) {
    assert(sv1 != NULL && sv2 != NULL);

    if (sv2->len == 0) {
        return 0;
    }

    if (sv1->len < sv2->len) {
        return (u64)NPOS;
    }

    for (u64 i = sv1->len - sv2->len + 1; i-- > 0;) {
        if (memcmp(sv1->data + i, sv2->data, sv2->len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}