#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/string_view.h"
#include "vane/utils/vector.h"

typedef enum FileLoadStatus FileLoadStatus;
typedef enum DirListStatus DirListStatus;

typedef struct DirEntry DirEntry;

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

enum DirListStatus {
    DIR_LIST_OK,
    DIR_LIST_ERR_INVALID_PATH,
    DIR_LIST_ERR_NOT_FOUND,
    DIR_LIST_ERR_ACCESS_DENIED,
    DIR_LIST_ERR_NOT_DIR,
    DIR_LIST_ERR_OPEN,
    DIR_LIST_ERR_READ,
    DIR_LIST_ERR_STAT,
};

struct DirEntry {
    String fullpath;
    u64 size;
    u64 modified_time;
    bool is_dir;
};

void dir_entry_destroy(DirEntry* entry);

FileLoadStatus file_content_load(StringView path, u8** data, u64* size);

DirListStatus directory_list(StringView path, Vector* entries, bool recursive);