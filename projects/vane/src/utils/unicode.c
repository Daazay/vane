#include "vane/utils/unicode.h"

// Surrogate ranges (UTF-16)
#define RUNE_SURROGATE_START      0xD800
#define RUNE_SURROGATE_END        0xDFFF
#define RUNE_HIGH_SURROGATE_START 0xD800
#define RUNE_HIGH_SURROGATE_END   0xDBFF
#define RUNE_LOW_SURROGATE_START  0xDC00
#define RUNE_LOW_SURROGATE_END    0xDFFF

#define RUNE_SUPPLEMENTARY_MIN    0x00010000

// Useful masks / constants for UTF-16 surrogate math
#define SURROGATE_CHILD_MASK      0x03FF
#define HIGH_SURROGATE_BASE       0xD800
#define LOW_SURROGATE_BASE        0xDC00
#define SURROGATE_OFFSET          0x10000

// UTF-8 bit patterns
#define UTF8_ASCII_MAX            0x7F

// UTF-8 byte masks
#define UTF8_1BYTE_MASK           0x80
#define UTF8_2BYTE_MASK           0xE0
#define UTF8_3BYTE_MASK           0xF0
#define UTF8_4BYTE_MASK           0xF8
#define UTF8_CONT_MASK            0xC0

// UTF-8 byte markers
#define UTF8_1BYTE_MARKER         0x00
#define UTF8_2BYTE_MARKER         0xC0
#define UTF8_3BYTE_MARKER         0xE0
#define UTF8_4BYTE_MARKER         0xF0
#define UTF8_CONT_MARKER          0x80

// UTF-8 payload masks
#define UTF8_2BYTE_PAYLOAD        0x1F
#define UTF8_3BYTE_PAYLOAD        0x0F
#define UTF8_4BYTE_PAYLOAD        0x07
#define UTF8_CONT_PAYLOAD         0x3F

#define UTF8_4BYTE_MAX_FIRST      0xF4

// Rune -> UTF length thresholds
#define UTF8_2BYTE_MIN            0x00000080
#define UTF8_3BYTE_MIN            0x00000800
#define UTF8_4BYTE_MIN            0x00010000


// -- utilities --

bool is_rune_valid(Rune r) {
    return (r <= RUNE_MAX && (r < RUNE_SURROGATE_START || r > RUNE_SURROGATE_END));
}

u32 utf8_rune_len(Rune r) {
    if (!is_rune_valid(r)) {
        return 0;
    }
    else if (r <= UTF8_ASCII_MAX) {
        return 1;
    }
    else if (r < UTF8_3BYTE_MIN) {
        return 2;
    }
    else if (r < UTF8_4BYTE_MIN) {
        return 3;
    }
    return 4;
}

u32 utf16_rune_len(Rune r) {
    if (!is_rune_valid(r)) {
        return 0;
    }
    else if (r < RUNE_SUPPLEMENTARY_MIN) {
        return 1;
    }
    return 2;
}

u32 utf32_rune_len(Rune r) {
    if (!is_rune_valid(r)) {
        return 0;
    }
    return 1;
}

// -- validation --

bool utf8_validate(const u8* data, u64 len) {
    if (data == NULL || len == 0) {
        return true;
    }

    u64 pos = 0;
    while (pos < len) {
        u64 read_len = 0;
        Rune r = utf8_decode_rune(data + pos, len - pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return false;
        }

        pos += read_len;
    }

    return true;
}

bool utf16_validate(const u16* data, u64 len) {
    if (data == NULL || len == 0) {
        return true;
    }

    u64 pos = 0;
    while (pos < len) {
        u64 read_len = 0;
        Rune r = utf16_decode_rune(data + pos, len - pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return false;
        }

        pos += read_len;
    }

    return true;
}

bool utf32_validate(const u32* data, u64 len) {
    if (data == NULL || len == 0) {
        return true;
    }

    for (u64 i = 0; i < len; i++) {
        if (!is_rune_valid(((const Rune*)data)[i])) {
            return false;
        }
    }

    return true;
}

// -- decoding --

Rune utf8_decode_rune(const u8* data, u64 len, u64* read_len) {
    if (data == NULL || len == 0) {
        if (read_len != NULL) {
            *read_len = 0;
        }
        return RUNE_EOF;
    }

    // ASCII fast path
    if (data[0] <= UTF8_ASCII_MAX) {
        if (read_len != NULL) {
            *read_len = 1;
        }
        return (Rune)data[0];
    }

    // 2-byte sequence
    if ((data[0] & UTF8_2BYTE_MASK) == UTF8_2BYTE_MARKER) {
        if (len >= 2 && (data[1] & UTF8_CONT_MASK) == UTF8_CONT_MARKER) {
            Rune r = ((data[0] & UTF8_2BYTE_PAYLOAD) << 6) |
                      (data[1] & UTF8_CONT_PAYLOAD);
            if (r >= UTF8_2BYTE_MIN) {
                if (read_len != NULL) {
                    *read_len = 2;
                }
                return r;
            }
        }
    }
    // 3-byte sequence
    else if ((data[0] & UTF8_3BYTE_MASK) == UTF8_3BYTE_MARKER) {
        if (len >= 3 &&
           (data[1] & UTF8_CONT_MASK) == UTF8_CONT_MARKER &&
           (data[2] & UTF8_CONT_MASK) == UTF8_CONT_MARKER) {
            Rune r = ((data[0] & UTF8_3BYTE_PAYLOAD) << 12) |
                     ((data[1] & UTF8_CONT_PAYLOAD)  << 6)  |
                      (data[2] & UTF8_CONT_PAYLOAD);

            if (r >= UTF8_3BYTE_MIN && (r < RUNE_SURROGATE_START || r > RUNE_SURROGATE_END)) {
                if (r >= UTF8_2BYTE_MIN) {
                    if (read_len != NULL) {
                        *read_len = 3;
                    }
                    return r;
                }
            }
        }
    }
    // 4-byte sequence
    else if ((data[0] & UTF8_4BYTE_MASK) == UTF8_4BYTE_MARKER &&
              data[0] <= UTF8_4BYTE_MAX_FIRST) {
        if (len >= 4 &&
           (data[1] & UTF8_CONT_MASK) == UTF8_CONT_MARKER &&
           (data[2] & UTF8_CONT_MASK) == UTF8_CONT_MARKER &&
           (data[3] & UTF8_CONT_MASK) == UTF8_CONT_MARKER) {
            Rune r = ((data[0] & UTF8_4BYTE_PAYLOAD) << 18) |
                     ((data[1] & UTF8_CONT_PAYLOAD)  << 12) |
                     ((data[2] & UTF8_CONT_PAYLOAD)  << 6)  |
                      (data[3] & UTF8_CONT_PAYLOAD);

            if (r >= UTF8_4BYTE_MIN && r <= RUNE_MAX) {
                if (r >= UTF8_2BYTE_MIN) {
                    if (read_len != NULL) {
                        *read_len = 4;
                    }
                    return r;
                }
            }
        }
    }

    if (read_len != NULL) {
        *read_len = 1;
    }

    return RUNE_INVALID;
}

Rune utf16_decode_rune(const u16* data, u64 len, u64* read_len) {
    if (data == NULL || len == 0) {
        if (read_len != NULL) {
            *read_len = 0;
        }
        return RUNE_EOF;
    }

    // BMP non-surrogate
    if (data[0] < RUNE_SURROGATE_START || data[0] > RUNE_SURROGATE_END) {
        if (read_len != NULL) {
            *read_len = 1;
        }
        return (Rune)data[0];
    }
    // High surrogate
    if (data[0] >= RUNE_HIGH_SURROGATE_START && data[0] <= RUNE_HIGH_SURROGATE_END) {
        if (len < 2) {
            if (read_len != NULL) {
                *read_len = 1;
            }
            return RUNE_INVALID;
        }
        else if (data[1] < RUNE_LOW_SURROGATE_START || data[1] > RUNE_LOW_SURROGATE_END) {
            if (read_len != NULL) {
                *read_len = 1;
            }
            return RUNE_INVALID;
        }

        Rune r = SURROGATE_OFFSET +
                 (((Rune)(data[0] - HIGH_SURROGATE_BASE)) << 10) +
                 ((Rune)(data[1] - LOW_SURROGATE_BASE));
        if (read_len != NULL) {
            *read_len = 2;
        }
        return r;
    }

    if (read_len != NULL) {
        *read_len = 1;
    }

    return RUNE_INVALID;
}

Rune utf32_decode_rune(const u32* data, u64 len, u64* read_len) {
    if (data == NULL || len == 0) {
        if (read_len != NULL) {
            *read_len = 0;
        }
        return RUNE_EOF;
    }

    Rune r = (Rune)data[0];
    if (read_len != NULL) {
        *read_len = 1;
    }

    if (is_rune_valid(r)) {
        return r;
    }

    return RUNE_INVALID;
}

// -- encoding --

u64 utf8_encode_rune(Rune r, u8* out, u64 out_len) {
    if (!is_rune_valid(r)) {
        return 0;
    }

    // 1-byte sequence
    if (r < UTF8_2BYTE_MIN) {
        if (out != NULL && out_len > 0) {
            out[0] = (u8)r;
        }
        return 1;
    }
    // 2-byte sequence
    else if (r < UTF8_3BYTE_MIN) {
        if (out != NULL && out_len > 1) {
            out[0] = UTF8_2BYTE_MARKER | (u8)(r >> 6);
            out[1] = UTF8_CONT_MARKER  | (u8)(r & UTF8_CONT_PAYLOAD);
        }
        return 2;
    }
    // 3-byte sequence
    else if (r < UTF8_4BYTE_MIN) {
        if (out != NULL && out_len > 2) {
            out[0] = UTF8_3BYTE_MARKER | (u8)(r >> 12);
            out[1] = UTF8_CONT_MARKER  | (u8)((r >> 6) & UTF8_CONT_PAYLOAD);
            out[2] = UTF8_CONT_MARKER  | (u8)(r & UTF8_CONT_PAYLOAD);
        }
        return 3;
    }

    // 4-byte sequence
    if (out != NULL && out_len > 3) {
        out[0] = UTF8_4BYTE_MARKER | (u8)(r >> 18);
        out[1] = UTF8_CONT_MARKER  | (u8)((r >> 12) & UTF8_CONT_PAYLOAD);
        out[2] = UTF8_CONT_MARKER  | (u8)((r >> 6) & UTF8_CONT_PAYLOAD);
        out[3] = UTF8_CONT_MARKER  | (u8)(r & UTF8_CONT_PAYLOAD);
    }

    return 4;
}

u64 utf16_encode_rune(Rune r, u16* out, u64 out_len) {
    if (!is_rune_valid(r)) {
        return 0;
    }

    if (r < RUNE_SUPPLEMENTARY_MIN) {
        if (out != NULL && out_len > 0) {
            out[0] = (u16)r;
        }
        return 1;
    }

    Rune v = r - SURROGATE_OFFSET;
    if (out != NULL && out_len >= 2) {
        out[0] = (u16)(HIGH_SURROGATE_BASE + ((v >> 10) & SURROGATE_CHILD_MASK));
        out[1] = (u16)(LOW_SURROGATE_BASE + (v & SURROGATE_CHILD_MASK));
    }

    return 2;
}

u64 utf32_encode_rune(Rune r, u32* out, u64 out_len) {
    if (!is_rune_valid(r)) {
        return 0;
    }

    if (out != NULL && out_len > 0) {
        out[0] = (u32)r;
    }

    return 1;
}

// -- counting --

u64 utf8_count_runes(const u8* data, u64 len) {
    if (data == NULL || len == 0) {
        return 0;
    }

    u64 pos = 0;
    u64 count = 0;

    while (pos < len) {
        u64 read_len = 0;
        Rune r = utf8_decode_rune(data + pos, len - pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        ++count;
        pos += read_len;
    }

    return count;
}

u64 utf16_count_runes(const u16* data, u64 len) {
    if (data == NULL || len == 0) {
        return 0;
    }

    u64 pos = 0;
    u64 count = 0;

    while (pos < len) {
        u64 read_len = 0;
        Rune r = utf16_decode_rune(data + pos, len - pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        ++count;
        pos += read_len;
    }

    return count;
}

u64 utf32_count_runes(const u32* data, u64 len) {
    if (data == NULL || len == 0) {
        return 0;
    }

    u64 pos = 0;
    u64 count = 0;

    while (pos < len) {
        u64 read_len = 0;
        Rune r = utf32_decode_rune(data + pos, len - pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        ++count;
        pos += read_len;
    }

    return count;
}

// -- convertion --

u64 utf8_to_utf16(const u8* in, u64 in_len, u16* out, u64 out_len) {
    if (in == NULL || in_len == 0) {
        return 0;
    }

    u64 in_pos = 0;
    u64 out_pos = 0;

    while (in_pos < in_len) {
        u64 read_len = 0;
        Rune r = utf8_decode_rune(in + in_pos, in_len - in_pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        in_pos += read_len;

        u64 written_len = 0;
        if (out == NULL || out_len == 0) {
            written_len = utf16_encode_rune(r, NULL, 0);
        }
        else {
            written_len = utf16_encode_rune(r, out + out_pos, out_len - out_pos);
        }

        if (written_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        out_pos += written_len;
    }

    return out_pos;
}

u64 utf8_to_utf32(const u8* in, u64 in_len, u32* out, u64 out_len) {
    if (in == NULL || in_len == 0) {
        return 0;
    }

    u64 in_pos = 0;
    u64 out_pos = 0;

    while (in_pos < in_len) {
        u64 read_len = 0;
        Rune r = utf8_decode_rune(in + in_pos, in_len - in_pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        in_pos += read_len;

        u64 written_len = 0;
        if (out == NULL || out_len == 0) {
            written_len = utf32_encode_rune(r, NULL, 0);
        }
        else {
            written_len = utf32_encode_rune(r, out + out_pos, out_len - out_pos);
        }

        if (written_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        out_pos += written_len;
    }

    return out_pos;
}

u64 utf16_to_utf8(const u16* in, u64 in_len, u8* out, u64 out_len) {
    if (in == NULL || in_len == 0) {
        return 0;
    }

    u64 in_pos = 0;
    u64 out_pos = 0;

    while (in_pos < in_len) {
        u64 read_len = 0;
        Rune r = utf16_decode_rune(in + in_pos, in_len - in_pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        in_pos += read_len;

        u64 written_len = 0;
        if (out == NULL || out_len == 0) {
            written_len = utf8_encode_rune(r, NULL, 0);
        }
        else {
            written_len = utf8_encode_rune(r, out + out_pos, out_len - out_pos);
        }

        if (written_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        out_pos += written_len;
    }

    return out_pos;
}

u64 utf16_to_utf32(const u16* in, u64 in_len, u32* out, u64 out_len) {
    if (in == NULL || in_len == 0) {
        return 0;
    }

    u64 in_pos = 0;
    u64 out_pos = 0;

    while (in_pos < in_len) {
        u64 read_len = 0;
        Rune r = utf16_decode_rune(in + in_pos, in_len - in_pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        in_pos += read_len;

        u64 written_len = 0;
        if (out == NULL || out_len == 0) {
            written_len = utf32_encode_rune(r, NULL, 0);
        }
        else {
            written_len = utf32_encode_rune(r, out + out_pos, out_len - out_pos);
        }

        if (written_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        out_pos += written_len;
    }

    return out_pos;
}

u64 utf32_to_utf8(const u32* in, u64 in_len, u8* out, u64 out_len) {
    if (in == NULL || in_len == 0) {
        return 0;
    }

    u64 in_pos = 0;
    u64 out_pos = 0;

    while (in_pos < in_len) {
        u64 read_len = 0;
        Rune r = utf32_decode_rune(in + in_pos, in_len - in_pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        in_pos += read_len;

        u64 written_len = 0;
        if (out == NULL || out_len == 0) {
            written_len = utf8_encode_rune(r, NULL, 0);
        }
        else {
            written_len = utf8_encode_rune(r, out + out_pos, out_len - out_pos);
        }

        if (written_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        out_pos += written_len;
    }

    return out_pos;
}

u64 utf32_to_utf16(const u32* in, u64 in_len, u16* out, u64 out_len) {
    if (in == NULL || in_len == 0) {
        return 0;
    }

    u64 in_pos = 0;
    u64 out_pos = 0;

    while (in_pos < in_len) {
        u64 read_len = 0;
        Rune r = utf32_decode_rune(in + in_pos, in_len - in_pos, &read_len);

        if (r == RUNE_INVALID || read_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        in_pos += read_len;

        u64 written_len = 0;
        if (out == NULL || out_len == 0) {
            written_len = utf16_encode_rune(r, NULL, 0);
        }
        else {
            written_len = utf16_encode_rune(r, out + out_pos, out_len - out_pos);
        }

        if (written_len == 0) {
            return UNICODE_LEN_INVALID;
        }

        out_pos += written_len;
    }

    return out_pos;
}