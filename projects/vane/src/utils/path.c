#include "vane/utils/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdarg.h>

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#endif

#include "vane/utils/vector.h"
#include "vane/utils/string_utils.h"
#include "vane/utils/string_builder.h"

static u64 path_skip_separators(StringView path, u64 offset) {
    while (offset < path.len && is_path_sep(path.data[offset])) {
        offset++;
    }
    return offset;
}

static u64 path_find_next_separator(StringView path, u64 offset) {
    while (offset < path.len && !is_path_sep(path.data[offset])) {
        offset++;
    }
    return offset;
}

static bool is_path_unc(StringView path) {
    if (path.len < 5) {
        return false;
    }
    if (!(is_path_sep(path.data[0]) && is_path_sep(path.data[1]))) {
        return false;
    }

    u64 server_end = path_find_next_separator(path, 2);
    if (server_end <= 2) {
        return false;
    }

    u64 share_end = path_find_next_separator(path, server_end + 1);
    return share_end > server_end + 1;
}

#if defined(PLATFORM_WINDOWS)

static bool is_path_win_drive_root(StringView path) {
    return path.len >= 3 &&
        is_alpha(path.data[0]) &&
        (path.data[1] == ':') &&
        is_path_sep(path.data[2]);
}

static u16* win_utf8_to_utf16(StringView path, u64* out_len) {
    *out_len = convert_utf8_to_utf16(path.data, path.len, NULL, 0);
    if (*out_len == (u64)UNICODE_INVALID_LEN) {
        return NULL;
    }

    u16* buf = malloc((*out_len + 1) * sizeof(u16));
    assert(buf != NULL);

    convert_utf8_to_utf16(path.data, path.len, buf, *out_len);
    buf[*out_len] = 0;

    return buf;
}

static String win_utf16_to_utf8(const u16* buf, u64 len) {
    u64 u8_len = convert_utf16_to_utf8(buf, len, NULL, 0);
    if (u8_len == (u64)UNICODE_INVALID_LEN) {
        return STRING_EMPTY;
    }

    StringBuilder sb = string_builder_create(u8_len);
    sb.len = convert_utf16_to_utf8(buf, len, sb.data, u8_len);
    sb.data[sb.len] = 0;

    return string_builder_release(&sb);
}

static DWORD win_path_get_attrs(StringView path) {
    u64 u16_len = 0;
    u16* u16_buf = win_utf8_to_utf16(path, &u16_len);
    if (u16_buf == NULL || u16_len == UNICODE_INVALID_LEN) {
        return INVALID_FILE_ATTRIBUTES;
    }

    DWORD attrs = GetFileAttributesW(u16_buf);
    free(u16_buf);

    return attrs;
}

#endif

static void path_builder_append_sv_with_sep(StringBuilder* sb, StringView sv, bool add_sep) {
    if (sb->len > 0 && add_sep && !is_path_sep(sb->data[sb->len - 1])) {
        string_builder_append_c(sb, PATH_SEP);
    }
    string_builder_append_sv(sb, sv);
}

String path_get_absolute(StringView path) {
    if (is_string_view_empty(path)) {
        return STRING_EMPTY;
    }

    if (is_path_absolute(path)) {
        return string_from_sv(path);
    }

#if defined(PLATFORM_WINDOWS)
    // convert UTF8 to UTF16 for Windows API
    u64 u16_len = 0;
    u16* u16_buf = win_utf8_to_utf16(path, &u16_len);
    if (u16_buf == NULL || u16_len == (u64)UNICODE_INVALID_LEN) {
        return STRING_EMPTY;
    }

    // get absolute path length
    DWORD need = GetFullPathNameW((LPCWSTR)u16_buf, 0, NULL, NULL);
    if (need == 0) {
        free(u16_buf);
        return STRING_EMPTY;
    }

    u16* abs_buf = malloc((u64)need * sizeof(u16));
    assert(abs_buf != NULL);

    DWORD wrote = GetFullPathNameW((LPCWSTR)u16_buf, need, (LPWSTR)abs_buf, NULL);
    free(u16_buf);

    String result = win_utf16_to_utf8(abs_buf, (u64)wrote);
    free(abs_buf);

    return result;
#else
    char cwd[PATH_MAX] = {0};
    if (!getcwd(cwd, sizeof(cwd))) {
        return STRING_EMPTY;
    }

    StringView cwd_sv = string_view_from_cstr(cwd);
    String joined = path_join_sv(cwd_sv, path);
    String normalized = path_get_normalized(string_get_view(joined));
    string_destroy(&joined);

    return normalized;
#endif
}

String path_get_normalized(StringView path) {
    if (is_string_view_empty(path)) {
        return STRING_EMPTY;
    }

    Vector comps = vector_create(4, VECTOR_SPECS(StringView, NULL));
    u64 i = 0;

    // Extract prefixes (UNC, Windows drive)
    StringView prefixes[2] = { STRING_VIEW_EMPTY, STRING_VIEW_EMPTY, };
    bool is_abs = is_path_absolute(path);

    if (is_path_unc(path)) {
        u64 server_end = path_find_next_separator(path, 2);
        u64 share_end = path_find_next_separator(path, server_end + 1);

        prefixes[0] = string_view_subview(path, 2, server_end - 2);
        prefixes[1] = string_view_subview(path, server_end + 1, share_end - (server_end + 1));

        i = share_end;
    }
#if defined(PLATFORM_WINDOWS)
    else if (is_path_win_drive_root(path)) {
        prefixes[0] = string_view_subview(path, 0, 2);
        i = 2;
    }
#endif

    while (i < path.len) {
        i = path_skip_separators(path, i);
        if (i >= path.len) {
            break;
        }

        u64 start = i;
        i = path_find_next_separator(path, i);
        StringView comp = string_view_subview(path, start, i - start);

        if (string_view_eq_sv(comp, STR_LIT("."))) {
            continue;
        }

        if (string_view_eq_sv(comp, STR_LIT(".."))) {
            if (comps.size > 0) {
                const StringView* last = vector_at_back(comps);
                if (!string_view_eq_sv(*last, STR_LIT(".."))) {
                    vector_pop_back(&comps);
                    continue;
                }
            }

            if (!is_string_view_empty(prefixes[0])) {
                continue;
            }
        }

        vector_push_back(&comps, &comp);
    }

    // Build normalized path
    StringBuilder sb = string_builder_create(64);

    if (!is_string_view_empty(prefixes[0])) {
        if (!is_string_view_empty(prefixes[1])) {
            string_builder_append_c(&sb, PATH_SEP);
            string_builder_append_c(&sb, PATH_SEP);
            string_builder_append_sv(&sb, prefixes[0]);
            string_builder_append_c(&sb, PATH_SEP);
            string_builder_append_sv(&sb, prefixes[1]);
        }
        else {
            string_builder_append_sv(&sb, prefixes[0]);
        }
        string_builder_append_c(&sb, PATH_SEP);
    }
    else if (is_abs) {
        string_builder_append_c(&sb, PATH_SEP);
    }


    // Add components
    for (u32 j = 0; j < comps.size; ++j) {
        const StringView* p = vector_at(comps, j);
        path_builder_append_sv_with_sep(&sb, *p, true);
    }

    // Handle empty path
    if (sb.len == 0) {
        string_builder_append_c(&sb, '.');
    }

    vector_destroy(&comps);

    string_builder_shrink_to_fit(&sb);
    return string_builder_release(&sb);
}

String path_get_cwd() {
#if defined (PLATFORM_WINDOWS)
    u16 buf[PATH_MAX] = {0};
    u64 len = GetCurrentDirectoryW(ARR_SIZE(buf), buf);
    if (len == 0 || len >= PATH_MAX) {
        return STRING_EMPTY;
    }
    return win_utf16_to_utf8(buf, len);
#else
    char buf[PATH_MAX] = { 0 };
    if (!getcwd(buf, sizeof(buf))) {
        return STRING_EMPTY;
    }
    return string_from_cstr(buf);
#endif
}

String path_join_cstr_impl(const char* path0, ...) {
    va_list va;
    va_start(va, path0);

    StringBuilder sb = string_builder_create(32);
    const char* p = path0;
    bool first = true;

    while (p != NULL) {
        StringView sv = string_view_from_cstr(p);
        p = va_arg(va, const char*);

        if (is_string_view_empty(sv)) {
            continue;
        }

        path_builder_append_sv_with_sep(&sb, sv, !first);
        first = false;
    }

    va_end(va);

    string_builder_shrink_to_fit(&sb);
    return string_builder_release(&sb);
}

String path_join_sv_impl(StringView path0, ...) {
    va_list va;
    va_start(va, path0);

    StringBuilder sb = string_builder_create(32);
    StringView path = path0;
    bool first = true;

    while (!is_string_view_empty(path)) {
        path_builder_append_sv_with_sep(&sb, path, !first);
        first = false;
        path = va_arg(va, StringView);
    }

    va_end(va);

    string_builder_shrink_to_fit(&sb);
    return string_builder_release(&sb);
}

StringView path_get_dir(StringView path) {
    if (is_string_view_empty(path)) {
        return STR_LIT(".");
    }
    if (string_view_eq_sv(path, STR_LIT("."))) {
        return STR_LIT(".");
    }
    if (string_view_eq_sv(path, STR_LIT(".."))) {
        return STR_LIT("..");
    }

    // Trim trailing separators
    u64 end = path.len;
    while (end > 0 && is_path_sep(path.data[end - 1])) {
        end--;
    }

    // Find last separator
    u64 last_sep = (u64)NPOS;
    for (u64 i = end; i-- > 0; ) {
        if (is_path_sep(path.data[i])) {
            last_sep = i;
            break;
        }
    }

    if (last_sep == (u64)NPOS) {
        return STR_LIT(".");
    }

    // skip trailing separators
    while (last_sep > 0 && is_path_sep(path.data[last_sep - 1])) {
        last_sep--;
    }

    if (last_sep == 0) {
        if (is_path_absolute(path)) {
            return STR_LIT(PATH_SEP_STR);
        }
        return STR_LIT(".");
    }

    return string_view_subview(path, 0, last_sep);
}

StringView path_get_basename(StringView path) {
    if (is_string_view_empty(path)) {
        return STRING_VIEW_EMPTY;
    }

    u64 i = path.len;
    while (i > 0 && is_path_sep(path.data[i - 1])) {
        --i;
    }

    if (i == 0) {
        if (is_path_absolute(path)) {
            return STR_LIT(PATH_SEP_STR);
        }
        return STRING_VIEW_EMPTY;
    }

    u64 start = i;
    while (start > 0 && !is_path_sep(path.data[start - 1])) {
        --start;
    }

    return string_view_subview(path, start, i - start);
}

StringView path_get_stem(StringView path) {
    StringView basename = path_get_basename(path);
    if (is_string_view_empty(basename)) {
        return STRING_VIEW_EMPTY;
    }

    u64 dot = string_view_find_last_c(basename, '.');
    if (dot == (u64)NPOS || dot == 0) {
        return basename;
    }

    return string_view_subview(basename, 0, dot);
}

StringView path_get_ext(StringView path) {
    StringView basename = path_get_basename(path);
    if (is_string_view_empty(basename)) {
        return STRING_VIEW_EMPTY;
    }

    u64 dot = string_view_find_last_c(basename, '.');
    if (dot == (u64)NPOS || dot == 0) {
        return STRING_VIEW_EMPTY;
    }

    return string_view_subview(basename, dot, basename.len - dot);
}

bool is_path_absolute(StringView path) {
    if (is_string_view_empty(path)){
        return false;
    }
#if defined(PLATFORM_WINDOWS)
    return is_path_win_drive_root(path) || is_path_unc(path) || is_path_sep(path.data[0]);
#else
    return is_path_unc(path) || path.data[0] == '/';
#endif
}

bool is_path_exist(StringView path) {
    if (is_string_view_empty(path)) {
        return false;
    }

#if defined(PLATFORM_WINDOWS)
    return win_path_get_attrs(path) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat((const char*)path.data, &st) == 0;
#endif
}

bool is_path_dir(StringView path) {
    if (is_string_view_empty(path)) {
        return false;
    }

#if defined(PLATFORM_WINDOWS)
    DWORD attrs = win_path_get_attrs(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat((const char*)path.data, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

bool is_path_file(StringView path) {
    if (is_string_view_empty(path)) {
        return false;
    }

#if defined(PLATFORM_WINDOWS)
    DWORD attrs = win_path_get_attrs(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat((const char*)path.data, &st) == 0 && S_ISREG(st.st_mode);
#endif
}