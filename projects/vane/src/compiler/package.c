#include "vane/compiler/package.h"

#include <stdlib.h>

Package* package_create(StringView path) {
    Package* package = malloc(sizeof(Package));
    assert(package != NULL);

    package->path = path;

    package->scope = NULL;
    package->parent_package = NULL;

    package->source_files = vector_create(PACKAGE_DEFAULT_SOURCE_FILE_COUNT,
        VECTOR_SPECS(const SourceFile*, NULL)
    );

    package->subpackages = vector_create(PACKAGE_DEFAULT_SUBPACKAGE_COUNT,
        VECTOR_SPECS(const Package*, NULL)
    );

    package->is_core = false;

    return package;
}

void package_destroy(Package* package) {
    if (package == NULL) {
        return;
    }

    vector_destroy(&package->source_files);
    vector_destroy(&package->subpackages);

    //scope_destroy(package->scope);B

    free(package);
}

bool package_resolve_symbol_decls(Package* package, Scope* global_scope) {
    assert(package != NULL);

    if (package->scope != NULL) {
        return true;
    }

    package->scope = scope_create(SCOPE_PACKAGE, global_scope, NULL);

    bool is_good = true;

    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* source_file = vector_at(package->source_files, i);

        if (!source_file_resolve_symbol_decls(source_file)) {
            is_good = false;
        }
    }

    return is_good;
}

bool package_bind_symbols(Package* package) {
    assert(package != NULL);

    bool is_good = true;

    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* source_file = vector_at(package->source_files, i);

        if (!source_file_bind_symbols(source_file)) {
            is_good = false;
        }
    }

    return is_good;
}