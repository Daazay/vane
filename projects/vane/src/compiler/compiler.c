#include "vane/compiler/compiler.h"

#include "vane/utils/path.h"
#include "vane/utils/terminal.h"

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

    compiler.rc = report_collector_create(build_options->log_verbosity, is_terminal_support_colors());

    return compiler;
}

void compiler_destroy(Compiler* compiler) {
    if (compiler == NULL) {
        return;
    }

    hashmap_destroy(&compiler->packages);
    hashmap_destroy(&compiler->source_files);

    report_collector_destroy(&compiler->rc);
}

Package* compiler_discover_packages(Compiler* compiler, StringView path) {
    assert(compiler != NULL);

    // Resolve absolute path
    String abs_path = path_get_absolute(path);
    StringView abs_path_sv = string_get_view(abs_path);

    StringView package_name = path_get_basename(abs_path_sv);

    REPORT_DEBUG(&compiler->rc, "driver", "discovering packages in '"SV_FMT"'.", SV_ARG(abs_path_sv));

    // Check if this path has already been processed
    Package* existing_package = hashmap_get(&compiler->packages, &abs_path_sv);
    if (existing_package != NULL) {
        REPORT_INFO(&compiler->rc, "driver", "package '" SV_FMT "' is already loaded", SV_ARG(package_name));
        string_destroy(&abs_path);
        return existing_package;
    }

    // Insert a placeholder to prevent duplicate scanning
    hashmap_insert(&compiler->packages, &abs_path, NULL);

    Vector entries = { 0 };
    DirListStatus status = directory_list(abs_path_sv, &entries, false);
    switch (status) {
    case DIR_LIST_OK: break;
    case DIR_LIST_ERR_INVALID_PATH:
        REPORT_ERROR(&compiler->rc, "driver", "invalid path: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_NOT_FOUND:
        REPORT_ERROR(&compiler->rc, "driver", "path not found: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_ACCESS_DENIED:
        REPORT_ERROR(&compiler->rc, "driver", "access denied for path: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_NOT_DIR:
        REPORT_ERROR(&compiler->rc, "driver", "path is not a directory: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_OPEN:
        REPORT_ERROR(&compiler->rc, "driver", "failed to open directory: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_READ:
        REPORT_ERROR(&compiler->rc, "driver", "failed to read directory: '" SV_FMT"'.", SV_ARG(abs_path_sv));
        return NULL;
    case DIR_LIST_ERR_STAT:
        REPORT_ERROR(&compiler->rc, "driver", "failed to stat directory: '" SV_FMT"'.", SV_ARG(abs_path_sv));
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
                REPORT_NOTE(&compiler->rc, "driver", "skipping '"SV_FMT"' (no package created)", SV_ARG(basename));
                continue;
            }

            // Lazily create package for this directory if needed
            if (package == NULL) {
                package = package_create(abs_path_sv);

                hashmap_insert(&compiler->packages, &abs_path, &package);
                REPORT_INFO(&compiler->rc, "driver", "registered package '" SV_FMT"'.", SV_ARG(package_name));
            }

            vector_push_back(&package->subpackages, &subpackage);
            subpackage->parent_package = package;

            REPORT_INFO(&compiler->rc, "driver", "subpackage '" SV_FMT "' added to package '" SV_FMT "'", SV_ARG(basename), SV_ARG(package_name));
        }
        // Process source files
        else {
            if (package == NULL) {
                package = package_create(abs_path_sv);

                hashmap_insert(&compiler->packages, &abs_path, &package);
                REPORT_INFO(&compiler->rc, "driver", "registered package '" SV_FMT"'.", SV_ARG(package_name));
            }

            hashmap_insert(&compiler->source_files, &entry->fullpath, NULL);
            REPORT_INFO(&compiler->rc, "driver", "added source file '" SV_FMT"'.", SV_ARG(basename));

            // Clear entry path since ownership transferred
            entry->fullpath = STRING_EMPTY;
        }
    }

    vector_destroy(&entries);

    return package;
}