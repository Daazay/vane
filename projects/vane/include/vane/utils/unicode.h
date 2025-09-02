#pragma once

#include "vane/utils/defines.h"

typedef i32 Rune;

#define RUNE_INVALID        0xFFFD
#define RUNE_MAX            0x0010FFFF
#define RUNE_EOF            (Rune)(-1)
#define UNICODE_INVALID_LEN -1

// -- utilities --

bool is_rune_valid(Rune r);

// -- decoding --

Rune decode_utf8_rune(const u8* data, u64 len, u32* read_len);

Rune decode_utf16_rune(const u16* data, u64 len, u32* read_len);

Rune decode_utf32_rune(const u32* data, u64 len, u32* read_len);

// -- encoding --

u64 encode_utf8_rune(Rune r, u8* out, u64 out_len);

u64 encode_utf16_rune(Rune r, u16* out, u64 out_len);

u64 encode_utf32_rune(Rune r, u32* out, u64 out_len);

// -- convertion --

u64 convert_utf8_to_utf16(const u8* in, u64 in_len, u16* out, u64 out_len);

u64 convert_utf8_to_utf32(const u8* in, u64 in_len, u32* out, u64 out_len);

u64 convert_utf16_to_utf8(const u16* in, u64 in_len, u8* out, u64 out_len);

u64 convert_utf16_to_utf32(const u16* in, u64 in_len, u32* out, u64 out_len);

u64 convert_utf32_to_utf8(const u32* in, u64 in_len, u8* out, u64 out_len);

u64 convert_utf32_to_utf16(const u32* in, u64 in_len, u16* out, u64 out_len);