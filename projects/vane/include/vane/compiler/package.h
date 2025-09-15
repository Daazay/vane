#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/hashmap.h"
#include "vane/utils/vector.h"

#include "vane/compiler/source_file.h"

#include "vane/sema/scope.h"

struct Compiler;
typedef struct Package Package;

#define PACKAGE_DEFAULT_SOURCE_FILE_COUNT 8
#define PACKAGE_DEFAULT_SUBPACKAGE_COUNT  8

struct Package {
    StringView path;

    // [SourceFile*, NULL]
    Vector source_files;

    // [Package*, NULL]
    Vector subpackages;

    Scope* scope;
    Package* parent_package;
    struct Compiler* compiler;

    Symbol* entry_point;

    bool is_core;
};

Package* package_create(StringView path, struct Compiler* compiler);

void package_destroy(Package* package);

void package_add_source_file(Package* package, SourceFile* source_file);

void package_add_subpackage(Package* package, Package* subpackage);

bool package_resolve_symbol_decls(Package* package, Scope* global_scope);

bool package_bind_symbols(Package* package);

bool package_validate_semantics(Package* package);

bool package_build_cfg(Package* package);