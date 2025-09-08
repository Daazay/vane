#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/hashmap.h"

#include "vane/compiler/build_options.h"
#include "vane/compiler/package.h"

#include "vane/diagnostic/report_collector.h"

typedef struct Compiler Compiler;

#define VANE_LANG_EXT                           ".vn"
#define COMPILER_DEFAULT_PACKAGE_COUNT          8
#define COMPILER_DEFAULT_SOURCE_FILE_COUNT      8
#define COMPILER_DEFAULT_COLLECTION_PATHS_COUNT 2

struct Compiler {
    BuildOptions* build_options;

    // key:   [String, &string_destroy]
    // value: [Package*, &package_destroy]
    Hashmap packages;

    // key:   [String, &string_destroy]
    // value: [SourceFile*, &source_file_destroy]
    Hashmap source_files;

    Package* entry_point;

    ReportCollector rc;
};

Compiler compiler_create(BuildOptions* build_options);

void compiler_destroy(Compiler* compiler);

Package* compiler_discover_packages(Compiler* compiler, StringView path);

bool compiler_parse_source_files(Compiler* compiler);

void compiler_dump_ast(const Compiler* compiler);

void compiler_dump_ast_dot(const Compiler* compiler);