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

// -- creation --

UTEST_F(TestString, string_from_cstr1) {
    S1 = string_from_cstr(NULL);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_from_cstr2) {
    S1 = string_from_cstr("");
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_from_cstr3) {
    S1 = string_from_cstr("test");
    ASSERT_STRNEQ("test", (const char*)S1.data, 4);
    ASSERT_EQ(4, S1.len);
    ASSERT_EQ(4, S1.cap);
}

UTEST_F(TestString, string_from_bytes1) {
    S1 = string_from_bytes(NULL, 0);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_from_bytes2) {
    S1 = string_from_bytes((const u8*)"", 1);
    ASSERT_EQ('\0', *(char*)S1.data);
    ASSERT_EQ(1, S1.len);
    ASSERT_EQ(1, S1.cap);
}

UTEST_F(TestString, string_from_bytes3) {
    S1 = string_from_bytes((const u8*)"test", 5);
    ASSERT_STRNEQ("test", (const char*)S1.data, 4);
    ASSERT_EQ(5, S1.len);
    ASSERT_EQ(5, S1.cap);
}

UTEST_F(TestString, string_from_fmt1) {
    S1 = string_from_fmt(NULL);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_from_fmt2) {
    S1 = string_from_fmt("");
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_from_fmt3) {
    S1 = string_from_fmt("%s, world", "hello");
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_EQ(12, S1.len);
    ASSERT_EQ(12, S1.cap);
}

UTEST_F(TestString, string_from_sv1) {
    StringView sv = string_view_from_cstr(NULL);
    S1 = string_from_sv(sv);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_from_sv2) {
    StringView sv = string_view_from_cstr("test");
    S1 = string_from_sv(sv);
    ASSERT_STRNEQ("test", (const char*)S1.data, 4);
    ASSERT_EQ(4, S1.len);
    ASSERT_EQ(4, S1.cap);
}

UTEST_F(TestString, string_from_sv3) {
    StringView sv = string_view_from_cstr("test");
    S1 = string_from_sv(sv);
    ASSERT_STRNEQ("test", (const char*)S1.data, 4);
    ASSERT_EQ(4, S1.len);
    ASSERT_EQ(4, S1.cap);
}

UTEST_F(TestString, string_clone1) {
    S1 = STRING_EMPTY;
    S2 = string_clone(S1);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_clone2) {
    S1 = string_from_cstr("hello");
    S2 = string_clone(S1);
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_EQ(5, S1.cap);
}

// -- appendings --

UTEST_F(TestString, string_append_c1) {
    string_append_c(&S1, 'a');
    ASSERT_STRNEQ("a", (const char*)S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_c2) {
    string_append_c(&S1, 'a');
    string_append_c(&S1, 'b');
    string_append_c(&S1, 'c');
    ASSERT_STRNEQ("abc", (const char*)S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_rune1) {
    string_append_rune(&S1, (Rune)'j');
    ASSERT_STRNEQ("j", (const char*)S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_rune2) {
    string_append_rune(&S1, (Rune)'h');
    string_append_rune(&S1, (Rune)'e');
    string_append_rune(&S1, (Rune)'l');
    string_append_rune(&S1, (Rune)'l');
    string_append_rune(&S1, (Rune)'o');
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_rune3) {
    string_append_rune(&S1, (Rune)0x00A9);
    ASSERT_STRNEQ("\xC2\xA9", (const char*)S1.data, 2);
    ASSERT_EQ(2, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_rune4) {
    string_append_rune(&S1, (Rune)0x00A9);
    string_append_rune(&S1, (Rune)'h');
    ASSERT_STRNEQ("\xC2\xA9h", (const char*)S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_cstr1) {
    string_append_cstr(&S1, NULL);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_cstr2) {
    string_append_cstr(&S1, "");
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_cstr3) {
    string_append_cstr(&S1, "abc");
    ASSERT_STRNEQ("abc", (const char*)S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_cstr4) {
    string_append_cstr(&S1, "hello");
    string_append_cstr(&S1, " ");
    string_append_cstr(&S1, "world");
    ASSERT_STRNEQ("hello world", (const char*)S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_bytes1) {
    string_append_bytes(&S1, NULL, 0);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_bytes2) {
    string_append_bytes(&S1, (const u8*)"", 1);
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_EQ(1, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_bytes3) {
    string_append_bytes(&S1, (const u8*)"hello", 6);
    string_append_bytes(&S1, (const u8*)" world", 7);
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_STRNEQ(" world", (const char*)S1.data + 6, 6);
    ASSERT_EQ(13, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_fmt1) {
    string_append_fmt(&S1, NULL);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_fmt2) {
    string_append_fmt(&S1, "");
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_fmt3) {
    string_append_fmt(&S1, "hello");
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_fmt4) {
    string_append_fmt(&S1, "hello %s", "world");
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_STRNEQ("hello world", (const char*)S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_fmt5) {
    S1 = string_from_cstr("hello");
    string_append_fmt(&S1, ", %s", "world");
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_STRNEQ("hello, world", (const char*)S1.data, 12);
    ASSERT_EQ(12, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_sv1) {
    StringView sv = STRING_VIEW_EMPTY;
    string_append_sv(&S1, sv);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_sv2) {
    StringView sv = string_view_from_cstr("hello");
    string_append_sv(&S1, sv);
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_sv3) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(" world");
    string_append_sv(&S1, sv1);
    string_append_sv(&S1, sv2);
    ASSERT_STRNEQ("hello world", (const char*)S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_str1) {
    string_append_str(&S1, S2);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_str2) {
    S2 = string_from_cstr("hello");
    string_append_str(&S1, S2);
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_str3) {
    S2 = string_from_cstr("hello");
    S3 = string_from_cstr(" world");
    string_append_str(&S1, S2);
    string_append_str(&S1, S3);
    ASSERT_STRNEQ("hello world", (const char*)S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_c1) {
    string_append_left_c(&S1, 'a');
    ASSERT_STRNEQ("a", (const char*)S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_c2) {
    string_append_left_c(&S1, 'a');
    string_append_left_c(&S1, 'b');
    string_append_left_c(&S1, 'c');
    ASSERT_STRNEQ("cba", (const char*)S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_rune1) {
    string_append_left_rune(&S1, (Rune)'j');
    ASSERT_STRNEQ("j", (const char*)S1.data, 1);
    ASSERT_EQ(1, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_rune2) {
    string_append_left_rune(&S1, (Rune)'h');
    string_append_left_rune(&S1, (Rune)'e');
    string_append_left_rune(&S1, (Rune)'l');
    string_append_left_rune(&S1, (Rune)'l');
    string_append_left_rune(&S1, (Rune)'o');
    ASSERT_STRNEQ("olleh", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_rune3) {
    string_append_left_rune(&S1, (Rune)0x00A9);
    ASSERT_STRNEQ("\xC2\xA9", (const char*)S1.data, 2);
    ASSERT_EQ(2, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_rune4) {
    string_append_left_rune(&S1, (Rune)0x00A9);
    string_append_left_rune(&S1, (Rune)'h');
    ASSERT_STRNEQ("h\xC2\xA9", (const char*)S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_cstr1) {
    string_append_left_cstr(&S1, NULL);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_cstr2) {
    string_append_left_cstr(&S1, "");
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_cstr3) {
    string_append_left_cstr(&S1, "abc");
    ASSERT_STRNEQ("abc", (const char*)S1.data, 3);
    ASSERT_EQ(3, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_cstr4) {
    string_append_left_cstr(&S1, "hello");
    string_append_left_cstr(&S1, " ");
    string_append_left_cstr(&S1, "world");
    ASSERT_STRNEQ("world hello", (const char*)S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_bytes1) {
    string_append_left_bytes(&S1, NULL, 0);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_bytes2) {
    string_append_left_bytes(&S1, (const u8*)"", 1);
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_EQ(1, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_bytes3) {
    string_append_left_bytes(&S1, (const u8*)"hello", 6);
    string_append_left_bytes(&S1, (const u8*)" world", 7);
    ASSERT_STRNEQ(" world", (const char*)S1.data, 6);
    ASSERT_STRNEQ("hello", (const char*)S1.data + 7, 5);
    ASSERT_EQ(13, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_fmt1) {
    string_append_left_fmt(&S1, NULL);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_fmt2) {
    string_append_left_fmt(&S1, "");
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_fmt3) {
    string_append_left_fmt(&S1, "hello");
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_fmt4) {
    string_append_left_fmt(&S1, "hello %s", "world");
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_STRNEQ("hello world", (const char*)S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_fmt5) {
    S1 = string_from_cstr("hello");
    string_append_left_fmt(&S1, ", %s", "world");
    ASSERT_NE(NULL, (const char*)S1.data);
    ASSERT_STRNEQ(", worldhello", (const char*)S1.data, 12);
    ASSERT_EQ(12, S1.len);
    ASSERT_LT(0, S1.cap);
}

UTEST_F(TestString, string_append_left_sv1) {
    StringView sv = STRING_VIEW_EMPTY;
    string_append_left_sv(&S1, sv);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_sv2) {
    StringView sv = string_view_from_cstr("hello");
    string_append_left_sv(&S1, sv);
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_sv3) {
    StringView sv1 = string_view_from_cstr("hello");
    StringView sv2 = string_view_from_cstr(" world");
    string_append_left_sv(&S1, sv1);
    string_append_left_sv(&S1, sv2);
    ASSERT_STRNEQ(" worldhello", (const char*)S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_str1) {
    string_append_left_str(&S1, S2);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_append_left_str2) {
    S2 = string_from_cstr("hello");
    string_append_left_str(&S1, S2);
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_append_left_str3) {
    S2 = string_from_cstr("hello");
    S3 = string_from_cstr(" world");
    string_append_left_str(&S1, S2);
    string_append_left_str(&S1, S3);
    ASSERT_STRNEQ(" worldhello", (const char*)S1.data, 11);
    ASSERT_EQ(11, S1.len);
    ASSERT_LE(0, S1.cap);
}

// -- replacement --

UTEST_F(TestString, string_replace_c1) {
    string_replace_c(&S1, 'a', 'z');
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_c2) {
    S1 = string_from_cstr("hello");
    string_replace_c(&S1, 'a', 'z');
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_c3) {
    S1 = string_from_cstr("hello");
    string_replace_c(&S1, 'l', 'L');
    ASSERT_STRNEQ("heLLo", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_rune1) {
    string_replace_rune(&S1, (Rune)'a', (Rune)'z');
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_rune2) {
    S1 = string_from_cstr("hello");
    string_replace_rune(&S1, (Rune)'a', (Rune)'z');
    ASSERT_STRNEQ("hello", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_rune3) {
    S1 = string_from_cstr("hello");
    string_replace_rune(&S1, (Rune)'l', (Rune)'L');
    ASSERT_STRNEQ("heLLo", (const char*)S1.data, 5);
    ASSERT_EQ(5, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_rune4) {
    S1 = string_from_cstr("hello");
    string_replace_rune(&S1, (Rune)'e', (Rune)0x1F600);
    ASSERT_STRNEQ("\x68\xF0\x9F\x98\x80\x6C\x6C\x6F", (const char*)S1.data, 8);
    ASSERT_EQ(8, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr1) {
    string_replace_cstr(&S1, NULL, NULL);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr2) {
    string_replace_cstr(&S1, NULL, "replace");
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr3) {
    string_replace_cstr(&S1, "find", NULL);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr4) {
    string_replace_cstr(&S1, "find", "replace");
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr5) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_cstr(&S1, "not found", "replace");
    ASSERT_STRNEQ("finding and replacement", (const char*)S1.data, 23);
    ASSERT_EQ(23, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr6) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_cstr(&S1, "find", NULL);
    ASSERT_STRNEQ("ing and replacement", (const char*)S1.data, 19);
    ASSERT_EQ(19, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_cstr7) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_cstr(&S1, "find", "replace");
    ASSERT_STRNEQ("replaceing and replacement", (const char*)S1.data, 26);
    ASSERT_EQ(26, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes1) {
    string_replace_bytes(&S1, NULL, 0, NULL, 0);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes2) {
    string_replace_bytes(&S1, NULL, 0, (const u8*)"replace", 9);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes3) {
    string_replace_bytes(&S1, (const u8*)"find", 5, NULL, 0);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes4) {
    string_replace_bytes(&S1, (const u8*)"find", 5, (const u8*)"replace", 9);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes5) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_bytes(&S1, (const u8*)"not found", 9, (const u8*)"replace", 7);
    ASSERT_STRNEQ("finding and replacement", (const char*)S1.data, 23);
    ASSERT_EQ(23, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes6) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_bytes(&S1, (const u8*)"find", 4, NULL, 0);
    ASSERT_STRNEQ("ing and replacement", (const char*)S1.data, 19);
    ASSERT_EQ(19, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_bytes7) {
    S1 = string_from_cstr("finding and replacement");
    string_replace_bytes(&S1, (const u8*)"find", 4, (const u8*)"replace", 7);
    ASSERT_STRNEQ("replaceing and replacement", (const char*)S1.data, 26);
    ASSERT_EQ(26, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv1) {
    StringView sv1 = STRING_VIEW_EMPTY;
    StringView sv2 = STRING_VIEW_EMPTY;
    string_replace_sv(&S1, sv1, sv2);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv2) {
    StringView sv1 = STRING_VIEW_EMPTY;
    StringView sv2 = string_view_from_cstr("replace");
    string_replace_sv(&S1, sv1, sv2);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv3) {
    StringView sv1 = string_view_from_cstr("find");
    StringView sv2 = STRING_VIEW_EMPTY;
    string_replace_sv(&S1, sv1, sv2);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv4) {
    StringView sv1 = string_view_from_cstr("find");
    StringView sv2 = string_view_from_cstr("replace");
    string_replace_sv(&S1, sv1, sv2);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv5) {
    StringView sv1 = string_view_from_cstr("not found");
    StringView sv2 = string_view_from_cstr("replace");
    S1 = string_from_cstr("finding and replacement");
    string_replace_sv(&S1, sv1, sv2);
    ASSERT_STRNEQ("finding and replacement", (const char*)S1.data, 23);
    ASSERT_EQ(23, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv6) {
    StringView sv1 = string_view_from_cstr("find");
    StringView sv2 = STRING_VIEW_EMPTY;
    S1 = string_from_cstr("finding and replacement");
    string_replace_sv(&S1, sv1, sv2);
    ASSERT_STRNEQ("ing and replacement", (const char*)S1.data, 19);
    ASSERT_EQ(19, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_sv7) {
    StringView sv1 = string_view_from_cstr("find");
    StringView sv2 = string_view_from_cstr("replace");
    S1 = string_from_cstr("finding and replacement");
    string_replace_sv(&S1, sv1, sv2);
    ASSERT_STRNEQ("replaceing and replacement", (const char*)S1.data, 26);
    ASSERT_EQ(26, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_str1) {
    S2 = STRING_EMPTY;
    S3 = STRING_EMPTY;
    string_replace_str(&S1, S2, S3);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_str2) {
    S2 = STRING_EMPTY;
    S3 = string_from_cstr("replace");
    string_replace_str(&S1, S2, S3);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_str3) {
    S2 = string_from_cstr("find");
    S3 = STRING_EMPTY;
    string_replace_str(&S1, S2, S3);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_str4) {
    S2 = string_from_cstr("find");
    S3 = string_from_cstr("replace");
    string_replace_str(&S1, S2, S3);
    ASSERT_EQ(NULL, (const char*)S1.data);
    ASSERT_EQ(0, S1.len);
    ASSERT_EQ(0, S1.cap);
}

UTEST_F(TestString, string_replace_str5) {
    S2 = string_from_cstr("not found");
    S3 = string_from_cstr("replace");
    S1 = string_from_cstr("finding and replacement");
    string_replace_str(&S1, S2, S3);
    ASSERT_STRNEQ("finding and replacement", (const char*)S1.data, 23);
    ASSERT_EQ(23, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_str6) {
    S2 = string_from_cstr("find");
    S3 = STRING_EMPTY;
    S1 = string_from_cstr("finding and replacement");
    string_replace_str(&S1, S2, S3);
    ASSERT_STRNEQ("ing and replacement", (const char*)S1.data, 19);
    ASSERT_EQ(19, S1.len);
    ASSERT_LE(0, S1.cap);
}

UTEST_F(TestString, string_replace_str7) {
    S2 = string_from_cstr("find");
    S3 = string_from_cstr("replace");
    S1 = string_from_cstr("finding and replacement");
    string_replace_str(&S1, S2, S3);
    ASSERT_STRNEQ("replaceing and replacement", (const char*)S1.data, 26);
    ASSERT_EQ(26, S1.len);
    ASSERT_LE(0, S1.cap);
}