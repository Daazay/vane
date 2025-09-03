#include "vane/compiler/compiler.h"

#include "vane/utils/path.h"

Compiler compiler_create(BuildOptions* build_options) {
    assert(build_options != NULL);

    Compiler compiler = {0};
    compiler.build_options = build_options;

    compiler.packages = hashmap_create(COMPILER_DEFAULT_PACKAGES_COUNT,
        HASHMAP_KEY_SPECS(String, &__string_get_hash,&__string_eq_str, &string_destroy),
        HASHMAP_VALUE_SPECS(Package*, &package_destroy)
    );

    compiler.entry_point = NULL;

    return compiler;
}

void compiler_destroy(Compiler* compiler) {
    if (compiler == NULL) {
        return;
    }

    hashmap_destroy(&compiler->packages);
}

Package* compiler_load_packages(Compiler* compiler, StringView path) {
    assert(compiler != NULL);

    String abs_path = path_get_absolute(path);

    // Check if package is already loaded
    if (hashmap_contains(&compiler->packages, &abs_path)) {
        string_destroy(&abs_path);
        return hashmap_get(&compiler->packages, &abs_path);
    }

    // Reserve spot (mark as visited)
    hashmap_insert(&compiler->packages, &abs_path, NULL);
    StringView abs_path_sv = string_get_view(abs_path);

    Vector entries = { 0 };
    DirListStatus status = directory_list(abs_path_sv, &entries, false);
    if (status != DIR_LIST_OK) {
        return NULL;
    }

    Package* package = NULL;
    for (u32 i = 0; i < entries.size; ++i) {
        DirEntry* entry = vector_at(entries, i);
        StringView basename = path_get_basename(string_get_view(entry->fullpath));

        // Skip hudden folders
        if (entry->is_dir && string_view_has_prefix_sv(basename, STR_LIT("."))) {
            continue;
        }

        // Skip files with invalid ext
        if (!entry->is_dir && !string_view_has_suffix_sv(basename, STR_LIT(VANE_LANG_EXT))) {
            continue;
        }

        // Process subpackages
        if (entry->is_dir) {
            Package* sub = compiler_load_packages(compiler, string_get_view(entry->fullpath));
            if (sub != NULL) {
                if (package == NULL) {
                    package = package_create(abs_path_sv);
                }
                vector_push_back(&package->subpackages, &sub);
                sub->parent_package = package;
            }
        }
        // Process source files
        else {
            if (package == NULL) {
                package = package_create(abs_path_sv);
            }
            SourceFile* source_file = source_file_create(string_get_view(entry->fullpath));
            hashmap_insert(&package->source_files, &entry->fullpath, &source_file);
            source_file->package = package;
            entry->fullpath = STRING_EMPTY;
        }
    }

    vector_destroy(&entries);

    if (package != NULL) {
        hashmap_insert(&compiler->packages, &abs_path, &package);
    }

    return package;
}