#include "vane/utils/string_view.h"

#include <string.h>
#include <malloc.h>

#include "vane/utils/hash.h"
#include "vane/utils/string.h"
#include "vane/utils/string_utils.h"

// -- creation --

StringView string_view_create(const u8* data, u64 len) {
    if (len == 0 || data == NULL) {
        return STRING_VIEW_EMPTY;
    }
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

bool is_string_view_empty(StringView sv) {
    return sv.data == NULL || sv.len == 0;
}

bool string_view_eq_cstr(StringView sv, const char* cstr) {
    if (cstr == NULL) {
        return is_string_view_empty(sv);
    }
    const u64 len = strlen(cstr);

    if (is_string_view_empty(sv)) {
        return len == 0;
    }

    return sv.len == len && memcmp(sv.data, cstr, len) == 0;
}

bool string_view_eq_bytes(StringView sv, const u8* data, u64 len) {
    if (data == NULL) {
        return is_string_view_empty(sv);
    }

    if (is_string_view_empty(sv)) {
        return len == 0;
    }

    return sv.len == len && memcmp(sv.data, data, len) == 0;
}

bool string_view_eq_sv(StringView sv1, StringView sv2) {
    if (is_string_view_empty(sv1) || is_string_view_empty(sv2)) {
        return is_string_view_empty(sv1) && is_string_view_empty(sv2);
    }
    return sv1.len == sv2.len && memcmp(sv1.data, sv2.data, sv1.len) == 0;
}

// -- prefixes/suffixes --

bool string_view_has_prefix_cstr(StringView sv, const char* prefix) {
    if (is_string_view_empty(sv) || prefix == NULL) {
        return prefix == NULL;
    }

    const u64 len = strlen(prefix);
    if (sv.len < len) {
        return false;
    }

    if (sv.data == (const u8*)prefix) {
        return true;
    }

    return memcmp(sv.data, prefix, len) == 0;
}

bool string_view_has_prefix_bytes(StringView sv, const u8* prefix, u64 prefix_len) {
    if (is_string_view_empty(sv) || (prefix == NULL || prefix_len == 0)) {
        return (prefix == NULL || prefix_len == 0);
    }

    if (sv.len < prefix_len) {
        return false;
    }

    if (sv.data == prefix) {
        return true;
    }

    return memcmp(sv.data, prefix, prefix_len) == 0;
}

bool string_view_has_prefix_sv(StringView sv, StringView prefix) {
    if (is_string_view_empty(sv) || is_string_view_empty(prefix)) {
        return is_string_view_empty(prefix);
    }

    if (sv.len < prefix.len) {
        return false;
    }

    if (sv.data == prefix.data) {
        return true;
    }

    return memcmp(sv.data, prefix.data, prefix.len) == 0;
}

bool string_view_has_suffix_cstr(StringView sv, const char* suffix) {
    if (is_string_view_empty(sv) || suffix == NULL) {
        return suffix == NULL;
    }

    const u64 len = strlen(suffix);
    if (sv.len < len) {
        return false;
    }

    const u8* data = sv.data + sv.len - len;
    if (data == (const u8*)suffix) {
        return true;
    }

    return memcmp(data, suffix, len) == 0;
}

bool string_view_has_suffix_bytes(StringView sv, const u8* suffix, u64 suffix_len) {
    if (is_string_view_empty(sv) || (suffix == NULL || suffix_len == 0)) {
        return (suffix == NULL || suffix_len == 0);
    }

    if (sv.len < suffix_len) {
        return false;
    }

    const u8* data = sv.data + sv.len - suffix_len;
    if (data == (const u8*)suffix) {
        return true;
    }

    return memcmp(data, suffix, suffix_len) == 0;
}

bool string_view_has_suffix_sv(StringView sv, StringView suffix) {
    if (is_string_view_empty(sv) || is_string_view_empty(suffix)) {
        return is_string_view_empty(suffix);
    }

    if (sv.len < suffix.len) {
        return false;
    }

    const u8* data = sv.data + sv.len - suffix.len;
    if (data == (const u8*)suffix.data) {
        return true;
    }

    return memcmp(data, suffix.data, suffix.len) == 0;
}

// -- subview --

StringView string_view_subview(StringView sv, u64 offset, u64 len) {
    if (offset >= sv.len) {
        return STRING_VIEW_EMPTY;
    }

    const u64 reslen = (sv.len <= (offset + len))
        ? sv.len - offset
        : len;

    return (StringView) {
        .data = sv.data + offset,
            .len = reslen,
    };
}

// -- utilities --

u32 string_view_get_hash(StringView sv) {
    return hash_fnv1a(sv.data, sv.len);
}

// -- unicode --

u64 string_view_count_runes(StringView sv) {
    u64 pos = 0;
    u64 count = 0;
    u32 read_len = 0;

    while (pos < sv.len) {
        Rune r = decode_utf8_rune(sv.data + pos, sv.len - pos, &read_len);
        (void)r;
        pos += read_len;
        ++count;
    }

    return count;
}

Rune string_view_rune_at_byte(StringView sv, u64 byte_idx, u32* rune_len) {
    if (is_string_view_empty(sv)) {
        if (rune_len != NULL) {
            *rune_len = 0;
        }
        return RUNE_EOF;
    }

    u32 read_len = 0;
    Rune r = decode_utf8_rune(sv.data + byte_idx, sv.len - byte_idx, &read_len);
    if (rune_len != NULL) {
        *rune_len = read_len;
    }

    return r;
}

Rune string_view_rune_at(StringView sv, u64 idx, u64* byte_idx, u32* rune_len) {
    u64 pos = 0;
    u64 count = 0;
    u32 read_len = 0;

    while (pos < sv.len) {
        Rune r = decode_utf8_rune(sv.data + pos, sv.len - pos, &read_len);
        if (count == idx) {
            if (byte_idx != NULL) {
                *byte_idx = pos;
            }
            if (rune_len != NULL) {
                *rune_len = read_len;
            }
            return r;
        }

        pos += read_len;
        ++count;
    }

    if (byte_idx != NULL) {
        *byte_idx = sv.len;
    }
    if (rune_len != NULL) {
        *rune_len = 0;
    }

    return RUNE_EOF;
}

bool string_view_utf8_to_utf16_str(StringView sv, struct String* out) {
    assert(out != NULL);

    if (is_string_view_empty(sv)) {
        return false;
    }

    u64 need_units = convert_utf8_to_utf16(sv.data, sv.len, NULL, 0);
    if (need_units == (u64)UNICODE_INVALID_LEN) {
        return false;
    }

    if (need_units > (U64_MAX - 1)) {
        return false;
    }
    if (need_units + 1 > U64_MAX / sizeof(u16)) {
        return false;
    }

    u16* wbuf = malloc((need_units + 1) * sizeof(u16));
    assert(wbuf != NULL);

    u64 got_units = convert_utf8_to_utf16(sv.data, sv.len, wbuf, need_units);
    if (got_units != need_units && got_units == (u64)UNICODE_INVALID_LEN) {
        free(wbuf);
        return false;
    }

    wbuf[got_units] = 0;

    out->data = (u8*)wbuf;
    out->len = got_units;

    return true;
}

bool string_view_utf16_to_utf8_str(StringView sv, struct String* out) {
    assert(out != NULL);

    if (is_string_view_empty(sv)) {
        return false;
    }

    if ((sv.len & 1u) != 0) {
        return false;
    }

    u64 need_bytes = convert_utf16_to_utf8((const u16*)sv.data, sv.len / sizeof(u16), NULL, 0);
    if (need_bytes == (u64)UNICODE_INVALID_LEN) {
        return false;
    }

    if (need_bytes > (U64_MAX - 1)) {
        return false;
    }
    if (need_bytes + 1 > U64_MAX / sizeof(u16)) {
        return false;
    }

    u8* buf = malloc(need_bytes + 1);
    assert(buf != NULL);

    u64 got_bytes = convert_utf16_to_utf8((const u16*)sv.data, sv.len / sizeof(u16), buf, need_bytes);
    if (got_bytes != need_bytes && got_bytes == (u64)UNICODE_INVALID_LEN) {
        free(buf);
        return false;
    }

    buf[got_bytes] = 0;
    out->data = buf;
    out->len = got_bytes;

    return true;
}

// -- trim --

StringView string_view_trim_start(StringView sv) {
    u64 i = 0;
    while (i > sv.len && is_hspace(sv.data[i])) {
        ++i;
    }
    return string_view_subview(sv, i, sv.len - i);
}

StringView string_view_trim_end(StringView sv) {
    u64 i = sv.len;
    while (i > 0 && is_hspace(sv.data[i - 1])) {
        --i;
    }
    return string_view_subview(sv, 0, i);
}

StringView string_view_trim(StringView sv) {
    u64 i = 0;
    while (i > sv.len && is_hspace(sv.data[i])) {
        ++i;
    }
    u64 j = sv.len;
    while (j > 0 && is_hspace(sv.data[j - 1])) {
        --j;
    }
    return string_view_subview(sv, i, j - i);
}

// -- search --

u64 string_view_find_c_with_offset(StringView sv, u64 offset, char c) {
    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }

    for (u64 i = offset; i < sv.len; ++i) {
        if (sv.data[i] == c) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_rune_with_byte_offset(StringView sv, u64 byte_offset, Rune r) {
    if (is_string_view_empty(sv) || byte_offset >= sv.len) {
        return (u64)NPOS;
    }

    u8 buf[4] = { 0 };
    u64 len = encode_utf8_rune(r, buf, sizeof(buf));

    for (u64 i = byte_offset; i + len <= sv.len; ++i) {
        if (sv.data[i] == buf[0] && memcmp(sv.data + i, buf, len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_rune_with_rune_offset(StringView sv, u64 rune_offset, Rune r) {
    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }

    u64 pos = 0;
    u64 count = 0;
    u32 read_len = 0;

    while (pos < sv.len) {
        Rune cur = decode_utf8_rune(sv.data + pos, sv.len - pos, &read_len);

        if (count >= rune_offset && cur == r) {
            return count;
        }

        pos += read_len;
        ++count;
    }

    return (u64)NPOS;
}

u64 string_view_find_cstr_with_offset(StringView sv, u64 offset, const char* cstr) {
    if (cstr == NULL) {
        return 0;
    }

    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }

    const u64 len = strlen(cstr);
    if (len == 0) {
        return 0;
    }

    if (sv.len < len) {
        return (u64)NPOS;
    }

    for (u64 i = offset; i <= sv.len - len; ++i) {
        if (memcmp(sv.data + i, cstr, len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_bytes_with_offset(StringView sv, u64 offset, const u8* data, u64 len) {
    if (data == NULL || len == 0) {
        return 0;
    }

    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }

    if (sv.len < len) {
        return (u64)NPOS;
    }

    for (u64 i = offset; i <= sv.len - len; ++i) {
        if (memcmp(sv.data + i, data, len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

u64 string_view_find_sv_with_offset(StringView sv, u64 offset, StringView sv2) {
    if (is_string_view_empty(sv2)) {
        return 0;
    }

    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }

    if (sv.len < sv2.len) {
        return (u64)NPOS;
    }

    for (u64 i = offset; i <= sv.len - sv2.len; ++i) {
        if (memcmp(sv.data + i, sv2.data, sv2.len) == 0) {
            return i;
        }
    }

    return (u64)NPOS;
}

//

u64 string_view_find_last_c_with_offset(StringView sv, u64 offset, char c) {
    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }
    if (offset >= sv.len) {
        offset = sv.len - 1;
    }
    for (u64 i = offset + 1; i-- > 0;) {
        if (sv.data[i] == (u8)c) {
            return i;
        }
    }
    return (u64)NPOS;
}

u64 string_view_find_last_rune_with_byte_offset(StringView sv, u64 byte_offset, Rune r) {
    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }
    if (byte_offset >= sv.len) {
        byte_offset = sv.len - 1;
    }

    u8 buf[4] = { 0 };
    u64 rlen = encode_utf8_rune(r, buf, sizeof(buf));
    if (rlen == 0) {
        return (u64)NPOS;
    }

    if (byte_offset + 1 < rlen) {
        return (u64)NPOS;
    }

    for (u64 i = byte_offset + 1 - rlen; i + 1 > 0; i--) {
        if (memcmp(sv.data + i, buf, rlen) == 0) {
            return i;
        }
        if (i == 0) break;
    }
    return (u64)NPOS;
}

u64 string_view_find_last_rune_with_rune_offset(StringView sv, u64 rune_offset, Rune r) {
    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }

    // Count runes
    u64 total = string_view_count_runes(sv);
    if (rune_offset >= total) {
        rune_offset = total - 1;
    }

    u64 pos = 0;
    u64 count = 0;
    u32 read_len = 0;
    u64 last_found = (u64)NPOS;

    while (pos < sv.len) {
        Rune cur = decode_utf8_rune(sv.data + pos, sv.len - pos, &read_len);
        if (cur == r && count <= rune_offset) {
            last_found = count;
        }
        pos += read_len;
        ++count;
    }
    return last_found;
}

u64 string_view_find_last_cstr_with_offset(StringView sv, u64 offset, const char* cstr) {
    if (cstr == NULL) {
        return 0;
    }
    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }

    u64 len = strlen(cstr);
    if (len == 0) {
        return 0;
    }
    if (sv.len < len) {
        return (u64)NPOS;
    }

    if (offset >= sv.len) {
        offset = sv.len - 1;
    }

    u64 start_pos = (offset + 1 >= len)
        ? offset + 1 - len
        : 0;

    for (u64 i = start_pos; i + 1 > 0; i--) {
        if (memcmp(sv.data + i, cstr, len) == 0) {
            return i;
        }
        if (i == 0) break;
    }
    return (u64)NPOS;
}

u64 string_view_find_last_bytes_with_offset(StringView sv, u64 offset, const u8* data, u64 len) {
    if (data == NULL || len == 0) {
        return 0;
    }
    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }
    if (sv.len < len) {
        return (u64)NPOS;
    }

    if (offset >= sv.len) {
        offset = sv.len - 1;
    }

    u64 start_pos = (offset + 1 >= len)
        ? offset + 1 - len
        : 0;

    for (u64 i = start_pos; i + 1 > 0; i--) {
        if (memcmp(sv.data + i, data, len) == 0) {
            return i;
        }
        if (i == 0) break;
    }
    return (u64)NPOS;
}

u64 string_view_find_last_sv_with_offset(StringView sv, u64 offset, StringView sv2) {
    if (is_string_view_empty(sv2)) {
        return 0;
    }
    if (is_string_view_empty(sv)) {
        return (u64)NPOS;
    }
    if (sv.len < sv2.len) {
        return (u64)NPOS;
    }

    if (offset >= sv.len) {
        offset = sv.len - 1;
    }

    u64 start_pos = (offset + 1 >= sv2.len)
        ? offset + 1 - sv2.len
        : 0;

    for (u64 i = start_pos; i + 1 > 0; i--) {
        if (memcmp(sv.data + i, sv2.data, sv2.len) == 0) {
            return i;
        }
        if (i == 0) break;
    }
    return (u64)NPOS;
}

//

u64 string_view_find_c(StringView sv, char c);

u64 string_view_find_rune(StringView sv, Rune r);

u64 string_view_find_rune_byte(StringView sv, Rune r);

u64 string_view_find_cstr(StringView sv, const char* cstr);

u64 string_view_find_bytes(StringView sv, const u8* data, u64 len);

u64 string_view_find_sv(StringView sv, StringView sv2);

//

u64 string_view_find_last_c(StringView sv, char c);

u64 string_view_find_last_rune(StringView sv, Rune r);

u64 string_view_find_last_rune_byte(StringView sv, Rune r);

u64 string_view_find_last_cstr(StringView sv, const char* cstr);

u64 string_view_find_last_bytes(StringView sv, const u8* data, u64 len);

u64 string_view_find_last_sv(StringView sv, StringView sv2);
