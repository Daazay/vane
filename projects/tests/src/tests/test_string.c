#include <utest/utest.h>

#include <vane/utils/string.h>

struct TestString {
    String s1;
    String s2;
    String s3;
};

#define S1 (utest_fixture->s1)
#define S2 (utest_fixture->s2)
#define S3 (utest_fixture->s3)

UTEST_F_SETUP(TestString) {
    S1 = STRING_EMPTY;
    S2 = STRING_EMPTY;
    S3 = STRING_EMPTY;
}

UTEST_F_TEARDOWN(TestString) {
    string_destroy(&S1);
    string_destroy(&S2);
    string_destroy(&S3);
}

// -- creation:string_from_cstr --

UTEST_F(TestString, string_from_cstr1) {
    S1 = string_from_cstr(NULL);
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_from_cstr2) {
    S1 = string_from_cstr("");
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_from_cstr3) {
    S1 = string_from_cstr("test");
    ASSERT_STRNEQ("test", S1.data, 4);
    ASSERT_EQ(4, S1.len);
    ASSERT_EQ(4, S1.cap);
}

// -- creation:string_from_bytes --

UTEST_F(TestString, string_from_bytes1) {
    S1 = string_from_bytes(NULL, 0);
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_from_bytes2) {
    S1 = string_from_bytes("", 1);
    ASSERT_EQ('\0', *(char*)S1.data);
    ASSERT_EQ(1, S1.len);
    ASSERT_EQ(1, S1.cap);
}

UTEST_F(TestString, string_from_bytes3) {
    S1 = string_from_bytes("test", 5);
    ASSERT_STRNEQ("test", S1.data, 4);
    ASSERT_EQ(5, S1.len);
    ASSERT_EQ(5, S1.cap);
}

// -- creation:string_from_sv --

UTEST_F(TestString, string_from_sv1) {
    StringView sv = string_view_from_cstr(NULL);
    S1 = string_from_sv(&sv);
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_from_sv2) {
    StringView sv = string_view_from_cstr("test");
    S1 = string_from_sv(&sv);
    ASSERT_STRNEQ("test", S1.data, 4);
    ASSERT_EQ(4, S1.len);
    ASSERT_EQ(4, S1.cap);
}

UTEST_F(TestString, string_from_sv3) {
    StringView sv = string_view_from_cstr("test");
    S1 = string_from_sv(&sv);
    ASSERT_STRNEQ("test", S1.data, 4);
    ASSERT_EQ(4, S1.len);
    ASSERT_EQ(4, S1.cap);
}

// -- creation:string_from_fmt --

UTEST_F(TestString, string_from_fmt1) {
    S1 = string_from_fmt("");
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_from_fmt2) {
    S1 = string_from_fmt("%s, world", "hello");
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(12, S1.len);
    ASSERT_EQ(12, S1.cap);
}

// -- creation:string_clone --

UTEST_F(TestString, string_clone1) {
    S1 = STRING_EMPTY;
    S2 = string_clone(&S1);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_clone2) {
    S1 = string_from_cstr("hello");
    S2 = string_clone(&S1);
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_EQ(5, S1.cap);
}

// -- concatination:string_concat_cstr --

UTEST_F(TestString, string_concat_cstr1) {
    S1 = string_concat_cstr(NULL, NULL);
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_concat_cstr2) {
    S1 = string_concat_cstr(NULL, "b");
    ASSERT_STRNEQ("b", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_EQ(1, S1.cap);
}

UTEST_F(TestString, string_concat_cstr3) {
    S1 = string_concat_cstr("a", NULL);
    ASSERT_STRNEQ("a", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_EQ(1, S1.cap);
}

UTEST_F(TestString, string_concat_cstr4) {
    S1 = string_concat_cstr("a", "b");
    ASSERT_STRNEQ("ab", S1.data, 2);
    ASSERT_EQ(2, S1.len);
    ASSERT_EQ(2, S1.cap);
}

UTEST_F(TestString, string_concat_cstr5) {
    S1 = string_concat_cstr("hello", " world");
    ASSERT_STRNEQ("hello world", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_EQ(11, S1.cap);
}

// -- concatination:string_concat_bytes --

UTEST_F(TestString, string_concat_bytes1) {
    const u8 buf1[] = { 0 };
    const u8 buf2[] = { 0 };
    S1 = string_concat_bytes(buf1, ARR_SIZE(buf1), buf2, ARR_SIZE(buf2));
    ASSERT_EQ('\0', *((const u8*)S1.data));
    ASSERT_EQ('\0', *((const u8*)(S1.data + 1)));
    ASSERT_EQ(2, S1.len);
    ASSERT_EQ(2, S1.cap);
}

UTEST_F(TestString, string_concat_bytes2) {
    const u8 buf1[] = "hello";
    const u8 buf2[] = { 0 };
    S1 = string_concat_bytes(buf1, ARR_SIZE(buf1), buf2, ARR_SIZE(buf2));
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(7, S1.len);
    ASSERT_EQ(7, S1.cap);
}

UTEST_F(TestString, string_concat_bytes3) {
    const u8 buf1[] = { 0 };
    const u8 buf2[] = " world";
    S1 = string_concat_bytes(buf1, ARR_SIZE(buf1), buf2, ARR_SIZE(buf2));
    ASSERT_STRNEQ(" world", S1.data + 1, 6);
    ASSERT_EQ(8, S1.len);
    ASSERT_EQ(8, S1.cap);
}

UTEST_F(TestString, string_concat_bytes4) {
    const u8 buf1[] = "hello";
    const u8 buf2[] = " world";
    S1 = string_concat_bytes(buf1, ARR_SIZE(buf1), buf2, ARR_SIZE(buf2));
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_STRNEQ(" world", S1.data + 6, 6);
    ASSERT_EQ(13, S1.len);
    ASSERT_EQ(13, S1.cap);
}

// -- concatination:string_concat_sv --

UTEST_F(TestString, string_concat_sv1) {
    StringView sv1 = STRING_VIEW_EMPTY;
    StringView sv2 = STRING_VIEW_EMPTY;
    S1 = string_concat_sv(&sv1, &sv2);
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_concat_sv2) {
    StringView sv1 = STRING_VIEW_EMPTY;
    StringView sv2 = string_view_from_cstr("b");
    S1 = string_concat_sv(&sv1, &sv2);
    ASSERT_STRNEQ("b", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_EQ(1, S1.cap);
}

UTEST_F(TestString, string_concat_sv3) {
    StringView sv1 = string_view_from_cstr("a");
    StringView sv2 = STRING_VIEW_EMPTY;
    S1 = string_concat_sv(&sv1, &sv2);
    ASSERT_STRNEQ("a", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_EQ(1, S1.cap);
}

UTEST_F(TestString, string_concat_sv4) {
    StringView sv1 = string_view_from_cstr("a");
    StringView sv2 = string_view_from_cstr("b");
    S1 = string_concat_sv(&sv1, &sv2);
    ASSERT_STRNEQ("ab", S1.data, 2);
    ASSERT_EQ(2, S1.len);
    ASSERT_EQ(2, S1.cap);
}

UTEST_F(TestString, string_concat_sv5) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(" world");
    S1 = string_concat_sv(&sv1, &sv2);
    ASSERT_STRNEQ("hello world", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_EQ(11, S1.cap);
}

// -- concatination:string_concat_str --

UTEST_F(TestString, string_concat_str1) {
    String s1 = STRING_EMPTY;
    String s2 = STRING_EMPTY;
    S1 = string_concat_str(&s1, &s2);
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_concat_str2) {
    String s1 = STRING_EMPTY;
    String s2 = string_from_cstr("b");
    S1 = string_concat_str(&s1, &s2);
    ASSERT_STRNEQ("b", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_EQ(1, S1.cap);
}

UTEST_F(TestString, string_concat_str3) {
    String s1 = string_from_cstr("a");
    String s2 = STRING_EMPTY;
    S1 = string_concat_str(&s1, &s2);
    ASSERT_STRNEQ("a", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_EQ(1, S1.cap);
}

UTEST_F(TestString, string_concat_str4) {
    String s1 = string_from_cstr("a");
    String s2 = string_from_cstr("b");
    S1 = string_concat_str(&s1, &s2);
    ASSERT_STRNEQ("ab", S1.data, 2);
    ASSERT_EQ(2, S1.len);
    ASSERT_EQ(2, S1.cap);
}

UTEST_F(TestString, string_concat_str5) {
    String s1 = string_from_cstr("hello");
    String s2 = string_from_cstr(" world");
    S1 = string_concat_str(&s1, &s2);
    ASSERT_STRNEQ("hello world", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_EQ(11, S1.cap);
}

// -- appending:string_append_c --

UTEST_F(TestString, string_append_c1) {
    string_append_c(&S1, 'a');
    ASSERT_STRNEQ("a", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_c2) {
    string_append_c(&S1, 'a');
    string_append_c(&S1, 'b');
    string_append_c(&S1, 'c');
    ASSERT_STRNEQ("abc", S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

// -- appending:string_append_rune --

UTEST_F(TestString, string_append_rune1) {
    string_append_rune(&S1, (Rune)'j');
    ASSERT_STRNEQ("j", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_rune2) {
    string_append_rune(&S1, (Rune)'h');
    string_append_rune(&S1, (Rune)'e');
    string_append_rune(&S1, (Rune)'l');
    string_append_rune(&S1, (Rune)'l');
    string_append_rune(&S1, (Rune)'o');
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_rune3) {
    string_append_rune(&S1, (Rune)0x00A9);
    ASSERT_STRNEQ("\xC2\xA9", S1.data, 2);
    ASSERT_EQ(2, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_rune4) {
    string_append_rune(&S1, (Rune)0x00A9);
    string_append_rune(&S1, (Rune)'h');
    ASSERT_STRNEQ("\xC2\xA9h", S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

// -- appending:string_append_cstr --

UTEST_F(TestString, string_append_cstr1) {
    string_append_cstr(&S1, NULL);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_cstr2) {
    string_append_cstr(&S1, "");
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_cstr3) {
    string_append_cstr(&S1, "abc");
    ASSERT_STRNEQ("abc", S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_cstr4) {
    string_append_cstr(&S1, "hello");
    string_append_cstr(&S1, " ");
    string_append_cstr(&S1, "world");
    ASSERT_STRNEQ("hello world", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LT(0, S1.cap);
}

// -- appending:string_append_bytes --

UTEST_F(TestString, string_append_bytes1) {
    string_append_bytes(&S1, NULL, 0);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_bytes2) {
    string_append_bytes(&S1, "", 1);
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(1, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_bytes3) {
    string_append_bytes(&S1, "hello", 6);
    string_append_bytes(&S1, " world", 7);
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_STRNEQ(" world", S1.data + 6, 6);
    ASSERT_EQ(13, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- appending:string_append_sv --

UTEST_F(TestString, string_append_sv1) {
    StringView sv = STRING_VIEW_EMPTY;
    string_append_sv(&S1, &sv);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_sv2) {
    StringView sv = string_view_from_cstr("hello");
    string_append_sv(&S1, &sv);
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_sv3) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(" world");
    string_append_sv(&S1, &sv1);
    string_append_sv(&S1, &sv2);
    ASSERT_STRNEQ("hello world", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- appending:string_append_str --

UTEST_F(TestString, string_append_str1) {
    string_append_str(&S1, &S2);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_str2) {
    S2 = string_from_cstr("hello");
    string_append_str(&S1, &S2);
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_str3) {
    S2 = string_from_cstr("hello");
    S3 = string_from_cstr(" world");
    string_append_str(&S1, &S2);
    string_append_str(&S1, &S3);
    ASSERT_STRNEQ("hello world", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LE(0, S1.cap);
}


// -- appending:string_append_c --

UTEST_F(TestString, string_append_left_c1) {
    string_append_left_c(&S1, 'a');
    ASSERT_STRNEQ("a", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_c2) {
    string_append_left_c(&S1, 'a');
    string_append_left_c(&S1, 'b');
    string_append_left_c(&S1, 'c');
    ASSERT_STRNEQ("cba", S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

// -- appending:string_append_left_rune --

UTEST_F(TestString, string_append_left_rune1) {
    string_append_left_rune(&S1, (Rune)'j');
    ASSERT_STRNEQ("j", S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_rune2) {
    string_append_left_rune(&S1, (Rune)'h');
    string_append_left_rune(&S1, (Rune)'e');
    string_append_left_rune(&S1, (Rune)'l');
    string_append_left_rune(&S1, (Rune)'l');
    string_append_left_rune(&S1, (Rune)'o');
    ASSERT_STRNEQ("olleh", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_rune3) {
    string_append_left_rune(&S1, (Rune)0x00A9);
    ASSERT_STRNEQ("\xC2\xA9", S1.data, 2);
    ASSERT_EQ(2, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_rune4) {
    string_append_left_rune(&S1, (Rune)0x00A9);
    string_append_left_rune(&S1, (Rune)'h');
    ASSERT_STRNEQ("h\xC2\xA9", S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

// -- appending:string_append_left_cstr --

UTEST_F(TestString, string_append_left_cstr1) {
    string_append_left_cstr(&S1, NULL);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_cstr2) {
    string_append_left_cstr(&S1, "");
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_cstr3) {
    string_append_left_cstr(&S1, "abc");
    ASSERT_STRNEQ("abc", S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_cstr4) {
    string_append_left_cstr(&S1, "hello");
    string_append_left_cstr(&S1, " ");
    string_append_left_cstr(&S1, "world");
    ASSERT_STRNEQ("world hello", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LT(0, S1.cap);
}

// -- appending:string_append_left_bytes --

UTEST_F(TestString, string_append_left_bytes1) {
    string_append_left_bytes(&S1, NULL, 0);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_bytes2) {
    string_append_left_bytes(&S1, "", 1);
    ASSERT_NE(NULL, S1.data);
    ASSERT_EQ(1, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_bytes3) {
    string_append_left_bytes(&S1, "hello", 6);
    string_append_left_bytes(&S1, " world", 7);
    ASSERT_STRNEQ(" world", S1.data, 6);
    ASSERT_STRNEQ("hello", S1.data + 7, 5);
    ASSERT_EQ(13, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- appending:string_append_left_sv --

UTEST_F(TestString, string_append_left_sv1) {
    StringView sv = STRING_VIEW_EMPTY;
    string_append_left_sv(&S1, &sv);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_sv2) {
    StringView sv = string_view_from_cstr("hello");
    string_append_left_sv(&S1, &sv);
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_sv3) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(" world");
    string_append_left_sv(&S1, &sv1);
    string_append_left_sv(&S1, &sv2);
    ASSERT_STRNEQ(" worldhello", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- appending:string_append_left_str --

UTEST_F(TestString, string_append_left_str1) {
    string_append_left_str(&S1, &S2);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_str2) {
    S2 = string_from_cstr("hello");
    string_append_left_str(&S1, &S2);
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_str3) {
    S2 = string_from_cstr("hello");
    S3 = string_from_cstr(" world");
    string_append_left_str(&S1, &S2);
    string_append_left_str(&S1, &S3);
    ASSERT_STRNEQ(" worldhello", S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- replacement:string_replace_c --

UTEST_F(TestString, string_replace_c1) {
    string_replace_c(&S1, 'a', 'z');
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_c2) {
    S1 = string_from_cstr("hello");
    string_replace_c(&S1, 'a', 'z');
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_c3) {
    S1 = string_from_cstr("hello");
    string_replace_c(&S1, 'l', 'L');
    ASSERT_STRNEQ("heLLo", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- replacement:string_replace_rune --

UTEST_F(TestString, string_replace_rune1) {
    string_replace_rune(&S1, (Rune)'a', (Rune)'z');
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_rune2) {
    S1 = string_from_cstr("hello");
    string_replace_rune(&S1, (Rune)'a', (Rune)'z');
    ASSERT_STRNEQ("hello", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_rune3) {
    S1 = string_from_cstr("hello");
    string_replace_rune(&S1, (Rune)'l', (Rune)'L');
    ASSERT_STRNEQ("heLLo", S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_rune4) {
    S1 = string_from_cstr("hello");
    string_replace_rune(&S1, (Rune)'e', (Rune)0x1F600);
    ASSERT_STRNEQ("\x68\xF0\x9F\x98\x80\x6C\x6C\x6F", S1.data, 8);
    ASSERT_EQ(8, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- replacement:string_replace_cstr --

UTEST_F(TestString, string_replace_cstr1) {
    string_replace_cstr(&S1, NULL, NULL);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr2) {
    string_replace_cstr(&S1, NULL, "replace");
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr3) {
    string_replace_cstr(&S1, "find", NULL);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr4) {
    string_replace_cstr(&S1, "find", "replace");
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr5) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_cstr(&S1, "not found", "replace");
    ASSERT_STRNEQ("finding and replacement", S1.data, 23);
    ASSERT_EQ(23, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr6) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_cstr(&S1, "find", NULL);
    ASSERT_STRNEQ("ing and replacement", S1.data, 19);
    ASSERT_EQ(19, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr7) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_cstr(&S1, "find", "replace");
    ASSERT_STRNEQ("replaceing and replacement", S1.data, 26);
    ASSERT_EQ(26, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- replacement:string_replace_bytes --

UTEST_F(TestString, string_replace_bytes1) {
    string_replace_bytes(&S1, NULL, 0, NULL, 0);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes2) {
    string_replace_bytes(&S1, NULL, 0, "replace", 9);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes3) {
    string_replace_bytes(&S1, "find", 5, NULL, 0);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes4) {
    string_replace_bytes(&S1, "find", 5, "replace", 9);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes5) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_bytes(&S1, "not found", 9, "replace", 7);
    ASSERT_STRNEQ("finding and replacement", S1.data, 23);
    ASSERT_EQ(23, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes6) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_bytes(&S1, "find", 4, NULL, 0);
    ASSERT_STRNEQ("ing and replacement", S1.data, 19);
    ASSERT_EQ(19, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes7) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_bytes(&S1, "find", 4, "replace", 7);
    ASSERT_STRNEQ("replaceing and replacement", S1.data, 26);
    ASSERT_EQ(26, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- replacement:string_replace_sv --

UTEST_F(TestString, string_replace_sv1) {
    StringView sv1 = STRING_VIEW_EMPTY;
    StringView sv2 = STRING_VIEW_EMPTY;
    string_replace_sv(&S1, &sv1, &sv2);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv2) {
    StringView sv1 = STRING_VIEW_EMPTY;
    StringView sv2 = string_view_from_cstr("replace");
    string_replace_sv(&S1, &sv1, &sv2);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv3) {
    StringView sv1 = string_view_from_cstr("find");
    StringView sv2 = STRING_VIEW_EMPTY;
    string_replace_sv(&S1, &sv1, &sv2);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv4) {
    StringView sv1 = string_view_from_cstr("find");
    StringView sv2 = string_view_from_cstr("replace");
    string_replace_sv(&S1, &sv1, &sv2);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv5) {
    StringView sv1 = string_view_from_cstr("not found");
    StringView sv2 = string_view_from_cstr("replace");
    S1 = string_from_cstr("finding and replacement");
    string_replace_sv(&S1, &sv1, &sv2);
    ASSERT_STRNEQ("finding and replacement", S1.data, 23);
    ASSERT_EQ(23, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv6) {
    StringView sv1 = string_view_from_cstr("find");
    StringView sv2 = STRING_VIEW_EMPTY;
    S1 = string_from_cstr("finding and replacement");
    string_replace_sv(&S1, &sv1, &sv2);
    ASSERT_STRNEQ("ing and replacement", S1.data, 19);
    ASSERT_EQ(19, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv7) {
    StringView sv1 = string_view_from_cstr("find");
    StringView sv2 = string_view_from_cstr("replace");
    S1 = string_from_cstr("finding and replacement");
    string_replace_sv(&S1, &sv1, &sv2);
    ASSERT_STRNEQ("replaceing and replacement", S1.data, 26);
    ASSERT_EQ(26, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- replacement:string_replace_str --

UTEST_F(TestString, string_replace_str1) {
    S2 = STRING_EMPTY;
    S3 = STRING_EMPTY;
    string_replace_str(&S1, &S2, &S3);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_str2) {
    S2 = STRING_EMPTY;
    S3 = string_from_cstr("replace");
    string_replace_str(&S1, &S2, &S3);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_str3) {
    S2 = string_from_cstr("find");
    S3 = STRING_EMPTY;
    string_replace_str(&S1, &S2, &S3);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_str4) {
    S2 = string_from_cstr("find");
    S3 = string_from_cstr("replace");
    string_replace_str(&S1, &S2, &S3);
    ASSERT_EQ(NULL, S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_str5) {
    S2 = string_from_cstr("not found");
    S3 = string_from_cstr("replace");
    S1 = string_from_cstr("finding and replacement");
    string_replace_str(&S1, &S2, &S3);
    ASSERT_STRNEQ("finding and replacement", S1.data, 23);
    ASSERT_EQ(23, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_str6) {
    S2 = string_from_cstr("find");
    S3 = STRING_EMPTY;
    S1 = string_from_cstr("finding and replacement");
    string_replace_str(&S1, &S2, &S3);
    ASSERT_STRNEQ("ing and replacement", S1.data, 19);
    ASSERT_EQ(19, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_str7) {
    S2 = string_from_cstr("find");
    S3 = string_from_cstr("replace");
    S1 = string_from_cstr("finding and replacement");
    string_replace_str(&S1, &S2, &S3);
    ASSERT_STRNEQ("replaceing and replacement", S1.data, 26);
    ASSERT_EQ(26, S1.len);
    ASSERT_LE(0, S1.cap);
}