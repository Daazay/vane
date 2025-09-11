#include "vane/utils/win.h"

#include <stdlib.h>

#if defined(PLATFORM_WINDOWS)

#include "vane/utils/string_builder.h"

bool win_utf8_to_utf16_alloc(StringView u8_sv, u16** out_w, u64* out_len) {
    if (out_w != NULL) {
        *out_w = NULL;
    }
    if (out_len != NULL) {
        *out_len = 0;
    }

    u64 need = convert_utf8_to_utf16(u8_sv.data, u8_sv.len, NULL, 0);
    if (need == (u64)UNICODE_INVALID_LEN) {
        return false;
    }

    u16* wbuf = malloc((need + 1) * sizeof(u16));
    assert(wbuf != NULL);

    convert_utf8_to_utf16(u8_sv.data, u8_sv.len, wbuf, need);
    wbuf[need] = 0;

    if (out_w != NULL) {
        *out_w = wbuf;
    }
    if (out_len != NULL) {
        *out_len = need;
    }

    return true;
}

String win_utf16_to_utf8_str(const u16* wbuf, u64 wlen) {
    if (wbuf == NULL) {
        return STRING_EMPTY;
    }

    u64 u8_len = convert_utf16_to_utf8(wbuf, wlen, NULL, 0);
    if (u8_len == (u64)UNICODE_INVALID_LEN) {
        return STRING_EMPTY;
    }

    StringBuilder sb = string_builder_create(u8_len);
    sb.len = convert_utf16_to_utf8(wbuf, wlen, sb.data, u8_len);
    sb.data[sb.len] = 0;

    return string_builder_release(&sb);
}

#endif