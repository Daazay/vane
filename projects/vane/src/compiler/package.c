#include "vane/compiler/package.h"

#include <stdlib.h>

#include "vane/compiler/compiler.h"

Package* package_create(StringView path, struct Compiler* compiler) {
    Package* package = malloc(sizeof(Package));
    assert(package != NULL);

    package->path = path;

    package->scope = NULL;
    package->parent_package = NULL;

    package->source_files = (Vector){ 0 };
    package->subpackages = (Vector) { 0 };

    package->compiler = compiler;
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

void package_add_source_file(Package* package, SourceFile* source_file) {
    assert(package != NULL && source_file != NULL);
    assert(source_file->package == NULL && source_file->package != package && "source file already belongs to another package");

    if (package->source_files.raw == 0) {
        package->source_files = vector_create(
            PACKAGE_DEFAULT_SOURCE_FILE_COUNT,
            VECTOR_SPECS(const SourceFile*, NULL)
        );
    }

    source_file->package = package;
    vector_push_back(&package->source_files, &source_file);
}

void package_add_subpackage(Package* package, Package* subpackage) {
    assert(package != NULL && subpackage != NULL);
    assert(subpackage->parent_package == NULL && subpackage->parent_package != package && "subpackage already attached elsewhere");

    if (package->subpackages.raw == 0) {
        package->subpackages = vector_create(
            PACKAGE_DEFAULT_SUBPACKAGE_COUNT,
            VECTOR_SPECS(const Package*, NULL)
        );
    }

    subpackage->parent_package = package;
    vector_push_back(&package->subpackages, &subpackage);
}

bool package_resolve_symbol_decls(Package* package, Scope* prelude_scope) {
    assert(package != NULL);
    assert(prelude_scope != NULL || (package->parent_package && package->parent_package->scope != NULL));

    if (package->scope != NULL) {
        return true;
    }

    // Chain to parent scope if available, otherwise to prelude/global.
    Scope* parent_chain = (package->parent_package && package->parent_package->scope)
        ? package->parent_package->scope
        : prelude_scope;

    assert(parent_chain != NULL && "A package scope must chain to a valid parent/prelude.");

    package->scope = scope_create(SCOPE_PACKAGE, parent_chain, NULL);

    REPORT_DEBUG(&package->compiler->rc, DIAG_SEMA_SYMBOLS, "created package scope for '" SV_FMT "'.", SV_ARG(package->path));

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
    assert(package->scope != NULL && "bind requires package scope (resolve decls first)");

    bool is_good = true;
    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* source_file = vector_at(package->source_files, i);

        assert(source_file->scope != NULL && "bind requires file scope; call resolve decls first");

        if (!source_file_bind_symbols(source_file)) {
            is_good = false;
        }
    }

    return is_good;
}

bool package_build_cfg(Package* package) {
    assert(package != NULL);

    bool is_good = true;

    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* source_file = vector_at(package->source_files, i);

        if (!source_file_build_cfgs(source_file)) {
            is_good = false;
        }
    }

    return is_good;
}