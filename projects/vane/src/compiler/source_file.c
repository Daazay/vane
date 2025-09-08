#include "vane/compiler/source_file.h"

#include <stdlib.h>

SourceFile* source_file_create(StringView path) {
    SourceFile* source_file = malloc(sizeof(SourceFile));
    assert(source_file != NULL);

    source_file->path = path;
    source_file->package = NULL;
    source_file->content = STRING_EMPTY;

    return source_file;
}

void source_file_destroy(SourceFile* source_file) {
    if (source_file == NULL) {
        return;
    }

    string_destroy(&source_file->content);

    free(source_file);
}

FileLoadStatus source_file_load_content(SourceFile* source_file) {
    assert(source_file != NULL);

    if (!is_string_empty(source_file->content)) {
        return FILE_LOAD_OK;
    }

    FileLoadStatus status = file_content_load(source_file->path, &source_file->content.data, &source_file->content.len);
    return status;
}