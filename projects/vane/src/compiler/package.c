#include "vane/compiler/package.h"

#include <stdlib.h>

Package* package_create(StringView path) {
    Package* package = malloc(sizeof(Package));
    assert(package != NULL);

    package->path = path;
    package->parent_package = NULL;

    package->source_files = vector_create(PACKAGE_DEFAULT_SOURCE_FILE_COUNT,
        VECTOR_SPECS(const SourceFile*, NULL)
    );

    package->subpackages = vector_create(PACKAGE_DEFAULT_SUBPACKAGE_COUNT,
        VECTOR_SPECS(const Package*, NULL)
    );

    return package;
}

void package_destroy(Package* package) {
    if (package == NULL) {
        return;
    }

    vector_destroy(&package->source_files);
    vector_destroy(&package->subpackages);

    free(package);
}