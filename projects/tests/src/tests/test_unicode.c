#include <utest/utest.h>

#include <vane/utils/unicode.h>

// -- utilities --

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

// -- decoding --

UTEST(TestUnicode, decode_utf8_rune1) {
    u32 read = 0;
    const u8 data[] = "A";
    Rune r = decode_utf8_rune(data, ARR_SIZE(data) - 1, &read);
    ASSERT_EQ(r, 'A');
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf8_rune2) {
    u32 read = 0;
    const u8 data[] = { 0xC2, 0xA2 };
    Rune r = decode_utf8_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x00A2);
    ASSERT_EQ(read, 2);
}

UTEST(TestUnicode, decode_utf8_rune3) {
    u32 read = 0;
    const u8 data[] = { 0xE2,0x82,0xAC };
    Rune r = decode_utf8_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x20AC);
    ASSERT_EQ(read, 3);
}

UTEST(TestUnicode, decode_utf8_rune4) {
    u32 read = 0;
    const u8 data[] = { 0xF0,0x90,0x8D,0x88 };
    Rune r = decode_utf8_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x10348);
    ASSERT_EQ(read, 4);
}

UTEST(TestUnicode, decode_utf8_rune5) {
    u32 read = 0;
    const u8 data[] = { 0xED,0xA0,0x80 }; // invalid surrogate
    Rune r = decode_utf8_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf8_rune6) {
    u32 read = 0;
    const u8 data[] = { 0 };
    Rune r = decode_utf8_rune(data, 0, &read);
    ASSERT_EQ(r, RUNE_EOF);
    ASSERT_EQ(read, 0);
}

UTEST(TestUnicode, decode_utf8_rune7) {
    u32 read = 0;
    const u8 data[] = { 0xF0,0x9F,0x98,0x80 }; // emoji
    Rune r = decode_utf8_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x1F600);
    ASSERT_EQ(read, 4);
}

UTEST(TestUnicode, decode_utf8_rune8) {
    u32 read = 0;
    const u8 data[] = { 0xC2 }; // truncated
    Rune r = decode_utf8_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}


UTEST(TestUnicode, decode_utf16_rune1) {
    u32 read = 0;
    u16 data[] = { 'A' };
    Rune r = decode_utf16_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 'A');
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf16_rune2) {
    u32 read = 0;
    u16 data[] = { 0xD83D,0xDE00 }; // emoji
    Rune r = decode_utf16_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x1F600);
    ASSERT_EQ(read, 2);
}

UTEST(TestUnicode, decode_utf16_rune3) {
    u32 read = 0;
    u16 data[] = { 0x263A };
    Rune r = decode_utf16_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x263A);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf16_rune4) {
    u32 read = 0;
    u16 data[] = { 0xD800 }; // lone surrogate
    Rune r = decode_utf16_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf16_rune5) {
    u32 read = 0;
    u16 data[] = { 0 };
    Rune r = decode_utf16_rune(data, 0, &read);
    ASSERT_EQ(r, RUNE_EOF);
    ASSERT_EQ(read, 0);
}

UTEST(TestUnicode, decode_utf16_rune6) {
    u32 read = 0;
    u16 data[] = { 0x00A9 };
    Rune r = decode_utf16_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x00A9);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf16_rune7) {
    u32 read = 0;
    u16 data[] = { 0x10400 >> 16,0x10400 & 0xFFFF }; // outside BMP
    Rune r = decode_utf16_rune(data, ARR_SIZE(data), &read);
    // Expect it to handle as surrogate pair? If not, returns 0
    ASSERT_TRUE(r != 0);
}

UTEST(TestUnicode, decode_utf16_rune8) {
    u32 read = 0;
    u16 data[] = { 0xDFFF }; // lone low surrogate
    Rune r = decode_utf16_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf32_rune1) {
    u32 read = 0;
    u32 data[] = { 'A' };
    Rune r = decode_utf32_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 'A');
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf32_rune2) {
    u32 read = 0;
    u32 data[] = { 0x1F600 };
    Rune r = decode_utf32_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x1F600);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf32_rune3) {
    u32 read = 0;
    u32 data[] = { 0x263A };
    Rune r = decode_utf32_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x263A);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf32_rune4) {
    u32 read = 0;
    u32 data[] = { 0x110000 };
    Rune r = decode_utf32_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf32_rune5) {
    u32 read = 0;
    u32 data[] = { 0xD800 };
    Rune r = decode_utf32_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, RUNE_INVALID);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf32_rune6) {
    u32 read = 0;
    u32 data[] = { 0 };
    Rune r = decode_utf32_rune(data, 0, &read);
    ASSERT_EQ(r, RUNE_EOF);
    ASSERT_EQ(read, 0);
}

UTEST(TestUnicode, decode_utf32_rune7) {
    u32 read = 0;
    u32 data[] = { 0x10FFFF };
    Rune r = decode_utf32_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x10FFFF);
    ASSERT_EQ(read, 1);
}

UTEST(TestUnicode, decode_utf32_rune8) {
    u32 read = 0;
    u32 data[] = { 0x30FB };
    Rune r = decode_utf32_rune(data, ARR_SIZE(data), &read);
    ASSERT_EQ(r, 0x30FB);
    ASSERT_EQ(read, 1);
}

// -- encoding --

UTEST(TestUnicode, encode_utf8_rune1) {
    u8 out[4] = { 0 };
    ASSERT_EQ(encode_utf8_rune('A', out, 4), 1);
}

UTEST(TestUnicode, encode_utf8_rune2) {
    u8 out[4] = { 0 };
    ASSERT_EQ(encode_utf8_rune(0x00A2, out, 4), 2);
}

UTEST(TestUnicode, encode_utf8_rune3) {
    u8 out[4] = { 0 };
    ASSERT_EQ(encode_utf8_rune(0x20AC, out, 4), 3);
}

UTEST(TestUnicode, encode_utf8_rune4) {
    u8 out[4] = { 0 };
    ASSERT_EQ(encode_utf8_rune(0x1F600, out, 4), 4);
}

UTEST(TestUnicode, encode_utf8_rune5) {
    u8 out[4] = { 0 };
    ASSERT_EQ(encode_utf8_rune(0x110000, out, 4), 0);
}

UTEST(TestUnicode, encode_utf8_rune6) {
    u8 out[4] = { 0 };
    ASSERT_EQ(encode_utf8_rune(0, out, 4), 1);
}

UTEST(TestUnicode, encode_utf8_rune7) {
    u8 out[4] = { 0 };
    ASSERT_EQ(encode_utf8_rune(0x263A, out, 4), 3);
}

UTEST(TestUnicode, encode_utf8_rune8) {
    u8 out[1] = { 0 };
    ASSERT_EQ(encode_utf8_rune('A', out, 1), 1);
}

UTEST(TestUnicode, encode_utf16_rune1) {
    u16 out[2] = { 0 };
    ASSERT_EQ(encode_utf16_rune('A', out, 2), 1);
}

UTEST(TestUnicode, encode_utf16_rune2) {
    u16 out[2] = { 0 };
    ASSERT_EQ(encode_utf16_rune(0x00A9, out, 2), 1);
}

UTEST(TestUnicode, encode_utf16_rune3) {
    u16 out[2] = { 0 };
    ASSERT_EQ(encode_utf16_rune(0x1F600, out, 2), 2);
}

UTEST(TestUnicode, encode_utf16_rune4) {
    u16 out[2] = { 0 };
    ASSERT_EQ(encode_utf16_rune(0x10400, out, 2), 2);
}

UTEST(TestUnicode, encode_utf16_rune5) {
    u16 out[2] = { 0 };
    ASSERT_EQ(encode_utf16_rune(0xD800, out, 2), 0);
}

UTEST(TestUnicode, encode_utf16_rune6) {
    u16 out[2] = { 0 };
    ASSERT_EQ(encode_utf16_rune(0, out, 2), 1);
}

UTEST(TestUnicode, encode_utf16_rune7) {
    u16 out[2] = { 0 };
    ASSERT_EQ(encode_utf16_rune(0x263A, out, 2), 1);
}

UTEST(TestUnicode, encode_utf16_rune8) {
    u16 out[1] = { 0 };
    ASSERT_EQ(encode_utf16_rune('A', out, 1), 1);
}

UTEST(TestUnicode, encode_utf32_rune1) {
    u32 out[1] = { 0 };
    ASSERT_EQ(encode_utf32_rune('A', out, 1), 1);
}

UTEST(TestUnicode, encode_utf32_rune2) {
    u32 out[1] = { 0 };
    ASSERT_EQ(encode_utf32_rune(0x00A9, out, 1), 1);
}

UTEST(TestUnicode, encode_utf32_rune3) {
    u32 out[1] = { 0 };
    ASSERT_EQ(encode_utf32_rune(0x1F600, out, 1), 1);
}

UTEST(TestUnicode, encode_utf32_rune4) {
    u32 out[1] = { 0 };
    ASSERT_EQ(encode_utf32_rune(0x10FFFF, out, 1), 1);
}

UTEST(TestUnicode, encode_utf32_rune5) {
    u32 out[1] = { 0 };
    ASSERT_EQ(encode_utf32_rune(0x110000, out, 1), 0);
}

UTEST(TestUnicode, encode_utf32_rune6) {
    u32 out[1] = { 0 };
    ASSERT_EQ(encode_utf32_rune(0, out, 1), 1);
}

UTEST(TestUnicode, encode_utf32_rune7) {
    u32 out[1] = { 0 };
    ASSERT_EQ(encode_utf32_rune(0x263A, out, 1), 1);
}

UTEST(TestUnicode, encode_utf32_rune8) {
    u32 out = 0;
    ASSERT_EQ(encode_utf32_rune('A', &out, 1), 1);
}

// -- convertion --

UTEST(TestUnicode, convert_utf8_to_utf16_1) {
    u8 in[] = "A"; u16 out[2];
    ASSERT_EQ(convert_utf8_to_utf16(in, ARR_SIZE(in) - 1, out, 2), 1);
}

UTEST(TestUnicode, convert_utf8_to_utf16_2) {
    u8 in[] = { 0xC2,0xA2 }; u16 out[2];
    ASSERT_EQ(convert_utf8_to_utf16(in, ARR_SIZE(in), out, 2), 1);
}

UTEST(TestUnicode, convert_utf8_to_utf16_3) {
    u8 in[] = { 0xF0,0x9F,0x98,0x80 }; u16 out[2];
    ASSERT_EQ(convert_utf8_to_utf16(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, convert_utf8_to_utf16_4) {
    u8 in[] = ""; u16 out[1];
    ASSERT_EQ(convert_utf8_to_utf16(in, 0, out, 1), 0);
}

UTEST(TestUnicode, convert_utf8_to_utf16_5) {
    u8 in[] = { 0xED,0xA0,0x80 }; u16 out[2];
    ASSERT_EQ(convert_utf8_to_utf16(in, ARR_SIZE(in), out, 2), UNICODE_INVALID_LEN);
}

UTEST(TestUnicode, convert_utf8_to_utf16_6) {
    u8 in[] = "AB"; u16 out[2];
    ASSERT_EQ(convert_utf8_to_utf16(in, ARR_SIZE(in) - 1, out, 2), 2);
}

UTEST(TestUnicode, convert_utf8_to_utf16_7) {
    u8 in[] = { 0xE2,0x82,0xAC }; u16 out[2];
    ASSERT_EQ(convert_utf8_to_utf16(in, ARR_SIZE(in), out, 2), 1);
}

UTEST(TestUnicode, convert_utf8_to_utf16_8) {
    u8 in[] = { 0xC2,0xA2,0xF0,0x9F,0x98,0x80 }; u16 out[3];
    ASSERT_EQ(convert_utf8_to_utf16(in, ARR_SIZE(in), out, 3), 3);
}

UTEST(TestUnicode, convert_utf8_to_utf32_1) {
    u8 in[] = "A"; u32 out[1];
    ASSERT_EQ(convert_utf8_to_utf32(in, ARR_SIZE(in) - 1, out, 1), 1);
}

UTEST(TestUnicode, convert_utf8_to_utf32_2) {
    u8 in[] = { 0xC2,0xA2 }; u32 out[1];
    ASSERT_EQ(convert_utf8_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf8_to_utf32_3) {
    u8 in[] = { 0xF0,0x9F,0x98,0x80 }; u32 out[1];
    ASSERT_EQ(convert_utf8_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf8_to_utf32_4) {
    u8 in[] = ""; u32 out[1];
    ASSERT_EQ(convert_utf8_to_utf32(in, 0, out, 1), 0);
}

UTEST(TestUnicode, convert_utf8_to_utf32_5) {
    u8 in[] = { 0xED,0xA0,0x80 }; u32 out[1];
    ASSERT_EQ(convert_utf8_to_utf32(in, ARR_SIZE(in), out, 1), UNICODE_INVALID_LEN);
}

UTEST(TestUnicode, convert_utf8_to_utf32_6) {
    u8 in[] = "AB"; u32 out[2];
    ASSERT_EQ(convert_utf8_to_utf32(in, ARR_SIZE(in) - 1, out, 2), 2);
}

UTEST(TestUnicode, convert_utf8_to_utf32_7) {
    u8 in[] = { 0xE2,0x82,0xAC }; u32 out[1];
    ASSERT_EQ(convert_utf8_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf8_to_utf32_8) {
    u8 in[] = { 0xC2,0xA2,0xF0,0x9F,0x98,0x80 }; u32 out[2];
    ASSERT_EQ(convert_utf8_to_utf32(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, convert_utf16_to_utf8_1) {
    u16 in[] = { 'A' }; u8 out[2];
    ASSERT_EQ(convert_utf16_to_utf8(in, ARR_SIZE(in), out, 2), 1);
}

UTEST(TestUnicode, convert_utf16_to_utf8_2) {
    u16 in[] = { 0x00A2 }; u8 out[2];
    ASSERT_EQ(convert_utf16_to_utf8(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, convert_utf16_to_utf8_3) {
    u16 in[] = { 0x263A }; u8 out[3];
    ASSERT_EQ(convert_utf16_to_utf8(in, ARR_SIZE(in), out, 3), 3);
}

UTEST(TestUnicode, convert_utf16_to_utf8_4) {
    u16 in[] = { 0xD83D,0xDE00 }; u8 out[4];
    ASSERT_EQ(convert_utf16_to_utf8(in, ARR_SIZE(in), out, 4), 4);
}

UTEST(TestUnicode, convert_utf16_to_utf8_5) {
    u16 in[] = { 0xD800 }; u8 out[2];
    ASSERT_EQ(convert_utf16_to_utf8(in, ARR_SIZE(in), out, 2), UNICODE_INVALID_LEN);
}

UTEST(TestUnicode, convert_utf16_to_utf8_6) {
    u16 in[] = { 0 }; u8 out[1];
    ASSERT_EQ(convert_utf16_to_utf8(in, 0, out, 1), 0);
}

UTEST(TestUnicode, convert_utf16_to_utf8_7) {
    u16 in[] = { 0x00A9,0x263A }; u8 out[5];
    ASSERT_EQ(convert_utf16_to_utf8(in, ARR_SIZE(in), out, 5), 5);
}

UTEST(TestUnicode, convert_utf16_to_utf8_8) {
    u16 in[] = { 0xD83D,0xDE00,'A' }; u8 out[5];
    ASSERT_EQ(convert_utf16_to_utf8(in, ARR_SIZE(in), out, 5), 5);
}

UTEST(TestUnicode, convert_utf16_to_utf32_1) {
    u16 in[] = { 'A' }; u32 out[1];
    ASSERT_EQ(convert_utf16_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf16_to_utf32_2) {
    u16 in[] = { 0x00A2 }; u32 out[1];
    ASSERT_EQ(convert_utf16_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf16_to_utf32_3) {
    u16 in[] = { 0x263A }; u32 out[1];
    ASSERT_EQ(convert_utf16_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf16_to_utf32_4) {
    u16 in[] = { 0xD83D,0xDE00 }; u32 out[1];
    ASSERT_EQ(convert_utf16_to_utf32(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf16_to_utf32_5) {
    u16 in[] = { 0xD800 }; u32 out[1];
    ASSERT_EQ(convert_utf16_to_utf32(in, ARR_SIZE(in), out, 1), UNICODE_INVALID_LEN);
}

UTEST(TestUnicode, convert_utf16_to_utf32_6) {
    u16 in[] = { 0 }; u32 out[1];
    ASSERT_EQ(convert_utf16_to_utf32(in, 0, out, 1), 0);
}

UTEST(TestUnicode, convert_utf16_to_utf32_7) {
    u16 in[] = { 0x00A9,0x263A }; u32 out[2];
    ASSERT_EQ(convert_utf16_to_utf32(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, convert_utf16_to_utf32_8) {
    u16 in[] = { 0xD83D,0xDE00,'A' }; u32 out[2];
    ASSERT_EQ(convert_utf16_to_utf32(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, convert_utf32_to_utf8_1) {
    u32 in[] = { 'A' }; u8 out[2];
    ASSERT_EQ(convert_utf32_to_utf8(in, ARR_SIZE(in), out, 2), 1);
}

UTEST(TestUnicode, convert_utf32_to_utf8_2) {
    u32 in[] = { 0x00A2 }; u8 out[2];
    ASSERT_EQ(convert_utf32_to_utf8(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, convert_utf32_to_utf8_3) {
    u32 in[] = { 0x263A }; u8 out[3];
    ASSERT_EQ(convert_utf32_to_utf8(in, ARR_SIZE(in), out, 3), 3);
}

UTEST(TestUnicode, convert_utf32_to_utf8_4) {
    u32 in[] = { 0x1F600 }; u8 out[4];
    ASSERT_EQ(convert_utf32_to_utf8(in, ARR_SIZE(in), out, 4), 4);
}

UTEST(TestUnicode, convert_utf32_to_utf8_5) {
    u32 in[] = { 0x110000 }; u8 out[4];
    ASSERT_EQ(convert_utf32_to_utf8(in, ARR_SIZE(in), out, 4), UNICODE_INVALID_LEN);
}

UTEST(TestUnicode, convert_utf32_to_utf8_6) {
    u32 in[] = { 0 }; u8 out[1];
    ASSERT_EQ(convert_utf32_to_utf8(in, 0, out, 1), 0);
}

UTEST(TestUnicode, convert_utf32_to_utf8_7) {
    u32 in[] = { 0x00A9,0x263A }; u8 out[6];
    ASSERT_EQ(convert_utf32_to_utf8(in, ARR_SIZE(in), out, 6), 2 + 3);
}

UTEST(TestUnicode, convert_utf32_to_utf8_8) {
    u32 in[] = { 0x1F600,'A' }; u8 out[5];
    ASSERT_EQ(convert_utf32_to_utf8(in, ARR_SIZE(in), out, 5), 4 + 1);
}

UTEST(TestUnicode, convert_utf32_to_utf16_1) {
    u32 in[] = { 'A' }; u16 out[1];
    ASSERT_EQ(convert_utf32_to_utf16(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf32_to_utf16_2) {
    u32 in[] = { 0x00A2 }; u16 out[1];
    ASSERT_EQ(convert_utf32_to_utf16(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf32_to_utf16_3) {
    u32 in[] = { 0x263A }; u16 out[1];
    ASSERT_EQ(convert_utf32_to_utf16(in, ARR_SIZE(in), out, 1), 1);
}

UTEST(TestUnicode, convert_utf32_to_utf16_4) {
    u32 in[] = { 0x1F600 }; u16 out[2];
    ASSERT_EQ(convert_utf32_to_utf16(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, convert_utf32_to_utf16_5) {
    u32 in[] = { 0x110000 }; u16 out[2];
    ASSERT_EQ(convert_utf32_to_utf16(in, ARR_SIZE(in), out, 2), UNICODE_INVALID_LEN);
}

UTEST(TestUnicode, convert_utf32_to_utf16_6) {
    u32 in[] = { 0 }; u16 out[1];
    ASSERT_EQ(convert_utf32_to_utf16(in, 0, out, 1), 0);
}

UTEST(TestUnicode, convert_utf32_to_utf16_7) {
    u32 in[] = { 0x00A9,0x263A }; u16 out[2];
    ASSERT_EQ(convert_utf32_to_utf16(in, ARR_SIZE(in), out, 2), 2);
}

UTEST(TestUnicode, convert_utf32_to_utf16_8) {
    u32 in[] = { 0x1F600,'A' }; u16 out[3];
    ASSERT_EQ(convert_utf32_to_utf16(in, ARR_SIZE(in), out, 3), 2 + 1);
}