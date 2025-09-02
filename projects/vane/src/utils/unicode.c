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
    return (r <= RUNE_MAX) &&
            (r < RUNE_SURROGATE_START || r > RUNE_SURROGATE_END);
}

// -- decoding --

Rune decode_utf8_rune(const u8* data, u64 len, u32* read_len) {
    if (data == NULL || len == 0) {
        if (read_len != NULL) *read_len = 0;
        return RUNE_EOF;
    }

    // ASCII fast path
    if (data[0] <= UTF8_ASCII_MAX) {
        if (read_len != NULL) *read_len = 1;
        return (Rune)data[0];
    }

    // 2-byte sequence
    else if ((data[0] & UTF8_2BYTE_MASK) == UTF8_2BYTE_MARKER) {
        if (len >= 2 && (data[1] & UTF8_CONT_MASK) == UTF8_CONT_MARKER) {
            Rune r = ((data[0] & UTF8_2BYTE_PAYLOAD) << 6) | (data[1] & UTF8_CONT_PAYLOAD);
            if (r >= UTF8_2BYTE_MIN) {
                if (read_len != NULL) *read_len = 2;
                return r;
            }
        }
    }

    // 3-byte sequence
    else if ((data[0] & UTF8_3BYTE_MASK) == UTF8_3BYTE_MARKER) {
        if (len >= 3 &&
            ((data[1] & UTF8_CONT_MASK) == UTF8_CONT_MARKER) &&
            ((data[2] & UTF8_CONT_MASK) == UTF8_CONT_MARKER)) {
            Rune r = ((data[0] & UTF8_3BYTE_PAYLOAD) << 12) |
                     ((data[1] & UTF8_CONT_PAYLOAD) << 6)   |
                      (data[2] & UTF8_CONT_PAYLOAD);
            if (r >= UTF8_3BYTE_MIN && (r < RUNE_SURROGATE_START || r > RUNE_SURROGATE_END)) {
                if (read_len != NULL) *read_len = 3;
                return r;
            }
        }
    }

    // 4-byte sequence
    else if ((data[0] & UTF8_4BYTE_MASK) == UTF8_4BYTE_MARKER) {
        if (len >= 4 &&
            ((data[1] & UTF8_CONT_MASK) == UTF8_CONT_MARKER) &&
            ((data[2] & UTF8_CONT_MASK) == UTF8_CONT_MARKER) &&
            ((data[3] & UTF8_CONT_MASK) == UTF8_CONT_MARKER)) {
            Rune r = ((data[0] & UTF8_4BYTE_PAYLOAD) << 18) |
                     ((data[1] & UTF8_CONT_PAYLOAD) << 12)  |
                     ((data[2] & UTF8_CONT_PAYLOAD) << 6)   |
                      (data[3] & UTF8_CONT_PAYLOAD);
            if (r >= UTF8_4BYTE_MIN && r <= RUNE_MAX) {
                if (read_len != NULL) *read_len = 4;
                return r;
            }
        }
    }

    if (read_len != NULL) *read_len = 1;
    return RUNE_INVALID;
}

Rune decode_utf16_rune(const u16* data, u64 len, u32* read_len) {
    if (data == NULL || len == 0) {
        if (read_len != NULL) *read_len = 0;
        return RUNE_EOF;
    }

    // BMP non-surrogate
    if (data[0] < RUNE_SURROGATE_START || data[0] > RUNE_SURROGATE_END) {
        if (read_len != NULL) *read_len = 1;
        return (Rune)data[0];
    }

    // High surrogate
    else if (data[0] >= RUNE_HIGH_SURROGATE_START && data[0] <= RUNE_HIGH_SURROGATE_END) {
        if (len < 2) {
            if (read_len != NULL) *read_len = 1;
            return RUNE_INVALID;
        }
        else if (data[1] < RUNE_LOW_SURROGATE_START || data[1] > RUNE_LOW_SURROGATE_END) {
            if (read_len != NULL) *read_len = 1;
            return RUNE_INVALID;
        }

        Rune r = SURROGATE_OFFSET + (((Rune)(data[0] - HIGH_SURROGATE_BASE)) << 10) +
                                     ((Rune)(data[1] - LOW_SURROGATE_BASE));
        if (read_len != NULL) *read_len = 2;
        return r;
    }

    if (read_len != NULL) *read_len = 1;
    return RUNE_INVALID;
}

Rune decode_utf32_rune(const u32* data, u64 len, u32* read_len) {
    if (data == NULL || len == 0) {
        if (read_len != NULL) *read_len = 0;
        return RUNE_EOF;
    }

    Rune r = (Rune)data[0];
    if (read_len != NULL) *read_len = 1;

    if (is_rune_valid(r)) {
        return r;
    }

    return RUNE_INVALID;
}

// -- encoding --

u64 encode_utf8_rune(Rune r, u8* out, u64 out_len) {
    if (!is_rune_valid(r)) {
        return 0;
    }

    // 1-byte sequence
    if (r < UTF8_2BYTE_MIN) {
        if (out != NULL && out_len >= 1) {
            out[0] = (u8)r;
        }
        return 1;
    }

    // 2-byte sequence
    else if (r < UTF8_3BYTE_MIN) {
        if (out != NULL && out_len >= 2) {
            out[0] = UTF8_2BYTE_MARKER | (u8)(r >> 6);
            out[1] = UTF8_CONT_MARKER  | (u8)(r & UTF8_CONT_PAYLOAD);
        }
        return 2;
    }

    // 3-byte sequence
    else if (r < UTF8_4BYTE_MIN) {
        if (out != NULL && out_len >= 3) {
            out[0] = UTF8_3BYTE_MARKER | (u8)(r >> 12);
            out[1] = UTF8_CONT_MARKER  | (u8)((r >> 6) & UTF8_CONT_PAYLOAD);
            out[2] = UTF8_CONT_MARKER  | (u8)(r & UTF8_CONT_PAYLOAD);
        }
        return 3;
    }

    // 4-byte sequence
    if (out != NULL && out_len >= 4) {
        out[0] = UTF8_4BYTE_MARKER | (u8)(r >> 18);
        out[1] = UTF8_CONT_MARKER  | (u8)((r >> 12) & UTF8_CONT_PAYLOAD);
        out[2] = UTF8_CONT_MARKER  | (u8)((r >> 6) & UTF8_CONT_PAYLOAD);
        out[3] = UTF8_CONT_MARKER  | (u8)(r & UTF8_CONT_PAYLOAD);
    }

    return 4;
}

u64 encode_utf16_rune(Rune r, u16* out, u64 out_len) {
    if (!is_rune_valid(r)) {
        return 0;
    }

    if (r < RUNE_SUPPLEMENTARY_MIN) {
        if (out != NULL && out_len >= 1) {
            out[0] = (u16)r;
        }
        return 1;
    }

    if (out != NULL && out_len >= 2) {
        Rune v = r - SURROGATE_OFFSET;
        out[0] = (u16)(HIGH_SURROGATE_BASE + ((v >> 10) & SURROGATE_CHILD_MASK));
        out[1] = (u16)(LOW_SURROGATE_BASE + (v & SURROGATE_CHILD_MASK));
    }

    return 2;
}

u64 encode_utf32_rune(Rune r, u32* out, u64 out_len) {
    if (!is_rune_valid(r)) {
        return 0;
    }
    if (out != NULL && out_len >= 1) {
        out[0] = (u32)r;
    }
    return 1;
}

// -- convertion --

#define DEFINE_CONVERT_FUN(IN, OUT) \
u64 convert_utf##IN##_to_utf##OUT(const u##IN* in, u64 in_len, u##OUT* out, u64 out_len) { \
    if (in == NULL || in_len == 0) { \
        return 0; \
    } \
    u64 in_pos = 0; \
    u64 out_pos = 0; \
    if (out == NULL || out_len == 0) { \
        while (in_pos < in_len) { \
            u32 read_len = 0; \
            Rune r = decode_utf##IN##_rune(in + in_pos, in_len - in_pos, &read_len); \
            if (r == RUNE_INVALID || read_len == 0) { \
                return (u64)UNICODE_INVALID_LEN; \
            } \
            in_pos += read_len; \
            out_pos += encode_utf##OUT##_rune(r, NULL, 0); \
        } \
        return out_pos; \
    } \
    while (in_pos < in_len) { \
        u32 read_len = 0; \
        Rune r = decode_utf##IN##_rune(in + in_pos, in_len - in_pos, &read_len); \
        if (r == RUNE_INVALID || read_len == 0) { \
            return (u64)UNICODE_INVALID_LEN; \
        } \
        in_pos += read_len; \
        u64 written_len = encode_utf##OUT##_rune(r, out + out_pos, out_len - out_pos); \
        if (written_len == 0) { \
            return (u64)UNICODE_INVALID_LEN; \
        } \
        out_pos += written_len; \
    } \
    return out_pos; \
}

DEFINE_CONVERT_FUN(8, 16);
DEFINE_CONVERT_FUN(8, 32);
DEFINE_CONVERT_FUN(16, 8);
DEFINE_CONVERT_FUN(16, 32);
DEFINE_CONVERT_FUN(32, 8);
DEFINE_CONVERT_FUN(32, 16);