#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/string_view.h"
#include "vane/utils/vector.h"

typedef enum FileLoadStatus FileLoadStatus;
typedef enum FileWriteStatus FileWriteStatus;
typedef enum DirListStatus DirListStatus;
typedef enum DirCreateStatus DirCreateStatus;

typedef struct DirEntry DirEntry;
typedef struct FileWriter FileWriter;

enum FileLoadStatus {
    FILE_LOAD_OK = 0,
    FILE_LOAD_ERR_EMPTY_CONTENT,
    FILE_LOAD_ERR_INVALID_PATH,
    FILE_LOAD_ERR_NOT_FOUND,
    FILE_LOAD_ERR_ACCESS_DENIED,
    FILE_LOAD_ERR_IS_DIR,
    FILE_LOAD_ERR_OPEN,
    FILE_LOAD_ERR_READ,
};

enum FileWriteStatus {
    FILE_WRITE_OK = 0,
    FILE_WRITE_ERR_INVALID_PATH,
    FILE_WRITE_ERR_NOT_FOUND,
    FILE_WRITE_ERR_ACCESS_DENIED,
    FILE_WRITE_ERR_IS_DIR,
    FILE_WRITE_ERR_OPEN,
    FILE_WRITE_ERR_WRITE,
    FILE_WRITE_ERR_FLUSH,
    FILE_WRITE_ERR_CLOSE,
};

enum DirListStatus {
    DIR_LIST_OK = 0,
    DIR_LIST_ERR_INVALID_PATH,
    DIR_LIST_ERR_NOT_FOUND,
    DIR_LIST_ERR_ACCESS_DENIED,
    DIR_LIST_ERR_NOT_DIR,
    DIR_LIST_ERR_OPEN,
    DIR_LIST_ERR_READ,
    DIR_LIST_ERR_STAT,
};

enum DirCreateStatus {
    DIR_CREATE_OK = 0,
    DIR_CREATE_ERR_INVALID_PATH,
    DIR_CREATE_ERR_EXISTS,
    DIR_CREATE_ERR_ACCESS_DENIED,
    DIR_CREATE_ERR_FAILED,
};

struct DirEntry {
    String fullpath;
    u64 size;
    u64 modified_time;
    bool is_dir;
};

struct FileWriter {
#if defined(PLATFORM_WINDOWS)
    void* handle;
#else
    int fd;
#endif
    bool is_open;
    bool owns;
};

FileLoadStatus file_content_load(StringView path, u8** data, u64* size);

DirListStatus directory_list(StringView path, Vector* entries, bool override_content);

DirCreateStatus directory_create(StringView path);

DirCreateStatus directory_create_recursive(StringView path);

FileWriter file_writer_get_stdout();

FileWriter file_writer_get_stderr();

bool is_file_writer_open(const FileWriter* w);

FileWriteStatus file_writer_open(FileWriter* w, StringView path, bool override_content);

FileWriteStatus file_writer_flush(FileWriter* w);

FileWriteStatus file_writer_close(FileWriter* w);

FileWriteStatus file_writer_write_cstr(FileWriter* w, const char* cstr);

FileWriteStatus file_writer_write_bytes(FileWriter* w, const u8* data, u64 len);

FileWriteStatus file_writer_write_sv(FileWriter* w, StringView sv);

FileWriteStatus file_writer_write_rune(FileWriter* w, Rune r);

FileWriteStatus file_writer_write_eol(FileWriter* w);

FileWriteStatus file_writer_write_format(FileWriter* w, const char* format, ...);

FileWriteStatus file_writer_write_format_va(FileWriter* w, const char* format, va_list va);