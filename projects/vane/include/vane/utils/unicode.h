#pragma once

#include "vane/utils/defines.h"

typedef i32 Rune;

#define RUNE_INVALID        0xFFFD
#define RUNE_MAX            0x0010FFFF
#define RUNE_EOF            -1
#define UNICODE_LEN_INVALID ((u64)U64_MAX)

// -- utilities --

bool is_rune_valid(Rune r);

u32 utf8_rune_len(Rune r);

u32 utf16_rune_len(Rune r);

u32 utf32_rune_len(Rune r);

// -- validation --

bool utf8_validate(const u8* data, u64 len);

bool utf16_validate(const u16* data, u64 len);

bool utf32_validate(const u32* data, u64 len);

// -- decoding --

Rune utf8_decode_rune(const u8* data, u64 len, u64* read_len);

Rune utf16_decode_rune(const u16* data, u64 len, u64* read_len);

Rune utf32_decode_rune(const u32* data, u64 len, u64* read_len);

// -- encoding --

u64 utf8_encode_rune(Rune r, u8* out, u64 out_len);

u64 utf16_encode_rune(Rune r, u16* out, u64 out_len);

u64 utf32_encode_rune(Rune r, u32* out, u64 out_len);

// -- counting --

u64 utf8_count_runes(const u8* data, u64 len);

u64 utf16_count_runes(const u16* data, u64 len);

u64 utf32_count_runes(const u32* data, u64 len);

// -- convertion --

u64 utf8_to_utf16(const u8* in, u64 in_len, u16* out, u64 out_len);

u64 utf8_to_utf32(const u8* in, u64 in_len, u32* out, u64 out_len);

u64 utf16_to_utf8(const u16* in, u64 in_len, u8* out, u64 out_len);

u64 utf16_to_utf32(const u16* in, u64 in_len, u32* out, u64 out_len);

u64 utf32_to_utf8(const u32* in, u64 in_len, u8* out, u64 out_len);

u64 utf32_to_utf16(const u32* in, u64 in_len, u16* out, u64 out_len);