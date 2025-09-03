#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"

#include "vane/utils/file_utils.h"

typedef struct SourceFile SourceFile;

struct SourceFile {
    String content;
    StringView path;

    struct Package* package;
};

SourceFile* source_file_create(StringView path);

void source_file_destroy(SourceFile* source_file);

FileLoadStatus source_file_load_content(SourceFile* source_file);