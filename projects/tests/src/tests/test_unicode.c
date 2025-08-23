#include <utest/utest.h>

#include <vane/utils/unicode.h>

// -- utilities:is_rune_valid --

UTEST(TestUnicode, is_rune_valid1) {
    ASSERT_TRUE(is_rune_valid('A'));
}

UTEST(TestUnicode, is_rune_valid2) {
    ASSERT_TRUE(is_rune_valid(0x10FFFF));
}

UTEST(TestUnicode, is_rune_valid3) {
    ASSERT_TRUE(is_rune_valid(0x00FFFD));
}

UTEST(TestUnicode, is_rune_valid4) {
    ASSERT_TRUE(is_rune_valid(-1));
}

UTEST(TestUnicode, is_rune_valid5) {
    ASSERT_FALSE(is_rune_valid(0x110000));
}

UTEST(TestUnicode, is_rune_valid6) {
    ASSERT_FALSE(is_rune_valid(0xD800));
}

UTEST(TestUnicode, is_rune_valid7) {
    ASSERT_FALSE(is_rune_valid(0xDFFF));
}

UTEST(TestUnicode, is_rune_valid8) {
    ASSERT_FALSE(is_rune_valid(0xD900));
}

// -- utilities:utf8_rune_len --

UTEST(TestUnicode, utf8_rune_len1) {
    ASSERT_EQ(1, utf8_rune_len('A'));
}

UTEST(TestUnicode, utf8_rune_len2) {
    ASSERT_EQ(2, utf8_rune_len(0x00A2)); // Cent sign
}

UTEST(TestUnicode, utf8_rune_len3) {
    ASSERT_EQ(3, utf8_rune_len(0x20AC)); // Euro sign
}

UTEST(TestUnicode, utf8_rune_len4) {
    ASSERT_EQ(4, utf8_rune_len(0x10348)); // Gothic letter
}

UTEST(TestUnicode, utf8_rune_len5) {
    ASSERT_EQ(2, utf8_rune_len(0x00E9));  // Accent aigu
}

UTEST(TestUnicode, utf8_rune_len6) {
    ASSERT_EQ(3, utf8_rune_len(0x20AC));  // Euro sign
}

UTEST(TestUnicode, utf8_rune_len7) {
    ASSERT_EQ(1, utf8_rune_len('1'));  // ASCII digit
}

UTEST(TestUnicode, utf8_rune_len8) {
    ASSERT_EQ(3, utf8_rune_len(0x30FB));  // Katakana middle dot
}

// -- utilities:utf16_rune_len --

UTEST(TestUnicode, utf16_rune_len1) {
    ASSERT_EQ(1, utf16_rune_len('A'));  // ASCII
}

UTEST(TestUnicode, utf16_rune_len2) {
    ASSERT_EQ(2, utf16_rune_len(0x1F600));  // Emoji
}

UTEST(TestUnicode, utf16_rune_len3) {
    ASSERT_EQ(0, utf16_rune_len(0xD800));  // Invalid surrogate
}

UTEST(TestUnicode, utf16_rune_len4) {
    ASSERT_EQ(1, utf16_rune_len(0x00A9));  // Copyright sign
}

UTEST(TestUnicode, utf16_rune_len5) {
    ASSERT_EQ(0, utf16_rune_len(0xD83D));  // First surrogate part
}

UTEST(TestUnicode, utf16_rune_len6) {
    ASSERT_EQ(1, utf16_rune_len(0x263A));  // Smiley face
}

UTEST(TestUnicode, utf16_rune_len7) {
    ASSERT_EQ(2, utf16_rune_len(0x10400));  // Symbol outside basic dim
}

UTEST(TestUnicode, utf16_rune_len8) {
    ASSERT_EQ(0, utf16_rune_len(0xDBFF));  // Invalid surrogate
}

// -- utilities:utf32_rune_len --

UTEST(TestUnicode, utf32_rune_len1) {
    ASSERT_EQ(1, utf32_rune_len('A')); // ASCII
}

UTEST(TestUnicode, utf32_rune_len2) {
    ASSERT_EQ(0, utf32_rune_len(0x110000)); // Outside valid range
}

UTEST(TestUnicode, utf32_rune_len3) {
    ASSERT_EQ(1, utf32_rune_len(0x00A9)); // copyright
}

UTEST(TestUnicode, utf32_rune_len4) {
    ASSERT_EQ(1, utf32_rune_len(0x263A)); // Emoji smile
}

UTEST(TestUnicode, utf32_rune_len5) {
    ASSERT_EQ(1, utf32_rune_len(0x1F600)); // Emoji
}

UTEST(TestUnicode, utf32_rune_len6) {
    ASSERT_EQ(1, utf32_rune_len(0x10348)); // Gothic letter
}

UTEST(TestUnicode, utf32_rune_len7) {
    ASSERT_EQ(0, utf32_rune_len(0xD800)); // Invalid surrogate
}

UTEST(TestUnicode, utf32_rune_len8) {
    ASSERT_EQ(1, utf32_rune_len(0x30FB)); // Katakana middle dot
}

// -- validation:utf8_validate --

UTEST(TestUnicode, utf8_validate1) {
    const u8 data[] = "hello";
    ASSERT_TRUE(utf8_validate(data, ARR_SIZE(data) - 1));
}

UTEST(TestUnicode, utf8_validate2) {
    const u8 data[] = { 0xC2, 0xA2 };
    ASSERT_TRUE(utf8_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf8_validate3) {
    const u8 data[] = { 0xC2 };
    ASSERT_FALSE(utf8_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf8_validate4) {
    const u8 data[] = { 0xED, 0xA0, 0x80 };
    ASSERT_FALSE(utf8_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf8_validate5) {
    const u8 data[] = { 0 };
    ASSERT_TRUE(utf8_validate(data, 0));
}

UTEST(TestUnicode, utf8_validate6) {
    const u8 data[] = { 0xF0,0x9F,0x98,0x80 }; // emoji
    ASSERT_TRUE(utf8_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf8_validate7) {
    const u8 data[] = { 0xC0,0xAF }; // overlong '/'
    ASSERT_FALSE(utf8_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf8_validate8) {
    const u8 data[] = { 0x41,0xED,0xA0,0x80,0x42 }; // mixed valid+invalid
    ASSERT_FALSE(utf8_validate(data, ARR_SIZE(data)));
}

// -- validation:utf16_validate --

UTEST(TestUnicode, utf16_validate1) {
    u16 data[] = { 'A','B','C' };
    ASSERT_TRUE(utf16_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf16_validate2) {
    u16 data[] = { 0xD83D,0xDE00 };
    ASSERT_TRUE(utf16_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf16_validate3) {
    u16 data[] = { 0xD800 };
    ASSERT_FALSE(utf16_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf16_validate4) {
    u16 data[] = { 0xDC00 };
    ASSERT_FALSE(utf16_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf16_validate5) {
    u16 data[] = { 0x263A };
    ASSERT_TRUE(utf16_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf16_validate6) {
    u16 data[] = { 0 };
    ASSERT_TRUE(utf16_validate(data, 0));
}

UTEST(TestUnicode, utf16_validate7) {
    u16 data[] = { 'H',0xD800,'i' };
    ASSERT_FALSE(utf16_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf16_validate8) {
    u16 data[] = { 0x0041,0x00A9,0x263A };
    ASSERT_TRUE(utf16_validate(data, ARR_SIZE(data)));
}

// -- validation:utf32_validate --

UTEST(TestUnicode, utf32_validate1) {
    u32 data[] = { 'A','B','C' };
    ASSERT_TRUE(utf32_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf32_validate2) {
    u32 data[] = { 0x1F600 };
    ASSERT_TRUE(utf32_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf32_validate3) {
    u32 data[] = { 0xD800 };
    ASSERT_FALSE(utf32_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf32_validate4) {
    u32 data[] = { 0x110000 };
    ASSERT_FALSE(utf32_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf32_validate5) {
    u32 data[] = { 0 };
    ASSERT_TRUE(utf32_validate(data, 0));
}

UTEST(TestUnicode, utf32_validate6) {
    u32 data[] = { 0x263A,0x1F600 };
    ASSERT_TRUE(utf32_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf32_validate7) {
    u32 data[] = { 0xFFFFFF };
    ASSERT_FALSE(utf32_validate(data, ARR_SIZE(data)));
}

UTEST(TestUnicode, utf32_validate8) {
    u32 data[] = { 0x0041,0x00A9,0x263A };
    ASSERT_TRUE(utf32_validate(data, ARR_SIZE(data)));
}

// -- decoding:utf8_decode_rune --

UTEST(TestUnicode, utf8_decode_rune1) {
    u64 read;
    const u8 data[] = "A";
    Rune r = utf8_decode_rune(data, ARR_SIZE(data) - 1, &read);
    ASSERT_EQ(r, 'A');
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf8_decode_rune2) {
    u64 read;
    const u8 data[] = { 0xC2, 0xA2 };
    Rune r = utf8_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x00A2);
    ASSERT_EQ(read, 2);
}

UTEST(TestUnicode, utf8_decode_rune3) {
    u64 read;
    const u8 data[] = { 0xE2,0x82,0xAC };
    Rune r = utf8_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x20AC);
    ASSERT_EQ(read, 3);
}

UTEST(TestUnicode, utf8_decode_rune4) {
    u64 read;
    const u8 data[] = { 0xF0,0x90,0x8D,0x88 };
    Rune r = utf8_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x10348);
    ASSERT_EQ(read, 4);
}

UTEST(TestUnicode, utf8_decode_rune5) {
    u64 read;
    const u8 data[] = { 0xED,0xA0,0x80 }; // invalid surrogate
    Rune r = utf8_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf8_decode_rune6) {
    u64 read;
    const u8 data[] = { 0 };
    Rune r = utf8_decode_rune(data, 0, &read);
    ASSERT_EQ(r, RUNE_EOF);
    ASSERT_EQ(read, 0);
}

UTEST(TestUnicode, utf8_decode_rune7) {
    u64 read;
    const u8 data[] = { 0xF0,0x9F,0x98,0x80 }; // emoji
    Rune r = utf8_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x1F600);
    ASSERT_EQ(read, 4);
}

UTEST(TestUnicode, utf8_decode_rune8) {
    u64 read;
    const u8 data[] = { 0xC2 }; // truncated
    Rune r = utf8_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

// -- decoding:utf16_decode_rune --

UTEST(TestUnicode, utf16_decode_rune1) {
    u64 read;
    u16 data[] = { 'A' };
    Rune r = utf16_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 'A');
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf16_decode_rune2) {
    u64 read;
    u16 data[] = { 0xD83D,0xDE00 }; // emoji
    Rune r = utf16_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x1F600);
    ASSERT_EQ(read, 2);
}

UTEST(TestUnicode, utf16_decode_rune3) {
    u64 read;
    u16 data[] = { 0x263A };
    Rune r = utf16_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x263A);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf16_decode_rune4) {
    u64 read;
    u16 data[] = { 0xD800 }; // lone surrogate
    Rune r = utf16_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf16_decode_rune5) {
    u64 read;
    u16 data[] = { 0 };
    Rune r = utf16_decode_rune(data, 0, &read);
    ASSERT_EQ(r, RUNE_EOF);
    ASSERT_EQ(read, 0);
}

UTEST(TestUnicode, utf16_decode_rune6) {
    u64 read;
    u16 data[] = { 0x00A9 };
    Rune r = utf16_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x00A9);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf16_decode_rune7) {
    u64 read;
    u16 data[] = { 0x10400 >> 16,0x10400 & 0xFFFF }; // outside BMP
    Rune r = utf16_decode_rune(data, ARR_SIZE(data), &read);
    // Expect it to handle as surrogate pair? If not, returns 0
    ASSERT_TRUE(r != 0);
}

UTEST(TestUnicode, utf16_decode_rune8) {
    u64 read;
    u16 data[] = { 0xDFFF }; // lone low surrogate
    Rune r = utf16_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

// -- decoding:utf32_decode_rune --

UTEST(TestUnicode, utf32_decode_rune1) {
    u64 read;
    u32 data[] = { 'A' };
    Rune r = utf32_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 'A');
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf32_decode_rune2) {
    u64 read;
    u32 data[] = { 0x1F600 };
    Rune r = utf32_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x1F600);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf32_decode_rune3) {
    u64 read;
    u32 data[] = { 0x263A };
    Rune r = utf32_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x263A);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf32_decode_rune4) {
    u64 read;
    u32 data[] = { 0x110000 };
    Rune r = utf32_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf32_decode_rune5) {
    u64 read;
    u32 data[] = { 0xD800 };
    Rune r = utf32_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf32_decode_rune6) {
    u64 read;
    u32 data[] = { 0 };
    Rune r = utf32_decode_rune(data, 0, &read);
    ASSERT_EQ(r, RUNE_EOF);
    ASSERT_EQ(read, 0);
}

UTEST(TestUnicode, utf32_decode_rune7) {
    u64 read;
    u32 data[] = { 0x10FFFF };
    Rune r = utf32_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x10FFFF);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, utf32_decode_rune8) {
    u64 read;
    u32 data[] = { 0x30FB };
    Rune r = utf32_decode_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x30FB);
    ASSERT_EQ(read, 1);
}

// -- encoding:utf8_encode_rune --

UTEST(TestUnicode, utf8_encode_rune1) {
    u8 out[4] = { 0 };
    ASSERT_EQ(utf8_encode_rune('A', out, 4), 1);
}

UTEST(TestUnicode, utf8_encode_rune2) {
    u8 out[4] = { 0 };
    ASSERT_EQ(utf8_encode_rune(0x00A2, out, 4), 2);
}

UTEST(TestUnicode, utf8_encode_rune3) {
    u8 out[4] = { 0 };
    ASSERT_EQ(utf8_encode_rune(0x20AC, out, 4), 3);
}

UTEST(TestUnicode, utf8_encode_rune4) {
    u8 out[4] = { 0 };
    ASSERT_EQ(utf8_encode_rune(0x1F600, out, 4), 4);
}

UTEST(TestUnicode, utf8_encode_rune5) {
    u8 out[4] = { 0 };
    ASSERT_EQ(utf8_encode_rune(0x110000, out, 4), 0);
}

UTEST(TestUnicode, utf8_encode_rune6) {
    u8 out[4] = { 0 };
    ASSERT_EQ(utf8_encode_rune(0, out, 4), 1);
}

UTEST(TestUnicode, utf8_encode_rune7) {
    u8 out[4] = { 0 };
    ASSERT_EQ(utf8_encode_rune(0x263A, out, 4), 3);
}

UTEST(TestUnicode, utf8_encode_rune8) {
    u8 out[1] = { 0 };
    ASSERT_EQ(utf8_encode_rune('A', out, 1), 1);
}


// -- encoding:utf16_encode_rune --

UTEST(TestUnicode, utf16_encode_rune1) {
    u16 out[2] = { 0 };
    ASSERT_EQ(utf16_encode_rune('A', out, 2), 1);
}

UTEST(TestUnicode, utf16_encode_rune2) {
    u16 out[2] = { 0 };
    ASSERT_EQ(utf16_encode_rune(0x00A9, out, 2), 1);
}

UTEST(TestUnicode, utf16_encode_rune3) {
    u16 out[2] = { 0 };
    ASSERT_EQ(utf16_encode_rune(0x1F600, out, 2), 2);
}

UTEST(TestUnicode, utf16_encode_rune4) {
    u16 out[2] = { 0 };
    ASSERT_EQ(utf16_encode_rune(0x10400, out, 2), 2);
}

UTEST(TestUnicode, utf16_encode_rune5) {
    u16 out[2] = { 0 };
    ASSERT_EQ(utf16_encode_rune(0xD800, out, 2), 0);
}

UTEST(TestUnicode, utf16_encode_rune6) {
    u16 out[2] = { 0 };
    ASSERT_EQ(utf16_encode_rune(0, out, 2), 1);
}

UTEST(TestUnicode, utf16_encode_rune7) {
    u16 out[2] = { 0 };
    ASSERT_EQ(utf16_encode_rune(0x263A, out, 2), 1);
}

UTEST(TestUnicode, utf16_encode_rune8) {
    u16 out[1] = { 0 };
    ASSERT_EQ(utf16_encode_rune('A', out, 1), 1);
}

// -- encoding:utf32_encode_rune --

UTEST(TestUnicode, utf32_encode_rune1) {
    u32 out[1] = { 0 };
    ASSERT_EQ(utf32_encode_rune('A', out, 1), 1);
}

UTEST(TestUnicode, utf32_encode_rune2) {
    u32 out[1] = { 0 };
    ASSERT_EQ(utf32_encode_rune(0x00A9, out, 1), 1);
}

UTEST(TestUnicode, utf32_encode_rune3) {
    u32 out[1] = { 0 };
    ASSERT_EQ(utf32_encode_rune(0x1F600, out, 1), 1);
}

UTEST(TestUnicode, utf32_encode_rune4) {
    u32 out[1] = { 0 };
    ASSERT_EQ(utf32_encode_rune(0x10FFFF, out, 1), 1);
}

UTEST(TestUnicode, utf32_encode_rune5) {
    u32 out[1] = { 0 };
    ASSERT_EQ(utf32_encode_rune(0x110000, out, 1), 0);
}

UTEST(TestUnicode, utf32_encode_rune6) {
    u32 out[1] = { 0 };
    ASSERT_EQ(utf32_encode_rune(0, out, 1), 1);
}

UTEST(TestUnicode, utf32_encode_rune7) {
    u32 out[1] = { 0 };
    ASSERT_EQ(utf32_encode_rune(0x263A, out, 1), 1);
}

UTEST(TestUnicode, utf32_encode_rune8) {
    u32 out = 0;
    ASSERT_EQ(utf32_encode_rune('A', &out, 0), 1);
}

// -- counting:utf8_count_runes --

UTEST(TestUnicode, utf8_count_runes1) {
    const u8 s[] = "";
    ASSERT_EQ(utf8_count_runes(s, 0), 0);
}

UTEST(TestUnicode, utf8_count_runes2) {
    const u8 s[] = "Hello";
    ASSERT_EQ(utf8_count_runes(s, ARR_SIZE(s) - 1), 5);
}

UTEST(TestUnicode, utf8_count_runes3) {
    const u8 s[] = { 0xC2,0xA2,0xE2,0x82,0xAC };
    ASSERT_EQ(utf8_count_runes(s, ARR_SIZE(s)), 2);
}

UTEST(TestUnicode, utf8_count_runes4) {
    const u8 s[] = { 0xF0,0x9F,0x98,0x80 };
    ASSERT_EQ(utf8_count_runes(s, ARR_SIZE(s)), 1);
}

UTEST(TestUnicode, utf8_count_runes5) {
    const u8 s[] = { 0xED,0xA0,0x80 };
    ASSERT_EQ(utf8_count_runes(s, ARR_SIZE(s)), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf8_count_runes6) {
    const u8 s[] = { 0x41,0xC2,0xA2,0xF0,0x9F,0x98,0x80 };
    ASSERT_EQ(utf8_count_runes(s, ARR_SIZE(s)), 3);
}

UTEST(TestUnicode, utf8_count_runes7) {
    const u8 s[] = { 0xE2,0x82,0xAC,0xC2,0xA2 };
    ASSERT_EQ(utf8_count_runes(s, ARR_SIZE(s)), 2);
}

UTEST(TestUnicode, utf8_count_runes8) {
    const u8 s[] = { 0x41 };
    ASSERT_EQ(utf8_count_runes(s, ARR_SIZE(s)), 1);
}

// -- counting:utf16_count_runes --

UTEST(TestUnicode, utf16_count_runes1) {
    u16 s[] = { 0 };
    ASSERT_EQ(utf16_count_runes(s, 0), 0);
}

UTEST(TestUnicode, utf16_count_runes2) {
    u16 s[] = { 'A','B','C' };
    ASSERT_EQ(utf16_count_runes(s, ARR_SIZE(s)), 3);
}

UTEST(TestUnicode, utf16_count_runes3) {
    u16 s[] = { 0xD83D,0xDE00 };
    ASSERT_EQ(utf16_count_runes(s, ARR_SIZE(s)), 1);
}

UTEST(TestUnicode, utf16_count_runes4) {
    u16 s[] = { 0x263A };
    ASSERT_EQ(utf16_count_runes(s, ARR_SIZE(s)), 1);
}

UTEST(TestUnicode, utf16_count_runes5) {
    u16 s[] = { 0xD800 };
    ASSERT_EQ(utf16_count_runes(s, ARR_SIZE(s)), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf16_count_runes6) {
    u16 s[] = { 'A',0xD83D,0xDE00,'B' };
    ASSERT_EQ(utf16_count_runes(s, ARR_SIZE(s)), 3);
}

UTEST(TestUnicode, utf16_count_runes7) {
    u16 s[] = { 0x00A9, 0x263A, 0xD83D, 0xDE00 };
    ASSERT_EQ(utf16_count_runes(s, ARR_SIZE(s)), 3);
}

UTEST(TestUnicode, utf16_count_runes8) {
    u16 s[] = { 'A' };
    ASSERT_EQ(utf16_count_runes(s, ARR_SIZE(s)), 1);
}

// -- counting:utf32_count_runes --

UTEST(TestUnicode, utf32_count_runes1) {
    u32 s[] = { 0 };
    ASSERT_EQ(utf32_count_runes(s, 0), 0);
}

UTEST(TestUnicode, utf32_count_runes2) {
    u32 s[] = { 'A','B','C' };
    ASSERT_EQ(utf32_count_runes(s, ARR_SIZE(s)), 3);
}

UTEST(TestUnicode, utf32_count_runes3) {
    u32 s[] = { 0x1F600 };
    ASSERT_EQ(utf32_count_runes(s, ARR_SIZE(s)), 1);
}

UTEST(TestUnicode, utf32_count_runes4) {
    u32 s[] = { 0x263A };
    ASSERT_EQ(utf32_count_runes(s, ARR_SIZE(s)), 1);
}

UTEST(TestUnicode, utf32_count_runes5) {
    u32 s[] = { 0x110000 };
    ASSERT_EQ(utf32_count_runes(s, ARR_SIZE(s)), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf32_count_runes6) {
    u32 s[] = { 'A',0x1F600,'B' };
    ASSERT_EQ(utf32_count_runes(s, ARR_SIZE(s)), 3);
}

UTEST(TestUnicode, utf32_count_runes7) {
    u32 s[] = { 0x00A9,0x263A,0x1F600 };
    ASSERT_EQ(utf32_count_runes(s, ARR_SIZE(s)), 3);
}

UTEST(TestUnicode, utf32_count_runes8) {
    u32 s[] = { 'A' };
    ASSERT_EQ(utf32_count_runes(s, ARR_SIZE(s)), 1);
}

// -- convertion:utf8_to_utf16 --

UTEST(TestUnicode, utf8_to_utf16_1) {
    u8 in[] = "A"; u16 out[2];
    ASSERT_EQ(utf8_to_utf16(in, ARR_SIZE(in) - 1, out, 2), 1);
}

UTEST(TestUnicode, utf8_to_utf16_2) {
    u8 in[] = { 0xC2,0xA2 }; u16 out[2];
    ASSERT_EQ(utf8_to_utf16(in, ARR_SIZE(in), out, 2), 1);
}

UTEST(TestUnicode, utf8_to_utf16_3) {
    u8 in[] = { 0xF0,0x9F,0x98,0x80 }; u16 out[2];
    ASSERT_EQ(utf8_to_utf16(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, utf8_to_utf16_4) {
    u8 in[] = ""; u16 out[1];
    ASSERT_EQ(utf8_to_utf16(in, 0, out, 1), 0);
}

UTEST(TestUnicode, utf8_to_utf16_5) {
    u8 in[] = { 0xED,0xA0,0x80 }; u16 out[2];
    ASSERT_EQ(utf8_to_utf16(in, ARR_SIZE(in), out, 2), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf8_to_utf16_6) {
    u8 in[] = "AB"; u16 out[2];
    ASSERT_EQ(utf8_to_utf16(in, ARR_SIZE(in) - 1, out, 2), 2);
}

UTEST(TestUnicode, utf8_to_utf16_7) {
    u8 in[] = { 0xE2,0x82,0xAC }; u16 out[2];
    ASSERT_EQ(utf8_to_utf16(in, ARR_SIZE(in), out, 2), 1);
}

UTEST(TestUnicode, utf8_to_utf16_8) {
    u8 in[] = { 0xC2,0xA2,0xF0,0x9F,0x98,0x80 }; u16 out[3];
    ASSERT_EQ(utf8_to_utf16(in, ARR_SIZE(in), out, 3), 3);
}

// -- convertion:utf8_to_utf32 --

UTEST(TestUnicode, utf8_to_utf32_1) {
    u8 in[] = "A"; u32 out[1];
    ASSERT_EQ(utf8_to_utf32(in, ARR_SIZE(in) - 1, out, 1), 1);
}

UTEST(TestUnicode, utf8_to_utf32_2) {
    u8 in[] = { 0xC2,0xA2 }; u32 out[1];
    ASSERT_EQ(utf8_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf8_to_utf32_3) {
    u8 in[] = { 0xF0,0x9F,0x98,0x80 }; u32 out[1];
    ASSERT_EQ(utf8_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf8_to_utf32_4) {
    u8 in[] = ""; u32 out[1];
    ASSERT_EQ(utf8_to_utf32(in, 0, out, 1), 0);
}

UTEST(TestUnicode, utf8_to_utf32_5) {
    u8 in[] = { 0xED,0xA0,0x80 }; u32 out[1];
    ASSERT_EQ(utf8_to_utf32(in, ARR_SIZE(in), out, 1), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf8_to_utf32_6) {
    u8 in[] = "AB"; u32 out[2];
    ASSERT_EQ(utf8_to_utf32(in, ARR_SIZE(in) - 1, out, 2), 2);
}

UTEST(TestUnicode, utf8_to_utf32_7) {
    u8 in[] = { 0xE2,0x82,0xAC }; u32 out[1];
    ASSERT_EQ(utf8_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf8_to_utf32_8) {
    u8 in[] = { 0xC2,0xA2,0xF0,0x9F,0x98,0x80 }; u32 out[2];
    ASSERT_EQ(utf8_to_utf32(in, ARR_SIZE(in), out, 2), 2);
}

// -- convertion:utf16_to_utf8 --

UTEST(TestUnicode, utf16_to_utf8_1) {
    u16 in[] = { 'A' }; u8 out[2];
    ASSERT_EQ(utf16_to_utf8(in, ARR_SIZE(in), out, 2), 1);
}

UTEST(TestUnicode, utf16_to_utf8_2) {
    u16 in[] = { 0x00A2 }; u8 out[2];
    ASSERT_EQ(utf16_to_utf8(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, utf16_to_utf8_3) {
    u16 in[] = { 0x263A }; u8 out[3];
    ASSERT_EQ(utf16_to_utf8(in, ARR_SIZE(in), out, 3), 3);
}

UTEST(TestUnicode, utf16_to_utf8_4) {
    u16 in[] = { 0xD83D,0xDE00 }; u8 out[4];
    ASSERT_EQ(utf16_to_utf8(in, ARR_SIZE(in), out, 4), 4);
}

UTEST(TestUnicode, utf16_to_utf8_5) {
    u16 in[] = { 0xD800 }; u8 out[2];
    ASSERT_EQ(utf16_to_utf8(in, ARR_SIZE(in), out, 2), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf16_to_utf8_6) {
    u16 in[] = { 0 }; u8 out[1];
    ASSERT_EQ(utf16_to_utf8(in, 0, out, 1), 0);
}

UTEST(TestUnicode, utf16_to_utf8_7) {
    u16 in[] = { 0x00A9,0x263A }; u8 out[5];
    ASSERT_EQ(utf16_to_utf8(in, ARR_SIZE(in), out, 5), 5);
}

UTEST(TestUnicode, utf16_to_utf8_8) {
    u16 in[] = { 0xD83D,0xDE00,'A' }; u8 out[5];
    ASSERT_EQ(utf16_to_utf8(in, ARR_SIZE(in), out, 5), 5);
}

// -- convertion:utf16_to_utf32 --

UTEST(TestUnicode, utf16_to_utf32_1) {
    u16 in[] = { 'A' }; u32 out[1];
    ASSERT_EQ(utf16_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf16_to_utf32_2) {
    u16 in[] = { 0x00A2 }; u32 out[1];
    ASSERT_EQ(utf16_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf16_to_utf32_3) {
    u16 in[] = { 0x263A }; u32 out[1];
    ASSERT_EQ(utf16_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf16_to_utf32_4) {
    u16 in[] = { 0xD83D,0xDE00 }; u32 out[1];
    ASSERT_EQ(utf16_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf16_to_utf32_5) {
    u16 in[] = { 0xD800 }; u32 out[1];
    ASSERT_EQ(utf16_to_utf32(in, ARR_SIZE(in), out, 1), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf16_to_utf32_6) {
    u16 in[] = { 0 }; u32 out[1];
    ASSERT_EQ(utf16_to_utf32(in, 0, out, 1), 0);
}

UTEST(TestUnicode, utf16_to_utf32_7) {
    u16 in[] = { 0x00A9,0x263A }; u32 out[2];
    ASSERT_EQ(utf16_to_utf32(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, utf16_to_utf32_8) {
    u16 in[] = { 0xD83D,0xDE00,'A' }; u32 out[2];
    ASSERT_EQ(utf16_to_utf32(in, ARR_SIZE(in), out, 2), 2);
}

// -- convertion:utf32_to_utf8 --

UTEST(TestUnicode, utf32_to_utf8_1) {
    u32 in[] = { 'A' }; u8 out[2];
    ASSERT_EQ(utf32_to_utf8(in, ARR_SIZE(in), out, 2), 1);
}

UTEST(TestUnicode, utf32_to_utf8_2) {
    u32 in[] = { 0x00A2 }; u8 out[2];
    ASSERT_EQ(utf32_to_utf8(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, utf32_to_utf8_3) {
    u32 in[] = { 0x263A }; u8 out[3];
    ASSERT_EQ(utf32_to_utf8(in, ARR_SIZE(in), out, 3), 3);
}

UTEST(TestUnicode, utf32_to_utf8_4) {
    u32 in[] = { 0x1F600 }; u8 out[4];
    ASSERT_EQ(utf32_to_utf8(in, ARR_SIZE(in), out, 4), 4);
}

UTEST(TestUnicode, utf32_to_utf8_5) {
    u32 in[] = { 0x110000 }; u8 out[4];
    ASSERT_EQ(utf32_to_utf8(in, ARR_SIZE(in), out, 4), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf32_to_utf8_6) {
    u32 in[] = { 0 }; u8 out[1];
    ASSERT_EQ(utf32_to_utf8(in, 0, out, 1), 0);
}

UTEST(TestUnicode, utf32_to_utf8_7) {
    u32 in[] = { 0x00A9,0x263A }; u8 out[6];
    ASSERT_EQ(utf32_to_utf8(in, ARR_SIZE(in), out, 6), 2 + 3);
}

UTEST(TestUnicode, utf32_to_utf8_8) {
    u32 in[] = { 0x1F600,'A' }; u8 out[5];
    ASSERT_EQ(utf32_to_utf8(in, ARR_SIZE(in), out, 5), 4 + 1);
}


// -- convertion:utf32_to_utf16 --

UTEST(TestUnicode, utf32_to_utf16_1) {
    u32 in[] = { 'A' }; u16 out[1];
    ASSERT_EQ(utf32_to_utf16(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf32_to_utf16_2) {
    u32 in[] = { 0x00A2 }; u16 out[1];
    ASSERT_EQ(utf32_to_utf16(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf32_to_utf16_3) {
    u32 in[] = { 0x263A }; u16 out[1];
    ASSERT_EQ(utf32_to_utf16(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, utf32_to_utf16_4) {
    u32 in[] = { 0x1F600 }; u16 out[2];
    ASSERT_EQ(utf32_to_utf16(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, utf32_to_utf16_5) {
    u32 in[] = { 0x110000 }; u16 out[2];
    ASSERT_EQ(utf32_to_utf16(in, ARR_SIZE(in), out, 2), UNICODE_LEN_INVALID);
}

UTEST(TestUnicode, utf32_to_utf16_6) {
    u32 in[] = { 0 }; u16 out[1];
    ASSERT_EQ(utf32_to_utf16(in, 0, out, 1), 0);
}

UTEST(TestUnicode, utf32_to_utf16_7) {
    u32 in[] = { 0x00A9,0x263A }; u16 out[2];
    ASSERT_EQ(utf32_to_utf16(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, utf32_to_utf16_8) {
    u32 in[] = { 0x1F600,'A' }; u16 out[3];
    ASSERT_EQ(utf32_to_utf16(in, ARR_SIZE(in), out, 3), 2 + 1);
}