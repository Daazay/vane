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

StringView compiler_get_collection_path(Compiler* compiler, StringView collection_name);

Package* compiler_load_package(Compiler* compiler, StringView dirpath);

Package* compiler_try_resolve_imported_package(Compiler* compiler, StringView collection_name, StringView package_path);

void compiler_dump_ast(const Compiler* compiler);

void compiler_dump_ast_dot(const Compiler* compiler);

bool compiler_parse_source_files(Compiler* compiler);

bool compiler_resolve_imports(Compiler* compiler);