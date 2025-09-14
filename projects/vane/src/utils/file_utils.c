#include "vane/utils/file_utils.h"

#include <stdlib.h>
#include <stdio.h>
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

static inline FileLoadStatus read_file_content_win(StringView path, u8** out, u64* out_len) {
    assert(out != NULL && out_len != NULL);

    *out = NULL;
    *out_len = 0;

    // Convert UTF8 to UTF16
    String u16 = STRING_EMPTY;
    if (!string_view_utf8_to_utf16_str(path, &u16)) {
        return FILE_LOAD_ERR_INVALID_PATH;
    }

    HANDLE h = CreateFileW((LPCWSTR)u16.data, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    string_destroy(&u16);

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
    String u16_pattern = STRING_EMPTY;
    if (!string_view_utf8_to_utf16_str(string_builder_get_view(pattern), &u16_pattern)) {
        string_builder_destroy(&pattern);
        return DIR_LIST_ERR_INVALID_PATH;
    }

    string_builder_destroy(&pattern);

    WIN32_FIND_DATAW find_data = { 0 };
    HANDLE h = FindFirstFileW((LPCWSTR)u16_pattern.data, &find_data);
    string_destroy(&u16_pattern);

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

        StringView sv = string_view_create((const u8*)find_data.cFileName, wcslen(find_data.cFileName) * sizeof(u16));

        // Convert filename to UTF8
        String filename = STRING_EMPTY;
        string_view_utf16_to_utf8_str(sv, &filename);

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

static inline void dir_entry_destroy(DirEntry* entry) {
    if (entry == NULL) {
        return;
    }

    string_destroy(&entry->fullpath);
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

DirCreateStatus directory_create(StringView path) {
    if (is_string_view_empty(path)) {
        return DIR_CREATE_ERR_INVALID_PATH;
    }

#if defined(PLATFORM_WINDOWS)
    String wpath = STRING_EMPTY;
    if (!string_view_utf8_to_utf16_str(path, &wpath)) {
        return DIR_CREATE_ERR_INVALID_PATH;
    }

    BOOL ok = CreateDirectoryW((LPCWSTR)wpath.data, NULL);
    DWORD err = GetLastError();
    string_destroy(&wpath);

    if (ok) {
        return DIR_CREATE_OK;
    }

    switch (err) {
    case ERROR_ALREADY_EXISTS: return DIR_CREATE_ERR_EXISTS;
    case ERROR_ACCESS_DENIED:  return DIR_CREATE_ERR_ACCESS_DENIED;
    default:                   return DIR_CREATE_ERR_FAILED;
    }
#else
    if (mkdir((const char*)path.data, 0755) == 0) {
        return DIR_CREATE_OK;
    }
    switch (errno) {
    case EEXIST: return DIR_CREATE_ERR_EXISTS;
    case EACCES: return DIR_CREATE_ERR_ACCESS_DENIED;
    case ENOENT: return DIR_CREATE_ERR_INVALID_PATH;
    default:     return DIR_CREATE_ERR_FAILED;
    }
#endif
}
DirCreateStatus directory_create_recursive(StringView path) {
    if (is_string_view_empty(path)) {
        return DIR_CREATE_ERR_INVALID_PATH;
    }

#if defined(PLATFORM_WINDOWS)
    // Convert UTF-8 -> UTF-16
    String wpath = STRING_EMPTY;
    if (!string_view_utf8_to_utf16_str(path, &wpath)) {
        return DIR_CREATE_ERR_INVALID_PATH;
    }

    u16* buf = (u16*)wpath.data;

    // Determine root length: skip roots so we don't try to create them
    u64 root_len = 0;
    if (wpath.len >= 2 && buf[1] == L':') {
        // "C:" or "C:\"
        root_len = (wpath.len >= 3 && (buf[2] == L'\\' || buf[2] == L'/')) ? 3 : 2;
    }
    else if (wpath.len >= 2 && buf[0] == L'\\' && buf[1] == L'\\') {
        // UNC: \\server\share\...
        u64 i = 2;
        // server
        while (i < wpath.len && buf[i] != L'\\' && buf[i] != L'/') {
            ++i;
        }
        if (i < wpath.len) {
            ++i;
        }
        // share
        while (i < wpath.len && buf[i] != L'\\' && buf[i] != L'/') {
            ++i;
        }
        if (i < wpath.len) {
            ++i;
        }
        root_len = i;
    }
    else if (wpath.len >= 1 && (buf[0] == L'\\' || buf[0] == L'/')) {
        root_len = 1;
    }

    // Create intermediate directories
    for (u64 i = (root_len ? root_len : 1); i < wpath.len; ++i) {
        if (buf[i] == L'\\' || buf[i] == L'/') {
            u16 saved = buf[i];
            buf[i] = L'\0';

            if (i != root_len) {
                if (!CreateDirectoryW((LPCWSTR)buf, NULL)) {
                    DWORD e = GetLastError();
                    if (e != ERROR_ALREADY_EXISTS) {
                        buf[i] = saved;
                        string_destroy(&wpath);
                        switch (e) {
                        case ERROR_ACCESS_DENIED:        return DIR_CREATE_ERR_ACCESS_DENIED;
                        case ERROR_PATH_NOT_FOUND:       return DIR_CREATE_ERR_INVALID_PATH;
                        case ERROR_INVALID_NAME:         return DIR_CREATE_ERR_INVALID_PATH;
                        case ERROR_FILENAME_EXCED_RANGE: return DIR_CREATE_ERR_INVALID_PATH;
                        default:                         return DIR_CREATE_ERR_FAILED;
                        }
                    }
                }
            }
            buf[i] = saved;
        }
    }

    // Create the final directory
    if (!CreateDirectoryW((LPCWSTR)buf, NULL)) {
        DWORD e = GetLastError();
        string_destroy(&wpath);
        switch (e) {
        case ERROR_ALREADY_EXISTS:        return DIR_CREATE_ERR_EXISTS;
        case ERROR_ACCESS_DENIED:         return DIR_CREATE_ERR_ACCESS_DENIED;
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_NAME:
        case ERROR_FILENAME_EXCED_RANGE:  return DIR_CREATE_ERR_INVALID_PATH;
        default:                          return DIR_CREATE_ERR_FAILED;
        }
    }

    string_destroy(&wpath);
    return DIR_CREATE_OK;

#else
    char* buf = (char*)path.data;
    size_t len = path.len;

    // For absolute paths, start after the first '/'
    size_t start = (len > 0 && buf[0] == '/') ? 1 : 0;

    // Create intermediate directories
    for (size_t i = start + 1; i < len; ++i) {
        if (buf[i] == '/') {
            char saved = buf[i];
            buf[i] = '\0';

            if (buf[i - 1] != '\0') {
                if (mkdir(buf, 0755) != 0) {
                    int e = errno;
                    if (e != EEXIST) {
                        buf[i] = saved;
                        switch (e) {
                        case EACCES: return DIR_CREATE_ERR_ACCESS_DENIED;
                        case ENOENT: return DIR_CREATE_ERR_INVALID_PATH; // parent missing
                        case ENAMETOOLONG:
                        case EINVAL:
                        case ELOOP:  return DIR_CREATE_ERR_INVALID_PATH;
                        default:     return DIR_CREATE_ERR_FAILED;
                        }
                    }
                }
            }

            buf[i] = saved;
        }
    }

    // Create final directory
    if (mkdir(buf, 0755) != 0) {
        int e = errno;
        switch (e) {
        case EEXIST:        return DIR_CREATE_ERR_EXISTS;
        case EACCES:        return DIR_CREATE_ERR_ACCESS_DENIED;
        case ENOENT:        return DIR_CREATE_ERR_INVALID_PATH;
        case ENAMETOOLONG:
        case EINVAL:
        case ELOOP:         return DIR_CREATE_ERR_INVALID_PATH;
        default:            return DIR_CREATE_ERR_FAILED;
        }
    }

    return DIR_CREATE_OK;
#endif
}

FileWriter file_writer_get_stdout() {
    FileWriter w = { 0 };
#if defined(PLATFORM_WINDOWS)
    w.handle  = GetStdHandle(STD_OUTPUT_HANDLE);
    w.is_open = (w.handle != NULL && w.handle != INVALID_HANDLE_VALUE);
    w.owns    = false;
#else
    w.fd      = 1;
    w.is_open = true;
    w.owns     = false;
#endif
    return w;
}

FileWriter file_writer_get_stderr() {
    FileWriter w = { 0 };
#if defined(PLATFORM_WINDOWS)
    w.handle  = GetStdHandle(STD_ERROR_HANDLE);
    w.is_open = (w.handle != NULL && w.handle != INVALID_HANDLE_VALUE);
    w.owns    = false;
#else
    w.fd      = 2;
    w.is_open = true;
    w.owns     = false;
#endif
    return w;
}

bool is_file_writer_open(const FileWriter* w) {
    assert(w != NULL);
    return w->is_open;
}

FileWriteStatus file_writer_open(FileWriter* w, StringView path, bool overwrite_content) {
    assert(w != NULL);

    w->is_open = false;
    w->owns = false;

    if (is_string_view_empty(path)) {
        return FILE_WRITE_ERR_INVALID_PATH;
    }

#if defined(PLATFORM_WINDOWS)
    String wpath = STRING_EMPTY;
    if (!string_view_utf8_to_utf16_str(path, &wpath)) {
        return FILE_WRITE_ERR_INVALID_PATH;
    }

    if (is_string_empty(wpath)) {
        return FILE_WRITE_ERR_INVALID_PATH;
    }

    DWORD disp = overwrite_content ? CREATE_ALWAYS : CREATE_NEW;

    HANDLE handle = CreateFileW((LPCWSTR)wpath.data, GENERIC_WRITE, FILE_SHARE_READ, NULL, disp, FILE_ATTRIBUTE_NORMAL, NULL);
    string_destroy(&wpath);

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            return FILE_WRITE_ERR_NOT_FOUND;
        }
        if (err == ERROR_ACCESS_DENIED) {
            return FILE_WRITE_ERR_ACCESS_DENIED;
        }
        if (err == ERROR_ALREADY_EXISTS) {
            return FILE_WRITE_ERR_OPEN;
        }
        return FILE_WRITE_ERR_OPEN;
    }
    w->handle  = handle;
    w->is_open = true;
    w->owns    = true;
    return FILE_WRITE_OK;
#else
    int flags = O_WRONLY | O_CREAT;
    if (overwrite_content) {
        flags |= O_TRUNC;
    }
    else {
        flags |= O_EXCL;
    }

    int fd = open((const char*)path.data, flags, 0644);
    int e = (fd < 0) ? errno : 0;
    if (fd < 0) {
        if (e == ENOENT) {
            return FILE_WRITE_ERR_NOT_FOUND;
        }
        if (e == EACCES) {
            return FILE_WRITE_ERR_ACCESS_DENIED;
        }
        if (e == EEXIST) {
            return FILE_WRITE_ERR_OPEN;
        }
        return FILE_WRITE_ERR_OPEN;
    }
    w->fd      = fd;
    w->is_open = true;
    w->owns    = true;
    return FILE_WRITE_OK;
#endif
}

FileWriteStatus file_writer_flush(FileWriter* w) {
    assert(w != NULL);

    if (!is_file_writer_open(w)) {
        return FILE_WRITE_ERR_FLUSH;
    }
#if defined(PLATFORM_WINDOWS)
    /* FlushFileBuffers fails on console handles; treat as OK if it's a console */
    DWORD t = GetFileType(w->handle);
    if (t == FILE_TYPE_CHAR) {
        return FILE_WRITE_OK;
    }
    if (!FlushFileBuffers(w->handle)) {
        return FILE_WRITE_ERR_FLUSH;
    }
    return FILE_WRITE_OK;
#else
    if (fsync(w->fd) != 0) {
        return FILE_WRITE_ERR_FLUSH;
    }
    return FILE_WRITE_OK;
#endif
}

FileWriteStatus file_writer_close(FileWriter* w) {
    assert(w != NULL);

    if (!w->is_open) {
        return FILE_WRITE_OK;
    }
#if defined(PLATFORM_WINDOWS)
    BOOL ok = TRUE;
    if (w->owns) {
        ok = CloseHandle(w->handle);
    }
    w->handle = NULL;
    w->is_open = false;
    w->owns = false;
    return ok ? FILE_WRITE_OK : FILE_WRITE_ERR_CLOSE;
#else
    int rc = 0;
    if (w->owns) {
        rc = close(w->fd);
    }
    w->fd = -1;
    w->is_open = false;
    w->owns = false;
    return (rc == 0) ? FILE_WRITE_OK : FILE_WRITE_ERR_CLOSE;
#endif
}

/* Internal: chunked write for u64 lengths */
static FileWriteStatus writer_write_all(FileWriter* w, const u8* data, u64 len) {
    assert(w != NULL);

    if (!is_file_writer_open(w)) {
        return FILE_WRITE_ERR_WRITE;
    }
    if (data == NULL || len == 0) {
        return FILE_WRITE_OK;
    }

#if defined(PLATFORM_WINDOWS)
    while (len > 0) {
        DWORD chunk = (len > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)len;
        DWORD wrote = 0;
        if (!WriteFile(w->handle, data, chunk, &wrote, NULL)) {
            return FILE_WRITE_ERR_WRITE;
        }
        if (wrote == 0) {
            return FILE_WRITE_ERR_WRITE;
        }
        data += wrote;
        len  -= wrote;
    }
    return FILE_WRITE_OK;
#else
    while (len > 0) {
        u64 chunk = (len > (u64)U64_MAX) ? (u64)U64_MAX : (u64)len;
        i64 wrote = write(w->fd, data, chunk);
        if (wrote < 0) {
            return FILE_WRITE_ERR_WRITE;
        }
        if (wrote == 0) {
            return FILE_WRITE_ERR_WRITE;
        }
        data += (u64)wrote;
        len  -= (u64)wrote;
    }
    return FILE_WRITE_OK;
#endif
}

FileWriteStatus file_writer_write_cstr(FileWriter* w, const char* cstr) {
    assert(w != NULL);

    if (cstr == NULL || *cstr == '\0') {
        return FILE_WRITE_OK;
    }
    return writer_write_all(w, (const u8*)cstr, (u64)strlen(cstr));
}

FileWriteStatus file_writer_write_bytes(FileWriter* w, const u8* data, u64 len) {
    assert(w != NULL);

    return writer_write_all(w, data, len);
}

FileWriteStatus file_writer_write_sv(FileWriter* w, StringView sv) {
    assert(w != NULL);

    if (is_string_view_empty(sv)) {
        return FILE_WRITE_OK;
    }
    return writer_write_all(w, (const u8*)sv.data, sv.len);
}

FileWriteStatus file_writer_write_rune(FileWriter* w, Rune r) {
    assert(w != NULL);

    u8 buf[4] = { 0 };
    u64 n = encode_utf8_rune(r, buf, sizeof(buf));
    return writer_write_all(w, buf, n);
}

FileWriteStatus file_writer_write_eol(FileWriter* w) {
    assert(w != NULL);
#if defined(PLATFORM_WINDOWS)
    static const u8 crlf[2] = { '\r', '\n' };
    return writer_write_all(w, crlf, 2);
#else
    static const u8 lf[1] = { '\n' };
    return writer_write_all(w, lf, 1);
#endif
}

FileWriteStatus file_writer_write_format(FileWriter* w, const char* format, ...) {
    assert(w != NULL);

    va_list va;
    va_start(va, format);
    FileWriteStatus st = file_writer_write_format_va(w, format, va);
    va_end(va);
    return st;
}

FileWriteStatus file_writer_write_format_va(FileWriter* w, const char* format, va_list va) {
    assert(w != NULL);

    if (format == NULL || *format == '\0') {
        return FILE_WRITE_OK;
    }

    /* First try a fixed stack buffer; if not enough, allocate. */
    char stackbuf[4096] = { 0 };
    va_list va2;
    va_copy(va2, va);
    int need = vsnprintf(stackbuf, sizeof(stackbuf), format, va2);
    va_end(va2);

    assert(need > 0);

    if ((size_t)need < sizeof(stackbuf)) {
        return writer_write_all(w, (const u8*)stackbuf, (u64)need);
    }

    char* heap = malloc((size_t)need + 1u);
    assert(heap != NULL);

    int need2 = vsnprintf(heap, (size_t)need + 1u, format, va);
    if (need2 < 0) {
        free(heap);
        return FILE_WRITE_ERR_WRITE;
    }

    FileWriteStatus st = writer_write_all(w, (const u8*)heap, (u64)need2);
    free(heap);
    return st;
}