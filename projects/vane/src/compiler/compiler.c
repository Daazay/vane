#include "vane/compiler/compiler.h"

#include "vane/utils/path.h"

Compiler compiler_create(BuildOptions* build_options) {
    assert(build_options != NULL);

    Compiler compiler = { 0 };
    compiler.build_options = build_options;

    compiler.packages = hashmap_create(COMPILER_DEFAULT_PACKAGE_COUNT,
        HASHMAP_KEY_SPECS(String, &string_view_item_hash, &string_view_item_eq, &string_destroy),
        HASHMAP_VALUE_SPECS(Package*, &package_destroy)
    );

    compiler.source_files = hashmap_create(COMPILER_DEFAULT_SOURCE_FILE_COUNT,
        HASHMAP_KEY_SPECS(String, &string_view_item_hash, &string_view_item_eq, &string_destroy),
        HASHMAP_VALUE_SPECS(SourceFile*, &source_file_destroy)
    );

    compiler.entry_point = NULL;

    return compiler;
}

void compiler_destroy(Compiler* compiler) {
    if (compiler == NULL) {
        return;
    }

    hashmap_destroy(&compiler->packages);
    hashmap_destroy(&compiler->source_files);
}

Package* compiler_discover_packages(Compiler* compiler, StringView path) {
    assert(compiler != NULL);

    // Resolve absolute path
    String abs_path = path_get_absolute(path);
    StringView abs_path_sv = string_get_view(abs_path);

    // Check if this path has already been processed
    Package* existing_package = hashmap_get(&compiler->packages, &abs_path_sv);
    if (existing_package != NULL) {
        string_destroy(&abs_path);
        return existing_package;
    }

    // Insert a placeholder to prevent duplicate scanning
    hashmap_insert(&compiler->packages, &abs_path, NULL);

    Vector entries = { 0 };
    DirListStatus status = directory_list(abs_path_sv, &entries, false);
    if (status != DIR_LIST_OK) {
        return NULL;
    }

    Package* package = NULL;

    for (u32 i = 0; i < entries.size; ++i) {
        DirEntry* entry = vector_at(entries, i);
        StringView basename = path_get_basename(string_get_view(entry->fullpath));

        // Skip hidden directories
        if (entry->is_dir && string_view_has_prefix_sv(basename, STR_LIT("."))) {
            continue;
        }

        // Skip files without the expected language extension
        if (!entry->is_dir && !string_view_has_suffix_sv(basename, STR_LIT(VANE_LANG_EXT))) {
            continue;
        }

        // Process subdirectories recursively
        if (entry->is_dir) {
            Package* subpackage = compiler_discover_packages(compiler, string_get_view(entry->fullpath));
            if (subpackage == NULL) {
                continue;
            }

            // Lazily create package for this directory if needed
            if (package == NULL) {
                package = package_create(abs_path_sv);
            }

            vector_push_back(&package->subpackages, &subpackage);
            subpackage->parent_package = package;
        }
        // Process source files
        else {
            if (package == NULL) {
                package = package_create(abs_path_sv);
            }

            hashmap_insert(&compiler->source_files, &entry->fullpath, NULL);

            // Clear entry path since ownership transferred
            entry->fullpath = STRING_EMPTY;
        }
    }

    vector_destroy(&entries);

    // If package was created, update hashmap with actual pointer
    if (package != NULL) {
        hashmap_insert(&compiler->packages, &abs_path, &package);
    }

    return package;
}