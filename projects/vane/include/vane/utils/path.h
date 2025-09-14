#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string_view.h"
#include "vane/utils/string.h"

#if defined(PLATFORM_WINDOWS)
#define PATH_SEP         '\\'
#define PATH_SEP_STR     "\\"
#define ALT_PATH_SEP     '/'
#define ALT_PATH_SEP_STR "/"
#else
#define ALT_PATH_SEP     '\\'
#define ALT_PATH_SEP_STR "\\"
#define PATH_SEP         '/'
#define PATH_SEP_STR     "/"
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static inline bool is_path_sep(char c) {
    return (c == PATH_SEP) || (c == ALT_PATH_SEP);
}

String path_get_absolute(StringView path);

String path_get_normalized(StringView path);

String path_get_relative(StringView base, StringView target);

String path_get_cwd();

String path_join_cstr_impl(const char* paths[], u32 count);

#define path_join_cstr(...) path_join_cstr_impl(((const char*[]){ __VA_ARGS__ }), ARR_SIZE(((const char*[]){ __VA_ARGS__ })))

String path_join_sv_impl(const StringView paths[], u32 count);

#define path_join_sv(...) path_join_sv_impl(((const StringView[]){ __VA_ARGS__ }), ARR_SIZE(((const StringView[]){ __VA_ARGS__ })))

StringView path_get_dir(StringView path);

StringView path_get_basename(StringView path);

StringView path_get_stem(StringView path);

StringView path_get_ext(StringView path);

bool is_path_absolute(StringView path);

bool is_path_exist(StringView path);

bool is_path_dir(StringView path);

bool is_path_file(StringView path);