#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/hashmap.h"

#include "vane/compiler/build_options.h"
#include "vane/compiler/package.h"

typedef struct Compiler Compiler;

#define VANE_LANG_EXT                           ".vn"
#define COMPILER_DEFAULT_PACKAGES_COUNT         8
#define COMPILER_DEFAULT_COLLECTION_PATHS_COUNT 2

struct Compiler {
    BuildOptions* build_options;

    // key:   [String, &string_destroy]
    // value: [Package*, &package_destroy]
    Hashmap packages;

    Package* entry_point;
};

Compiler compiler_create(BuildOptions* build_options);

void compiler_destroy(Compiler* compiler);

Package* compiler_load_packages(Compiler* compiler, StringView path);