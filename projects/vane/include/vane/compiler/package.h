#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/hashmap.h"
#include "vane/utils/vector.h"

#include "vane/compiler/source_file.h"

typedef struct Package Package;

#define PACKAGE_DEFAULT_SOURCE_FILE_COUNT 8
#define PACKAGE_DEFAULT_SUBPACKAGE_COUNT 8

struct Package {
    StringView path;

    // key:   [String, &string_destroy]
    // value: [SourceFile*, &source_file_destroy]
    Hashmap source_files;

    // [Package*, NULL]
    Vector subpackages;

    const Package* parent_package;
};

Package* package_create(StringView path);

void package_destroy(Package* package);