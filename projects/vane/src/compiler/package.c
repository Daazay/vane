#include "vane/compiler/package.h"

#include <stdlib.h>

Package* package_create(StringView path) {
    Package* package = malloc(sizeof(Package));
    assert(package != NULL);

    package->path = path;
    package->parent_package = NULL;

    package->source_files = hashmap_create(PACKAGE_DEFAULT_SOURCE_FILE_COUNT,
        HASHMAP_KEY_SPECS(String, &__string_get_hash, &__string_eq_str, &string_destroy),
        HASHMAP_VALUE_SPECS(SourceFile*, &source_file_destroy)
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

    hashmap_destroy(&package->source_files);
    vector_destroy(&package->subpackages);

    free(package);
}