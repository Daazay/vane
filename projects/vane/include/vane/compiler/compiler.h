#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/vector.h"
#include "vane/utils/hashmap.h"

#include "vane/sema/type_system.h"

#include "vane/compiler/build_options.h"
#include "vane/compiler/package.h"

#include "vane/diagnostic/report_collector.h"

typedef struct Compiler Compiler;

#define VANE_LANG_EXT                           ".vn"
#define COMPILER_DEFAULT_PACKAGE_COUNT          8
#define COMPILER_DEFAULT_SOURCE_FILE_COUNT      8
#define COMPILER_DEFAULT_COLLECTION_PATHS_COUNT 2
#define COMPILER_DEFAULT_SOURCE_FILE_QUEUE_SIZE 8

struct Compiler {
    BuildOptions* build_options;

    // key:   [String, &string_destroy]
    // value: [Package*, &package_destroy]
    Hashmap packages;

    // key:   [String, &string_destroy]
    // value: [SourceFile*, &source_file_destroy]
    Hashmap source_files;

    Vector source_files_queue;

    Package* entry_point;

    ReportCollector rc;

    // visible to all packages
    // here lays all builtins: types, functions, and etc
    struct Scope* global_scope;
    // Scopes for core packages
    struct Scope* prelude_scope;

    TypeSystem ts;
};

Compiler compiler_create(BuildOptions* build_options);

void compiler_destroy(Compiler* compiler);

bool compiler_run_command(Compiler* compiler);

StringView compiler_get_collection_path(Compiler* compiler, StringView collection_name);

Package* compiler_load_core_collection(Compiler* compiler);

Package* compiler_load_package(Compiler* compiler, StringView dirpath, bool is_core);

Package* compiler_resolve_import(Compiler* compiler, SourceFile* source_file, const ImportEntry* e);

// dump functions

void compiler_dump_ast_text(const Compiler* compiler);

void compiler_dump_ast_dot(const Compiler* compiler);

void compiler_dump_symbols(const Compiler* compiler);

void compiler_dump_types(const Compiler* compiler);

//

bool compiler_parse_source_files(Compiler* compiler);

bool compiler_resolve_imports(Compiler* compiler);

bool compiler_resolve_symbol_decls(Compiler* compiler);

bool compiler_bind_symbols(Compiler* compiler);

bool compiler_resolve_types(Compiler* compiler);

bool compiler_build_cfgs(Compiler* compiler);