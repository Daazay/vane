#include "vane/utils/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdarg.h>

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#include "vane/utils/win.h"
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

static DWORD win_path_get_attrs(StringView path) {
    u64 u16_len = 0;
    u16* u16_buf = NULL;
    if (!win_utf8_to_utf16_alloc(path, &u16_buf, &u16_len)) {
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
    u16* u16_buf = NULL;
    if (!win_utf8_to_utf16_alloc(path, &u16_buf, &u16_len)) {
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

    String result = win_utf16_to_utf8_str(abs_buf, (u64)wrote);
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

static inline u64 path_root_len(StringView path) {
    // Always detect UNC
    if (is_path_unc(path)) {
        u64 i = 2;
        while (i < path.len && !is_path_sep(path.data[i])) {
            ++i;
        }
        if (i >= path.len) {
            return path.len;
        }
        ++i;
        while (i < path.len && !is_path_sep(path.data[i])) {
            ++i;
        }
        if (i < path.len) {
            ++i;
        }
        return i;
    }

#if defined(PLATFORM_WINDOWS)
    // Drive root: C:\ or C:/
    if (is_path_win_drive_root(path)) {
        if (path.len >= 3 && is_path_sep(path.data[2])) {
            return 3;
        }
        return 2;
    }
#endif
    // POSIX absolute root or Windows root '/'
    if (path.len >= 1 && is_path_sep(path.data[0])) {
        return 1;
    }
    return 0;
}

static inline void split_components_after_root(StringView p, u64 root_len, Vector* out) {
    assert(out != NULL);

    u64 i = root_len;

    while (i < p.len) {
        i = path_skip_separators(p, i);
        if (i >= p.len) {
            break;
        }
        u64 start = i;
        i = path_find_next_separator(p, i);
        StringView comp = string_view_subview(p, start, i - start);

        // skip "." and collapse ".." when possible (like normalization does)
        if (string_view_eq_sv(comp, STR_LIT("."))) {
            continue;
        }

        if (string_view_eq_sv(comp, STR_LIT(".."))) {
            if (out->size > 0) {
                const StringView* last = vector_at_back(*out);
                if (!string_view_eq_sv(*last, STR_LIT(".."))) {
                    vector_pop_back(out);
                    continue;
                }
            }
        }
        vector_push_back(out, &comp);
    }
}

String path_get_relative(StringView base, StringView target) {
    if (is_string_view_empty(base)) {
        return path_get_absolute(target);
    }
    if (is_string_view_empty(target)) {
        return STRING_EMPTY;
    }

    String abs_base   = path_get_absolute(base);
    String abs_target = path_get_absolute(target);

    StringView abs_base_sv   = string_get_view(abs_base);
    StringView abs_target_sv = string_get_view(abs_target);

    u64 broot = path_root_len(abs_base_sv);
    u64 troot = path_root_len(abs_target_sv);

    // if roots differ, cannot relativize
#if defined(PLATFORM_WINDOWS)
    // compare UNC/drive roots case-insensitively on Windows
    if (broot != troot || troot == 0 || broot == 0) {
        // additional check: try to compare drive or UNC share when both non-zero
        bool same_root = false;
        if (broot > 0 && troot > 0) {
            StringView br = string_view_subview(abs_base_sv, 0, broot);
            StringView tr = string_view_subview(abs_target_sv, 0, troot);
            same_root = string_view_eq_sv(br, tr);
        }
        if (!same_root) {
            string_destroy(&abs_base);
            return abs_target;
        }
    } else {
        // same length roots: ensure equal
        StringView br = string_view_subview(abs_base_sv, 0, broot);
        StringView tr = string_view_subview(abs_target_sv, 0, troot);
        if (!string_view_eq_sv(br, tr)) {
            string_destroy(&abs_base);
            return abs_target;
        }
    }
#else
    // POSIX: simple byte compare of roots
    if (broot != troot || !string_view_eq_bytes(string_view_subview(abs_base_sv, 0, broot), abs_target_sv.data, troot)) {
        string_destroy(&abs_base);
        return abs_target;
    }
#endif

    // split components after root
    Vector bcomps = vector_create(4, VECTOR_SPECS(StringView, NULL));
    Vector tcomps = vector_create(4, VECTOR_SPECS(StringView, NULL));

    split_components_after_root(abs_base_sv, broot, &bcomps);
    split_components_after_root(abs_target_sv, troot, &tcomps);

    // find common prefix length
    u32 common = 0;
    u32 blen = bcomps.size;
    u32 tlen = tcomps.size;

    while (common < blen && common < tlen) {
        StringView* bc = vector_at(bcomps, common);
        StringView* tc = vector_at(tcomps, common);

        if (!string_view_eq_sv(*bc, *tc)) {
            break;
        }
        ++common;
    }

    // build relative: for each remaining base component → "..", then append target remainder
    StringBuilder sb = string_builder_create(64);

    u32 up_count = blen - common;
    for (u32 i = 0; i < up_count; ++i) {
        if (sb.len > 0) {
            string_builder_append_c(&sb, PATH_SEP);
        }
        string_builder_append_cstr(&sb, "..");
    }

    for (u32 i = common; i < tlen; ++i) {
        StringView* tc = vector_at(tcomps, i);
        if (sb.len > 0) {
            string_builder_append_c(&sb, PATH_SEP);
        }
        string_builder_append_sv(&sb, *tc);
    }

    // same path
    if (sb.len == 0) {
        string_builder_append_c(&sb, '.');
    }

    String rel = string_builder_release(&sb);

    vector_destroy(&bcomps);
    vector_destroy(&tcomps);
    string_destroy(&abs_base);
    string_destroy(&abs_target);

    return rel;
}

String path_get_cwd() {
#if defined (PLATFORM_WINDOWS)
    u16 buf[PATH_MAX] = {0};
    u64 len = GetCurrentDirectoryW(ARR_SIZE(buf), buf);
    if (len == 0 || len >= PATH_MAX) {
        return STRING_EMPTY;
    }
    return win_utf16_to_utf8_str(buf, (u64)len);
#else
    char buf[PATH_MAX] = { 0 };
    if (!getcwd(buf, sizeof(buf))) {
        return STRING_EMPTY;
    }
    return string_from_cstr(buf);
#endif
}

String path_join_cstr_impl(const char* paths[], u32 count) {
    if (paths == NULL || count == 0) {
        return STRING_EMPTY;
    }

    StringBuilder sb = string_builder_create(32);
    bool first = true;

    for (u32 i = 0; i < count; ++i) {
        StringView sv = string_view_from_cstr(paths[i]);
        if (is_string_view_empty(sv)) {
            continue;
        }

        path_builder_append_sv_with_sep(&sb, sv, !first);
        first = false;
    }

    string_builder_shrink_to_fit(&sb);
    return string_builder_release(&sb);
}

String path_join_sv_impl(const StringView paths[], u32 count) {
    if (paths == NULL || count == 0) {
        return STRING_EMPTY;
    }

    // quick capacity guess
    u64 cap = 0;
    for (u32 i = 0; i < count; ++i) {
        cap += paths[i].len + 1;
    }

    StringBuilder sb = string_builder_create(cap ? (u64)cap : 32);
    bool first = true;

    for (u32 i = 0; i < count; ++i) {
        if (is_string_view_empty(paths[i])) {
            continue;
        }

        path_builder_append_sv_with_sep(&sb, paths[i], !first);
        first = false;
    }

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