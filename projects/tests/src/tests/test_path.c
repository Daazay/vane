#include <utest/utest.h>

#include <vane/utils/path.h>

struct TestPath {
    String path0;
    String path1;
    String path2;
};

#define P0 utest_fixture->path0
#define P1 utest_fixture->path1
#define P2 utest_fixture->path2

UTEST_F_SETUP(TestPath) {
    P0 = STRING_EMPTY;
    P1 = STRING_EMPTY;
    P2 = STRING_EMPTY;
}

UTEST_F_TEARDOWN(TestPath) {
    string_destroy(&P0);
    string_destroy(&P1);
    string_destroy(&P2);
}

// is_path_absolute

UTEST_F(TestPath, is_path_absolute1) {
    StringView input = STRING_VIEW_EMPTY;
#if defined(PLATFORM_WINDOWS)
    ASSERT_FALSE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute2) {
    StringView input = STR_LIT("\\");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute3) {
    StringView input = STR_LIT("C:\\");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute4) {
    StringView input = STR_LIT("C:\\folder\\file.txt");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute5) {
    StringView input = STR_LIT("D:/sub/folder");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute6) {
    StringView input = STR_LIT("\\\\server\\share");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_path_absolute(input));
#else
    ASSERT_TRUE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute7) {
    StringView input = STR_LIT("\\\\server\\share\\folder");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_path_absolute(input));
#else
    ASSERT_TRUE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute8) {
    StringView input = STR_LIT("folder\\file.text");
#if defined(PLATFORM_WINDOWS)
    ASSERT_FALSE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute9) {
    StringView input = STR_LIT(".\\folder");
#if defined(PLATFORM_WINDOWS)
    ASSERT_FALSE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute10) {
    StringView input = STR_LIT("home/user");
#if defined(PLATFORM_WINDOWS)
    ASSERT_FALSE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute11) {
    StringView input = STR_LIT("/home/user");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_path_absolute(input));
#else
    ASSERT_TRUE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute12) {
    StringView input = STR_LIT("/");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_path_absolute(input));
#else
    ASSERT_TRUE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute13) {
    StringView input = STR_LIT("../file");
#if defined(PLATFORM_WINDOWS)
    ASSERT_FALSE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, is_path_absolute14) {
    StringView input = STR_LIT("./file");
#if defined(PLATFORM_WINDOWS)
    ASSERT_FALSE(is_path_absolute(input));
#else
    ASSERT_FALSE(is_path_absolute(input));
#endif
}

UTEST_F(TestPath, path_get_normalized1) {
    StringView input = STRING_VIEW_EMPTY;
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(is_string_empty(P0));
#else
    ASSERT_TRUE(is_string_empty(P0));
#endif
}

UTEST_F(TestPath, path_get_normalized2) {
    StringView input = STR_LIT(".");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT(".")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT(".")));
#endif
}

UTEST_F(TestPath, path_get_normalized3) {
    StringView input = STR_LIT("..");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("..")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("..")));
#endif
}

UTEST_F(TestPath, path_get_normalized4) {
    StringView input = STR_LIT("file");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("file")));
#endif
}

UTEST_F(TestPath, path_get_normalized5) {
    StringView input = STR_LIT("file.txt");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("file.txt")));
#endif
}

UTEST_F(TestPath, path_get_normalized6) {
    StringView input = STR_LIT("folder\\sub\\file.txt");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/sub/file.txt")));
#endif
}

UTEST_F(TestPath, path_get_normalized7) {
    StringView input = STR_LIT("folder/sub/file.txt");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/sub/file.txt")));
#endif
}

UTEST_F(TestPath, path_get_normalized8) {
    StringView input = STR_LIT("./file");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("file")));
#endif
}

UTEST_F(TestPath, path_get_normalized9) {
    StringView input = STR_LIT("./folder/../file");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("file")));
#endif
}

UTEST_F(TestPath, path_get_normalized10) {
    StringView input = STR_LIT("folder//sub///file");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\sub\\file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/sub/file")));
#endif
}

UTEST_F(TestPath, path_get_normalized11) {
    StringView input = STR_LIT("/folder/./sub/../file");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("\\folder\\file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("/folder/file")));
#endif
}

UTEST_F(TestPath, path_get_normalized12) {
    StringView input = STR_LIT("C:\\folder\\.\\sub\\..\\file");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("C:\\folder\\file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("C:/folder/file")));
#endif
}

UTEST_F(TestPath, path_get_normalized13) {
    StringView input = STR_LIT("C:/folder/sub//file.txt");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("C:\\folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("C:/folder/sub/file.txt")));
#endif
}

UTEST_F(TestPath, path_get_normalized14) {
    StringView input = STR_LIT("\\\\server\\share\\folder\\..\\file");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("\\\\server\\share\\file")));
#else
    //printf("%.*s\n", (i32)P0.len, (const char*)P0.data);
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("//server/share/file")));
#endif
}

UTEST_F(TestPath, path_get_normalized15) {
    StringView input = STR_LIT("/folder//sub///file");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("\\folder\\sub\\file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("/folder/sub/file")));
#endif
}

UTEST_F(TestPath, path_get_normalized16) {
    StringView input = STR_LIT("folder/sub/.");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\sub")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/sub")));
#endif
}

UTEST_F(TestPath, path_get_normalized17) {
    StringView input = STR_LIT("folder/sub/..");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder")));
#endif
}

UTEST_F(TestPath, path_get_normalized18) {
    StringView input = STR_LIT(".hidden");
    P0 = path_get_normalized(input);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT(".hidden")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT(".hidden")));
#endif
}

// path_join_cstr

UTEST_F(TestPath, path_join_cstr1) {
    P0 = path_join_cstr("folder", "sub", "file.txt");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/sub/file.txt")));
#endif
}

UTEST_F(TestPath, path_join_cstr2) {
    P0 = path_join_cstr("folder/", "sub/file");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/\\sub/file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder//sub/file")));
#endif
}

UTEST_F(TestPath, path_join_cstr3) {
    P0 = path_join_cstr("C:\\folder", "sub\\file.txt");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("C:\\folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("C:\\folder/sub\\file.txt")));
#endif
}

UTEST_F(TestPath, path_join_cstr4) {
    P0 = path_join_cstr(NULL);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STRING_VIEW_EMPTY));
#else
    ASSERT_TRUE(string_eq_sv(P0, STRING_VIEW_EMPTY));
#endif
}

UTEST_F(TestPath, path_join_cstr5) {
    P0 = path_join_cstr("folder", "", "file");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/file")));
#endif
}

UTEST_F(TestPath, path_join_cstr6) {
    P0 = path_join_cstr("folder", "sub", "file.txt");
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/sub/file.txt")));
#endif
}

// path_join_sv

UTEST_F(TestPath, path_join_sv1) {
    P0 = path_join_sv(STR_LIT("folder"), STR_LIT("sub"), STR_LIT("file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/sub/file.txt")));
#endif
}

UTEST_F(TestPath, path_join_sv2) {
    P0 = path_join_sv(STR_LIT("folder/"), STR_LIT("sub/file"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/\\sub/file")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder//sub/file")));
#endif
}

UTEST_F(TestPath, path_join_sv3) {
    P0 = path_join_sv(STR_LIT("C:\\folder"), STR_LIT("sub\\file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("C:\\folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("C:\\folder/sub\\file.txt")));
#endif
}

UTEST_F(TestPath, path_join_sv4) {
    P0 = path_join_sv(STRING_VIEW_EMPTY);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STRING_VIEW_EMPTY));
#else
    ASSERT_TRUE(string_eq_sv(P0, STRING_VIEW_EMPTY));
#endif
}

UTEST_F(TestPath, path_join_sv5) {
    P0 = path_join_sv(STR_LIT("folder"), STRING_VIEW_EMPTY, STR_LIT("file"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder")));
#endif
}

UTEST_F(TestPath, path_join_sv6) {
    P0 = path_join_sv(STR_LIT("folder"), STR_LIT("sub"), STR_LIT("file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder\\sub\\file.txt")));
#else
    ASSERT_TRUE(string_eq_sv(P0, STR_LIT("folder/sub/file.txt")));
#endif
}

// path_get_basename

UTEST_F(TestPath, path_get_basename1) {
    StringView basename = path_get_basename(STR_LIT("file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("file.txt")));
#else
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("file.txt")));
#endif
}

UTEST_F(TestPath, path_get_basename2) {
    StringView basename = path_get_basename(STR_LIT("folder/sub/file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("file.txt")));
#else
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("file.txt")));
#endif
}

UTEST_F(TestPath, path_get_basename3) {
    StringView basename = path_get_basename(STR_LIT("C:\\folder\\file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("file.txt")));
#else
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("file.txt")));
#endif
}

UTEST_F(TestPath, path_get_basename4) {
    StringView basename = path_get_basename(STR_LIT("/folder/sub/"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("sub")));
#else
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("sub")));
#endif
}

UTEST_F(TestPath, path_get_basename5) {
    StringView basename = path_get_basename(STR_LIT("/"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("\\")));
#else
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT("/")));
#endif
}

UTEST_F(TestPath, path_get_basename6) {
    StringView basename = path_get_basename(STRING_VIEW_EMPTY);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(basename, STRING_VIEW_EMPTY));
#else
    ASSERT_TRUE(string_view_eq_sv(basename, STRING_VIEW_EMPTY));
#endif
}

UTEST_F(TestPath, path_get_basename7) {
    StringView basename = path_get_basename(STR_LIT(".hidden"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT(".hidden")));
#else
    ASSERT_TRUE(string_view_eq_sv(basename, STR_LIT(".hidden")));
#endif
}

// path_get_dir

UTEST_F(TestPath, path_get_dir1) {
    StringView dir = path_get_dir(STR_LIT("file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT(".")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT(".")));
#endif
}

UTEST_F(TestPath, path_get_dir2) {
    StringView dir = path_get_dir(STR_LIT("."));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT(".")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT(".")));
#endif
}

UTEST_F(TestPath, path_get_dir3) {
    StringView dir = path_get_dir(STR_LIT(".."));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("..")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("..")));
#endif
}

UTEST_F(TestPath, path_get_dir4) {
    StringView dir = path_get_dir(STR_LIT("folder/sub/file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("folder/sub")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("folder/sub")));
#endif
}

UTEST_F(TestPath, path_get_dir5) {
    StringView dir = path_get_dir(STR_LIT("C:\\folder\\file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("C:\\folder")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("C:\\folder")));
#endif
}

UTEST_F(TestPath, path_get_dir6) {
    StringView dir = path_get_dir(STR_LIT("\\\\server\\share\\file"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("\\\\server\\share")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("\\\\server\\share")));
#endif
}

UTEST_F(TestPath, path_get_dir7) {
    StringView dir = path_get_dir(STR_LIT("/folder/sub/"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("/folder")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT("/folder")));
#endif
}

UTEST_F(TestPath, path_get_dir8) {
    StringView dir = path_get_dir(STR_LIT("/"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT(".")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT(".")));
#endif
}

UTEST_F(TestPath, path_get_dir9) {
    StringView dir = path_get_dir(STRING_VIEW_EMPTY);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT(".")));
#else
    ASSERT_TRUE(string_view_eq_sv(dir, STR_LIT(".")));
#endif
}

// path_get_stem

UTEST_F(TestPath, path_get_stem1) {
    StringView stem = path_get_stem(STRING_VIEW_EMPTY);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(stem, STRING_VIEW_EMPTY));
#else
    ASSERT_TRUE(string_view_eq_sv(stem, STRING_VIEW_EMPTY));
#endif
}

UTEST_F(TestPath, path_get_stem2) {
    StringView stem = path_get_stem(STR_LIT("file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(stem, STR_LIT("file")));
#else
    ASSERT_TRUE(string_view_eq_sv(stem, STR_LIT("file")));
#endif
}

UTEST_F(TestPath, path_get_stem3) {
    StringView stem = path_get_stem(STR_LIT("archive.tar.gz"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(stem, STR_LIT("archive.tar")));
#else
    ASSERT_TRUE(string_view_eq_sv(stem, STR_LIT("archive.tar")));
#endif
}

UTEST_F(TestPath, path_get_stem4) {
    StringView stem = path_get_stem(STR_LIT(".hidden"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(stem, STR_LIT(".hidden")));
#else
    ASSERT_TRUE(string_view_eq_sv(stem, STR_LIT(".hidden")));
#endif
}

UTEST_F(TestPath, path_get_stem5) {
    StringView stem = path_get_stem(STR_LIT("folder/sub/file"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(stem, STR_LIT("file")));
#else
    ASSERT_TRUE(string_view_eq_sv(stem, STR_LIT("file")));
#endif
}

// path_get_ext

UTEST_F(TestPath, path_get_ext1) {
    StringView ext = path_get_ext(STR_LIT("file.txt"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(ext, STR_LIT(".txt")));
#else
    ASSERT_TRUE(string_view_eq_sv(ext, STR_LIT(".txt")));
#endif
}

UTEST_F(TestPath, path_get_ext2) {
    StringView ext = path_get_ext(STR_LIT("archive.tar.gz"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(ext, STR_LIT(".gz")));
#else
    ASSERT_TRUE(string_view_eq_sv(ext, STR_LIT(".gz")));
#endif
}

UTEST_F(TestPath, path_get_ext3) {
    StringView ext = path_get_ext(STR_LIT("file"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(ext, STRING_VIEW_EMPTY));
#else
    ASSERT_TRUE(string_view_eq_sv(ext, STRING_VIEW_EMPTY));
#endif
}

UTEST_F(TestPath, path_get_ext4) {
    StringView ext = path_get_ext(STR_LIT(".hidden"));
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(ext, STRING_VIEW_EMPTY));
#else
    ASSERT_TRUE(string_view_eq_sv(ext, STRING_VIEW_EMPTY));
#endif
}

UTEST_F(TestPath, path_get_ext5) {
    StringView ext = path_get_ext(STRING_VIEW_EMPTY);
#if defined(PLATFORM_WINDOWS)
    ASSERT_TRUE(string_view_eq_sv(ext, STRING_VIEW_EMPTY));
#else
    ASSERT_TRUE(string_view_eq_sv(ext, STRING_VIEW_EMPTY));
#endif
}
