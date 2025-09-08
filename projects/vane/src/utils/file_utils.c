#include "vane/utils/file_utils.h"

#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include "vane/utils/path.h"
#include "vane/utils/string.h"
#include "vane/utils/string_builder.h"

static inline u64 strip_utf8_dom(const u8* buf, u64 len) {
    if (len >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
        return 3;
    }
    return 0;
}

#if defined(PLATFORM_WINDOWS)

static u64 filetime_to_unixtime(const FILETIME* ft) {
    ULARGE_INTEGER ull = { 0 };
    ull.LowPart = ft->dwLowDateTime;
    ull.HighPart = ft->dwHighDateTime;
    return (ull.QuadPart / 10000000) - 11644473600LL;
}

static u16* file_win_utf8_to_utf16(StringView path, u64* out_len) {
    *out_len = convert_utf8_to_utf16(path.data, path.len, NULL, 0);

    if (*out_len == UNICODE_INVALID_LEN) {
        return NULL;
    }

    u16* buf = malloc((*out_len + 1) * sizeof(u16));
    assert(buf != NULL);

    convert_utf8_to_utf16(path.data, path.len, buf, *out_len);
    buf[*out_len] = 0;

    return buf;
}

static String file_win_utf16_to_utf8(const u16* buf, u64 len) {
    u64 u8_len = convert_utf16_to_utf8(buf, len, NULL, 0);

    if (u8_len == UNICODE_INVALID_LEN) {
        return STRING_EMPTY;
    }

    StringBuilder sb = string_builder_create(u8_len);
    sb.len = convert_utf16_to_utf8(buf, len, sb.data, u8_len);

    return string_builder_release(&sb);
}

static inline FileLoadStatus read_file_content_win(StringView path, u8** out, u64* out_len) {
    assert(out != NULL && out_len != NULL);

    *out = NULL;
    *out_len = 0;

    // Convert UTF8 to UTF16
    u64 u16_len = 0;
    u16* u16_buf = file_win_utf8_to_utf16(path, &u16_len);
    if (u16_buf == NULL || u16_len == UNICODE_INVALID_LEN) {
        return FILE_LOAD_ERR_INVALID_PATH;
    }

    HANDLE h = CreateFileW((LPCWSTR)u16_buf, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    free(u16_buf);

    if (h == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        switch (err) {
        case ERROR_PATH_NOT_FOUND:
        case ERROR_FILE_NOT_FOUND:
            return FILE_LOAD_ERR_NOT_FOUND;
        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
            return FILE_LOAD_ERR_ACCESS_DENIED;
        case ERROR_DIRECTORY:
            return FILE_LOAD_ERR_IS_DIR;
        default:
            return FILE_LOAD_ERR_OPEN;
        }
    }

    LARGE_INTEGER li = { 0 };
    if (!GetFileSizeEx(h, &li)) {
        CloseHandle(h);
        return FILE_LOAD_ERR_READ;
    }

    if (li.QuadPart == 0) {
        CloseHandle(h);
        return FILE_LOAD_ERR_EMPTY_CONTENT;
    }

    u64 size = (u64)li.QuadPart;
    u8* buf = malloc(size + 1);
    assert(buf != NULL);

    DWORD did_read = 0;
    BOOL read_status = ReadFile(h, buf, (DWORD)size, &did_read, NULL);
    CloseHandle(h);

    if (!read_status || did_read != size) {
        free(buf);
        return FILE_LOAD_ERR_READ;
    }

    buf[size] = 0;
    *out = buf;
    *out_len = size;

    return FILE_LOAD_OK;
}

static DirListStatus directory_list_win(StringView path, Vector* entries, bool recursive) {
    assert(entries != NULL);

    // Build search pattern
    StringBuilder pattern = string_builder_create(path.len + 2);
    string_builder_append_sv(&pattern, path);
    string_builder_append_c(&pattern, '\\');
    string_builder_append_c(&pattern, '*');

    // Convert pattern to UTF16
    u64 u16_pattern_len = 0;
    u16* u16_pattern = file_win_utf8_to_utf16(string_builder_get_view(pattern), &u16_pattern_len);
    string_builder_destroy(&pattern);

    if (u16_pattern == NULL || u16_pattern_len == UNICODE_INVALID_LEN) {
        return DIR_LIST_ERR_INVALID_PATH;
    }

    WIN32_FIND_DATAW find_data = { 0 };
    HANDLE h = FindFirstFileW((LPCWSTR)u16_pattern, &find_data);
    free(u16_pattern);

    if (h == INVALID_HANDLE_VALUE) {
        switch (GetLastError()) {
        case ERROR_FILE_NOT_FOUND: return DIR_LIST_ERR_NOT_FOUND;
        case ERROR_PATH_NOT_FOUND: return DIR_LIST_ERR_INVALID_PATH;
        case ERROR_ACCESS_DENIED:  return DIR_LIST_ERR_ACCESS_DENIED;
        default: return DIR_LIST_ERR_OPEN;
        }
    }

    DirListStatus status = DIR_LIST_OK;

    do {
        // Skip "." and ".." directories
        if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0) {
            continue;
        }

        // Convert filename to UTF8
        String filename = file_win_utf16_to_utf8((const u16*)find_data.cFileName, wcslen(find_data.cFileName));

        if (is_string_empty(filename)) {
            continue;
        }

        // Build full path using path_join_sv
        String fullpath = path_join_sv(path, string_get_view(filename));
        string_destroy(&filename);

        DirEntry entry = {
            .fullpath = fullpath,
            .size = ((u64)find_data.nFileSizeHigh << 32) | find_data.nFileSizeLow,
            .is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0,
            .modified_time = filetime_to_unixtime(&find_data.ftLastWriteTime),
        };

        vector_push_back(entries, &entry);

        if (recursive && entry.is_dir) {
            status = directory_list_win(string_get_view(fullpath), entries, true);
            if (status != DIR_LIST_OK) {
                break;
            }
        }
    } while (FindNextFileW(h, &find_data));

    FindClose(h);
    return status;
}

#else

static inline FileLoadStatus read_file_content_unix(StringView path, u8** out, u64* out_len) {
    assert(out != NULL && out_len != NULL);

    *out = NULL;
    *out_len = 0;

    i32 fd = open((const char*)path.data, O_RDONLY);
    if (fd < 0) {
        switch (errno) {
        case ENOENT:
            return FILE_LOAD_ERR_NOT_FOUND;
        case EACCES:
            return FILE_LOAD_ERR_ACCESS_DENIED;
        case EISDIR:
            return FILE_LOAD_ERR_IS_DIR;
        default:
            return FILE_LOAD_ERR_OPEN;
        }
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return (errno == EACCES)
            ? FILE_LOAD_ERR_ACCESS_DENIED
            : FILE_LOAD_ERR_READ;
    }

    if (S_ISDIR(st.st_mode)) {
        close(fd);
        return FILE_LOAD_ERR_IS_DIR;
    }

    u64 size = (u64)st.st_size;
    if (size == 0) {
        close(fd);
        return FILE_LOAD_ERR_EMPTY_CONTENT;
    }

    u8* buf = malloc(size + 1);
    assert(buf != NULL);

    u64 read_total = 0;
    while (read_total < size) {
        i64 readed = read(fd, buf + read_total, (u64)(size - read_total));
        if (readed < 0) {
            free(buf);
            close(fd);
            return FILE_LOAD_ERR_READ;
        }
        if (readed == 0) {
            break;
        }
        read_total += (u64)readed;
    }
    close(fd);

    buf[read_total] = 0;
    *out = buf;
    *out_len = read_total;

    return FILE_LOAD_OK;
}

static DirListStatus directory_list_unix(StringView path, Vector* entries, bool recursive) {
    assert(entries != NULL);

    DIR* dir = opendir((const char*)path.data);

    if (dir == NULL) {
        switch (errno) {
        case ENOENT:  return DIR_LIST_ERR_NOT_FOUND;
        case ENOTDIR: return DIR_LIST_ERR_NOT_DIR;
        case EACCES:  return DIR_LIST_ERR_ACCESS_DENIED;
        default: return DIR_LIST_ERR_OPEN;
        }
    }

    DirListStatus status = DIR_LIST_OK;
    struct dirent* entry_ptr = NULL;

    while ((entry_ptr = readdir(dir)) != NULL) {
        StringView filename = string_view_from_cstr(entry_ptr->d_name);

        if (string_view_eq_sv(filename, STR_LIT(".")) ||
            string_view_eq_sv(filename, STR_LIT(".."))) {
            continue;
        }

        // Build full path using path_join_sv
        String fullpath = path_join_sv(path, filename);

        struct stat st;
        if (stat((const char*)fullpath.data, &st) != 0) {
            string_destroy(&fullpath);
            continue;
        }

        DirEntry entry = {
           .fullpath = fullpath,
           .size = (u64)st.st_size,
           .is_dir = S_ISDIR(st.st_mode),
           .modified_time = (u64)st.st_mtime
        };

        vector_push_back(entries, &entry);

        if (recursive && entry.is_dir) {
            status = directory_list_unix(string_get_view(fullpath), entries, true);
            if (status != DIR_LIST_OK) {
                break;
            }
        }
    }

    closedir(dir);
    return status;
}

#endif

void dir_entry_destroy(DirEntry* entry) {
    if (entry == NULL) {
        return;
    }

    string_destroy(&entry->fullpath);
}

FileLoadStatus file_content_load(StringView path, u8** data, u64* size) {
    assert(data != NULL && size != NULL);

    u8* raw = NULL;
    u64 raw_len = 0;

#if defined(PLATFORM_WINDOWS)
    FileLoadStatus status = read_file_content_win(path, &raw, &raw_len);
#else
    FileLoadStatus status = read_file_content_unix(path, &raw, &raw_len);
#endif
    if (status != FILE_LOAD_OK) {
        return status;
    }

    u64 bom_len = strip_utf8_dom(raw, raw_len);
    if (bom_len > 0) {
        u64 content_len = raw_len - bom_len;
        memmove(raw, raw + bom_len, content_len);
        raw[content_len] = 0;
        *size = content_len;
    }
    else {
        *size = raw_len;
    }

    *data = raw;
    return status;
}

DirListStatus directory_list(StringView path, Vector* entries, bool recursive) {
    assert(entries != NULL);

    if (is_string_view_empty(path)) {
        return DIR_LIST_ERR_INVALID_PATH;
    }

    *entries = vector_create(4, VECTOR_SPECS(DirEntry, &dir_entry_destroy));
#if defined(PLATFORM_WINDOWS)
    DirListStatus status = directory_list_win(path, entries, recursive);
#else
    DirListStatus status = directory_list_unix(path, entries, recursive);
#endif

    if (status != DIR_LIST_OK) {
        vector_destroy(entries);
    }

    return status;
}