#include <utest/utest.h>

#include <vane/utils/string_view.h>


// -- creation --

UTEST(TestStringView, string_view_from_cstr1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ(NULL, (const char*)sv.data);
    ASSERT_EQ(0, sv.len);
}

UTEST(TestStringView, string_view_from_cstr2) {
    StringView sv = string_view_from_cstr("");
    ASSERT_EQ(NULL, (const char*)sv.data);
    ASSERT_EQ(0, sv.len);
}

UTEST(TestStringView, string_view_from_cstr3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_STREQ("hello", (const char*)sv.data);
    ASSERT_EQ(5, sv.len);
}

// -- comparison:is_string_view_empty --

UTEST(TestStringView, is_string_view_empty1) {
    StringView sv = STRING_VIEW_EMPTY;
    ASSERT_TRUE(is_string_view_empty(sv));
}

UTEST(TestStringView, is_string_view_empty2) {
    StringView sv = string_view_from_cstr("");
    ASSERT_TRUE(is_string_view_empty(sv));
}

UTEST(TestStringView, is_string_view_empty3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(is_string_view_empty(sv));
}

// -- comparison:string_view_eq_cstr --

UTEST(TestStringView, string_view_eq_cstr1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_eq_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_eq_cstr2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_eq_cstr(sv, ""));
}

UTEST(TestStringView, string_view_eq_cstr3) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_eq_cstr(sv, "hello"));
}

UTEST(TestStringView, string_view_eq_cstr4) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_eq_cstr(sv, ""));
}

UTEST(TestStringView, string_view_eq_cstr5) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_TRUE(string_view_eq_cstr(sv, "hello"));
}

// -- comparison:string_view_eq_bytes --

UTEST(TestStringView, string_view_eq_bytes1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_eq_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_eq_bytes2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_eq_bytes(sv, (const u8*)"", 1));
}

UTEST(TestStringView, string_view_eq_bytes3) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_eq_bytes(sv, (const u8*)"hello", 6));
}

UTEST(TestStringView, string_view_eq_bytes4) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_eq_bytes(sv, (const u8*)"", 0));
}

UTEST(TestStringView, string_view_eq_bytes5) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_eq_bytes(sv, (const u8*)"hello", 6));
}

// -- comparison:string_view_eq_sv --

UTEST(TestStringView, string_view_eq_sv1) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_eq_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_eq_sv2) {
    StringView sv1 = string_view_from_cstr("");
    StringView sv2 = string_view_from_cstr("");
    ASSERT_TRUE(string_view_eq_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_eq_sv3) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_eq_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_eq_sv4) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_eq_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_eq_sv5) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("hello");
    ASSERT_TRUE(string_view_eq_sv(sv1, sv2));
}

// -- prefixes:string_view_has_prefix_cstr

UTEST(TestStringView, string_view_has_prefix_cstr1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_has_prefix_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_has_prefix_cstr2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_has_prefix_cstr(sv, "sub"));
}

UTEST(TestStringView, string_view_has_prefix_cstr3) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_prefix_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_has_prefix_cstr4) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_prefix_cstr(sv, "sub"));
}

UTEST(TestStringView, string_view_has_prefix_cstr5) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_prefix_cstr(sv, "syb"));
}

UTEST(TestStringView, string_view_has_prefix_cstr6) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_prefix_cstr(sv, "substring"));
}

UTEST(TestStringView, string_view_has_prefix_cstr7) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_prefix_cstr(sv, "substrings"));
}

// -- prefixes:string_view_has_prefix_bytes

UTEST(TestStringView, string_view_has_prefix_bytes1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_has_prefix_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_has_prefix_bytes2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_has_prefix_bytes(sv, (const u8*)"sub", 4));
}

UTEST(TestStringView, string_view_has_prefix_bytes3) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_prefix_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_has_prefix_bytes4) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_prefix_bytes(sv, (const u8*)"sub", 4));
}

UTEST(TestStringView, string_view_has_prefix_bytes5) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_prefix_bytes(sv, (const u8*)"syb", 4));
}

UTEST(TestStringView, string_view_has_prefix_bytes6) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_prefix_bytes(sv, (const u8*)"substring", 10));
}

UTEST(TestStringView, string_view_has_prefix_bytes7) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_prefix_bytes(sv, (const u8*)"substrings", 10));
}

// -- prefixes:string_view_has_prefix_bytes

UTEST(TestStringView, string_view_has_prefix_sv1) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_has_prefix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_prefix_sv2) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr("sub");
    ASSERT_FALSE(string_view_has_prefix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_prefix_sv3) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_has_prefix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_prefix_sv4) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr("sub");
    ASSERT_TRUE(string_view_has_prefix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_prefix_sv5) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr("syb");
    ASSERT_FALSE(string_view_has_prefix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_prefix_sv6) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_prefix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_prefix_sv7) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr("substrings");
    ASSERT_FALSE(string_view_has_prefix_sv(sv1, sv2));
}

// -- suffixes:string_view_has_suffix_cstr

UTEST(TestStringView, string_view_has_suffix_cstr1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_has_suffix_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_has_suffix_cstr2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_has_suffix_cstr(sv, "string"));
}

UTEST(TestStringView, string_view_has_suffix_cstr3) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_suffix_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_has_suffix_cstr4) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_suffix_cstr(sv, "string"));
}

UTEST(TestStringView, string_view_has_suffix_cstr5) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_suffix_cstr(sv, "strING"));
}

UTEST(TestStringView, string_view_has_suffix_cstr6) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_suffix_cstr(sv, "substring"));
}

UTEST(TestStringView, string_view_has_suffix_cstr7) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_suffix_cstr(sv, "substrings"));
}

// -- suffixes:string_view_has_suffix_bytes

UTEST(TestStringView, string_view_has_suffix_bytes1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_has_suffix_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_has_suffix_bytes2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_has_suffix_bytes(sv, (const u8*)"string", 4));
}

UTEST(TestStringView, string_view_has_suffix_bytes3) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_suffix_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_has_suffix_bytes4) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_suffix_bytes(sv, (const u8*)"string", 7));
}

UTEST(TestStringView, string_view_has_suffix_bytes5) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_suffix_bytes(sv, (const u8*)"strING", 7));
}

UTEST(TestStringView, string_view_has_suffix_bytes6) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_suffix_bytes(sv, (const u8*)"substring", 10));
}

UTEST(TestStringView, string_view_has_suffix_bytes7) {
    StringView sv = string_view_from_cstr("substring");
    ASSERT_FALSE(string_view_has_suffix_bytes(sv, (const u8*)"substrings", 10));
}

// -- suffixes:string_view_has_suffix_sv

UTEST(TestStringView, string_view_has_suffix_sv1) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_has_suffix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_suffix_sv2) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr("string");
    ASSERT_FALSE(string_view_has_suffix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_suffix_sv3) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_has_suffix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_suffix_sv4) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr("string");
    ASSERT_TRUE(string_view_has_suffix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_suffix_sv5) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr("strING");
    ASSERT_FALSE(string_view_has_suffix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_suffix_sv6) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr("substring");
    ASSERT_TRUE(string_view_has_suffix_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_has_suffix_sv7) {
    StringView sv1 = string_view_from_cstr("substring");
    StringView sv2 = string_view_from_cstr("substrings");
    ASSERT_FALSE(string_view_has_suffix_sv(sv1, sv2));
}

// -- subview --

UTEST(TestStringView, string_view_subview1) {
    StringView sv = string_view_from_cstr(NULL);
    StringView sub = string_view_subview(sv, 0, 0);
    ASSERT_TRUE(is_string_view_empty(sub));
}

UTEST(TestStringView, string_view_subview2) {
    StringView sv = string_view_from_cstr(NULL);
    StringView sub = string_view_subview(sv, 0, 10);
    ASSERT_TRUE(is_string_view_empty(sub));
}

UTEST(TestStringView, string_view_subview3) {
    StringView sv = string_view_from_cstr("hello world");
    StringView sub = string_view_subview(sv, 0, 0);
    ASSERT_TRUE(is_string_view_empty(sub));
}

UTEST(TestStringView, string_view_subview4) {
    StringView sv = string_view_from_cstr("hello world");
    StringView sub = string_view_subview(sv, 0, 5);
    ASSERT_STRNEQ("hello", (const char*)sub.data, 5);
    ASSERT_EQ(5, sub.len);
}

UTEST(TestStringView, string_view_subview5) {
    StringView sv = string_view_from_cstr("hello world");
    StringView sub = string_view_subview(sv, 0, 15);
    ASSERT_STRNEQ("hello world", (const char*)sub.data, 11);
    ASSERT_EQ(11, sub.len);
}

UTEST(TestStringView, string_view_subview6) {
    StringView sv = string_view_from_cstr("hello world");
    StringView sub = string_view_subview(sv, 6, 11);
    ASSERT_STRNEQ("world", (const char*)sub.data, 5);
    ASSERT_EQ(5, sub.len);
}

UTEST(TestStringView, string_view_subview7) {
    StringView sv = string_view_from_cstr("hello world");
    StringView sub = string_view_subview(sv, 6, 15);
    ASSERT_STRNEQ("world", (const char*)sub.data, 5);
    ASSERT_EQ(5, sub.len);
}

UTEST(TestStringView, string_view_subview8) {
    StringView sv = string_view_from_cstr("hello world");
    StringView sub = string_view_subview(sv, 11, 11);
    ASSERT_TRUE(is_string_view_empty(sub));
}

// -- unicode:string_view_count_runes --

UTEST(TestStringView, string_view_count_runes1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_count_runes(sv));
}

UTEST(TestStringView, string_view_count_runes2) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(5, string_view_count_runes(sv));
}

UTEST(TestStringView, string_view_count_runes3) {
    StringView sv = string_view_from_cstr("\xC2\xA9\xE2\x98\xBA\xF0\x9F\x98\x80");
    ASSERT_EQ(3, string_view_count_runes(sv));
}

UTEST(TestStringView, string_view_count_runes4) {
    StringView sv = string_view_from_cstr("\xED\xA0\x80");
    ASSERT_EQ(3, string_view_count_runes(sv));
}

// -- unicode:string_view_rune_at_byte --

UTEST(TestStringView, string_view_rune_at_byte1) {
    StringView sv = string_view_from_cstr(NULL);
    u32 rune_len = 0;
    ASSERT_EQ(RUNE_EOF, string_view_rune_at_byte(sv, 0, &rune_len));
    ASSERT_EQ(0, rune_len);
}

UTEST(TestStringView, string_view_rune_at_byte2) {
    StringView sv = string_view_from_cstr(NULL);
    u32 rune_len = 0;
    ASSERT_EQ(RUNE_EOF, string_view_rune_at_byte(sv, 5, &rune_len));
    ASSERT_EQ(0, rune_len);
}

UTEST(TestStringView, string_view_rune_at_byte3) {
    StringView sv = string_view_from_cstr("hello");
    u32 rune_len = 0;
    ASSERT_EQ((Rune)'h', string_view_rune_at_byte(sv, 0, &rune_len));
    ASSERT_EQ(1, rune_len);
}

UTEST(TestStringView, string_view_rune_at_byte4) {
    StringView sv = string_view_from_cstr("hello");
    u32 rune_len = 0;
    ASSERT_EQ((Rune)'l', string_view_rune_at_byte(sv, 3, &rune_len));
    ASSERT_EQ(1, rune_len);
}

UTEST(TestStringView, string_view_rune_at_byte5) {
    StringView sv = string_view_from_cstr("\xC2\xA9\xE2\x98\xBA\xF0\x9F\x98\x80");
    u32 rune_len = 0;
    ASSERT_EQ(0x00A9, string_view_rune_at_byte(sv, 0, &rune_len));
    ASSERT_EQ(2, rune_len);
}

UTEST(TestStringView, string_view_rune_at_byte6) {
    StringView sv = string_view_from_cstr("\xC2\xA9\xE2\x98\xBA\xF0\x9F\x98\x80");
    u32 rune_len = 0;
    ASSERT_EQ(0x263A, string_view_rune_at_byte(sv, 2, &rune_len));
    ASSERT_EQ(3, rune_len);
}

// -- unicode:string_view_rune_at --

UTEST(TestStringView, string_view_rune_at1) {
    StringView sv = string_view_from_cstr(NULL);
    u32 rune_len = 0;
    u64 byte_idx = 0;
    ASSERT_EQ(RUNE_EOF, string_view_rune_at(sv, 0, &byte_idx, &rune_len));
    ASSERT_EQ(0, rune_len);
    ASSERT_EQ(0, byte_idx);
}

UTEST(TestStringView, string_view_rune_at2) {
    StringView sv = string_view_from_cstr(NULL);
    u32 rune_len = 0;
    u64 byte_idx = 0;
    ASSERT_EQ(RUNE_EOF, string_view_rune_at(sv, 0, &byte_idx, &rune_len));
    ASSERT_EQ(0, rune_len);
    ASSERT_EQ(0, byte_idx);
}

UTEST(TestStringView, string_view_rune_at3) {
    StringView sv = string_view_from_cstr("hello");
    u32 rune_len = 0;
    u64 byte_idx = 0;
    ASSERT_EQ('h', string_view_rune_at(sv, 0, &byte_idx, &rune_len));
    ASSERT_EQ(1, rune_len);
    ASSERT_EQ(0, byte_idx);
}

UTEST(TestStringView, string_view_rune_at4) {
    StringView sv = string_view_from_cstr("hello");
    u32 rune_len = 0;
    u64 byte_idx = 0;
    ASSERT_EQ('l', string_view_rune_at(sv, 3, &byte_idx, &rune_len));
    ASSERT_EQ(1, rune_len);
    ASSERT_EQ(3, byte_idx);
}

UTEST(TestStringView, string_view_rune_at5) {
    StringView sv = string_view_from_cstr("\xC2\xA9\xE2\x98\xBA\xF0\x9F\x98\x80");
    u32 rune_len = 0;
    u64 byte_idx = 0;
    ASSERT_EQ(0x00A9, string_view_rune_at(sv, 0, &byte_idx, &rune_len));
    ASSERT_EQ(2, rune_len);
    ASSERT_EQ(0, byte_idx);
}

UTEST(TestStringView, string_view_rune_at6) {
    StringView sv = string_view_from_cstr("\xC2\xA9\xE2\x98\xBA\xF0\x9F\x98\x80");
    u32 rune_len = 0;
    u64 byte_idx = 0;
    ASSERT_EQ(0x1F600, string_view_rune_at(sv, 2, &byte_idx, &rune_len));
    ASSERT_EQ(4, rune_len);
    ASSERT_EQ(5, byte_idx);
}

// -- search:string_view_contains_c --

UTEST(TestStringView, string_view_contains_c1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_contains_c(sv, 'a'));
}

UTEST(TestStringView, string_view_contains_c2) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_contains_c(sv, 'a'));
}

UTEST(TestStringView, string_view_contains_c3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_TRUE(string_view_contains_c(sv, 'l'));
}

// -- search:string_view_contains_rune --

UTEST(TestStringView, string_view_contains_rune1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_contains_rune(sv, (Rune)'a'));
}

UTEST(TestStringView, string_view_contains_rune2) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_contains_rune(sv, (Rune)'a'));
}

UTEST(TestStringView, string_view_contains_rune3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_TRUE(string_view_contains_rune(sv, (Rune)'l'));
}

UTEST(TestStringView, string_view_contains_rune4) {
    StringView sv = string_view_from_cstr("\xC2\xA9\xE2\x98\xBA\xF0\x9F\x98\x80");
    ASSERT_TRUE(string_view_contains_rune(sv, (Rune)0x1F600));
}

// -- search:string_view_contains_cstr --

UTEST(TestStringView, string_view_contains_cstr1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_contains_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_contains_cstr2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_contains_cstr(sv, "a"));
}

UTEST(TestStringView, string_view_contains_cstr3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_TRUE(string_view_contains_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_contains_cstr4) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_contains_cstr(sv, "a"));
}

UTEST(TestStringView, string_view_contains_cstr5) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_TRUE(string_view_contains_cstr(sv, "l"));
}

UTEST(TestStringView, string_view_contains_cstr6) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_TRUE(string_view_contains_cstr(sv, "el"));
}

// -- search:string_view_contains_bytes --

UTEST(TestStringView, string_view_contains_bytes1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_contains_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_contains_bytes2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_FALSE(string_view_contains_bytes(sv, (const u8*)"a", 2));
}

UTEST(TestStringView, string_view_contains_bytes3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_TRUE(string_view_contains_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_contains_bytes4) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_contains_bytes(sv, (const u8*)"a", 2));
}

UTEST(TestStringView, string_view_contains_bytes5) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_contains_bytes(sv, (const u8*)"l", 2));
}

UTEST(TestStringView, string_view_contains_bytes6) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_FALSE(string_view_contains_bytes(sv, (const u8*)"el", 3));
}

// -- search:string_view_contains_sv --

UTEST(TestStringView, string_view_contains_sv1) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_contains_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_contains_sv2) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr("a");
    ASSERT_FALSE(string_view_contains_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_contains_sv3) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_TRUE(string_view_contains_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_contains_sv4) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("a");
    ASSERT_FALSE(string_view_contains_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_contains_sv5) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("l");
    ASSERT_TRUE(string_view_contains_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_contains_sv6) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("el");
    ASSERT_TRUE(string_view_contains_sv(sv1, sv2));
}

// -- search:string_view_find_c --

UTEST(TestStringView, string_view_find_c1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ((u64)NPOS, string_view_find_c(sv, 'a'));
}

UTEST(TestStringView, string_view_find_c2) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_c(sv, 'a'));
}

UTEST(TestStringView, string_view_find_c3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(2, string_view_find_c(sv, 'l'));
}

// -- search:string_view_find_rune --

UTEST(TestStringView, string_view_find_rune1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ((u64)NPOS, string_view_find_rune(sv, (Rune)'a'));
}

UTEST(TestStringView, string_view_find_rune2) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_rune(sv, (Rune)'a'));
}

UTEST(TestStringView, string_view_find_rune3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(2, string_view_find_rune(sv, (Rune)'l'));
}

UTEST(TestStringView, string_view_find_rune4) {
    StringView sv = string_view_from_cstr("\xC2\xA9\xE2\x98\xBA\xF0\x9F\x98\x80");
    ASSERT_EQ(5, string_view_find_rune(sv, (Rune)0x1F600));
}

UTEST(TestStringView, string_view_find_rune5) {
    StringView sv = string_view_from_cstr("\xC2\xA9\xE2\x98\xBA\xF0\x9F\x98\x80");
    ASSERT_EQ((u64)NPOS, string_view_find_rune(sv, (Rune)'l'));
}

// -- search:string_view_find_cstr --

UTEST(TestStringView, string_view_find_cstr1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_find_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_find_cstr2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ((u64)NPOS, string_view_find_cstr(sv, "a"));
}

UTEST(TestStringView, string_view_find_cstr3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(0, string_view_find_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_find_cstr4) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_cstr(sv, "a"));
}

UTEST(TestStringView, string_view_find_cstr5) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(2, string_view_find_cstr(sv, "l"));
}

UTEST(TestStringView, string_view_find_cstr6) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(1, string_view_find_cstr(sv, "el"));
}

// -- search:string_view_find_bytes --

UTEST(TestStringView, string_view_find_bytes1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_find_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_find_bytes2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ((u64)NPOS, string_view_find_bytes(sv, (const u8*)"a", 2));
}

UTEST(TestStringView, string_view_find_bytes3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(0, string_view_find_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_find_bytes4) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_bytes(sv, (const u8*)"a", 2));
}

UTEST(TestStringView, string_view_find_bytes5) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_bytes(sv, (const u8*)"l", 2));
}

UTEST(TestStringView, string_view_find_bytes6) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_bytes(sv, (const u8*)"el", 3));
}

// -- search:string_view_find_sv --

UTEST(TestStringView, string_view_find_sv1) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_find_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_sv2) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr("a");
    ASSERT_EQ((u64)NPOS, string_view_find_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_sv3) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_find_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_sv4) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("a");
    ASSERT_EQ((u64)NPOS, string_view_find_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_sv5) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("l");
    ASSERT_EQ(2, string_view_find_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_sv6) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("el");
    ASSERT_EQ(1, string_view_find_sv(sv1, sv2));
}

// -- search:string_view_find_last_c --

UTEST(TestStringView, string_view_find_last_c1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ((u64)NPOS, string_view_find_last_c(sv, 'a'));
}

UTEST(TestStringView, string_view_find_last_c2) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_last_c(sv, 'a'));
}

UTEST(TestStringView, string_view_find_last_c3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(3, string_view_find_last_c(sv, 'l'));
}

// -- search:string_view_find_last_cstr --

UTEST(TestStringView, string_view_find_last_cstr1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_find_last_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_find_last_cstr2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ((u64)NPOS, string_view_find_last_cstr(sv, "a"));
}

UTEST(TestStringView, string_view_find_last_cstr3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(0, string_view_find_last_cstr(sv, NULL));
}

UTEST(TestStringView, string_view_find_last_cstr4) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_last_cstr(sv, "a"));
}

UTEST(TestStringView, string_view_find_last_cstr5) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(3, string_view_find_last_cstr(sv, "l"));
}

UTEST(TestStringView, string_view_find_last_cstr6) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(1, string_view_find_last_cstr(sv, "el"));
}

// -- search:string_view_find_bytes --

UTEST(TestStringView, string_view_find_last_bytes1) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_find_last_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_find_last_bytes2) {
    StringView sv = string_view_from_cstr(NULL);
    ASSERT_EQ((u64)NPOS, string_view_find_last_bytes(sv, (const u8*)"a", 2));
}

UTEST(TestStringView, string_view_find_last_bytes3) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ(0, string_view_find_last_bytes(sv, NULL, 0));
}

UTEST(TestStringView, string_view_find_last_bytes4) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_last_bytes(sv, (const u8*)"a", 2));
}

UTEST(TestStringView, string_view_find_last_bytes5) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_last_bytes(sv, (const u8*)"l", 2));
}

UTEST(TestStringView, string_view_find_last_bytes6) {
    StringView sv = string_view_from_cstr("hello");
    ASSERT_EQ((u64)NPOS, string_view_find_last_bytes(sv, (const u8*)"el", 3));
}

// -- search:string_view_find_last_sv --

UTEST(TestStringView, string_view_find_last_sv1) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_find_last_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_last_sv2) {
    StringView sv1 = string_view_from_cstr(NULL);
    StringView sv2 = string_view_from_cstr("a");
    ASSERT_EQ((u64)NPOS, string_view_find_last_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_last_sv3) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(NULL);
    ASSERT_EQ(0, string_view_find_last_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_last_sv4) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("a");
    ASSERT_EQ((u64)NPOS, string_view_find_last_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_last_sv5) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("l");
    ASSERT_EQ(3, string_view_find_last_sv(sv1, sv2));
}

UTEST(TestStringView, string_view_find_last_sv6) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr("el");
    ASSERT_EQ(1, string_view_find_last_sv(sv1, sv2));
}