#include <stdio.h>
#include <stdlib.h>

#include <vane/compiler/build_options.h>
#include <vane/compiler/compiler.h>

#include <vane/utils/hashmap.h>
#include <vane/utils/path.h>
#include <vane/utils/file_utils.h>

#include <vane/scanner/token_stream.h>

// Print indentation for tree structure
static inline void print_indent(u32 indent) {
    for (u32 i = 0; i < indent; i++) {
        printf("  ");
    }
}

// Print a package and its contents recursively
static void print_package_tree(const Package* package, u32 indent) {
    StringView basename = path_get_basename(package->path);
    print_indent(indent);
    printf("📦 Package: %.*s\n", (int)basename.len, basename.data);

    // Print source files
    if (package->source_files.size > 0) {
        print_indent(indent);
        printf("  📄 Source Files (%u):\n", package->source_files.size);

        HashmapIterator it = hashmap_get_it(&package->source_files);
        String file_path;
        const SourceFile* source_file;

        while (hashmap_it_next(&it, &file_path, &source_file)) {
            StringView filename = path_get_basename(string_get_view(file_path));
            print_indent(indent + 2);
            printf("• %.*s\n", (int)filename.len, filename.data);
        }
    }

    // Print subpackages
    if (package->subpackages.size > 0) {
        print_indent(indent);
        printf("  📦 Subpackages (%u):\n", package->subpackages.size);

        for (u32 i = 0; i < package->subpackages.size; i++) {
            Package* subpackage = vector_at(package->subpackages, i);
            print_package_tree(subpackage, indent + 2);
        }
    }
}

// Print build options information
static void print_build_options(const BuildOptions* options) {
    printf("Build Configuration:\n");
    printf("====================\n");

    printf("Command: ");
    switch (options->command) {
        case BUILD_COMMAND_HELP: printf("help\n"); break;
        case BUILD_COMMAND_BUILD: printf("build\n"); break;
        case BUILD_COMMAND_MISSING: printf("(none)\n"); break;
    }

    printf("Root Path: %.*s\n", (int)options->root_path.len, options->root_path.data);
    printf("Output Directory: %.*s\n", (int)options->output_dir.len, options->output_dir.data);
    printf("Jobs: %u\n", options->jobs);

    // Print collections
    if (options->collections.size > 0) {
        printf("Collections (%u):\n", options->collections.size);
        HashmapIterator it = hashmap_get_it(&options->collections);
        String name;
        String path;

        while (hashmap_it_next(&it, &name, &path)) {
            printf("  %.*s -> %.*s\n",
                   (int)name.len, name.data,
                   (int)path.len, path.data);
        }
    }

    // Print defines
    if (options->defines.size > 0) {
        printf("Defines (%u):\n", options->defines.size);
        HashmapIterator it = hashmap_get_it(&options->defines);
        String key;
        String value;

        while (hashmap_it_next(&it, &key, &value)) {
            printf("  %.*s = %.*s\n",
                   (int)key.len, key.data,
                   (int)value.len, value.data);
        }
    }
    printf("\n");
}

// Print compiler information
static void print_compiler_info(const Compiler* compiler) {
    printf("Compiler Information:\n");
    printf("=====================\n");
    printf("Packages Loaded: %u\n", compiler->packages.size);

    // Find and print the root package (the one without a parent)
    HashmapIterator it = hashmap_get_it(&compiler->packages);
    String package_path;
    const Package* package;

    while (hashmap_it_next(&it, &package_path, &package)) {
        if (package == NULL) {
            continue;
        }
        if (package->parent_package == NULL) {
            printf("Root Package: %.*s\n\n", (int)package_path.len, package_path.data);
            print_package_tree(package, 0);
            break;
        }
    }
}

int main(int argc, const char **argv) {
    BuildOptions build_options = { 0 };
    build_options_init(&build_options);

    if (!build_options_parse_args(&build_options, argc, argv)) {
        build_options_destroy(&build_options);
        return 1;
    }

    if (build_options.command == BUILD_COMMAND_HELP ||
        build_options.command == BUILD_COMMAND_MISSING) {
        print_usage(argv[0]);
        build_options_destroy(&build_options);
        return 0;
    }

    Compiler compiler = compiler_create(&build_options);

    // Load root package
    Package* root_package = compiler_load_packages(&compiler, string_get_view(build_options.root_path));
    if (root_package == NULL) {
        printf("failed to project from path: \"%.*s\"\n", (i32)build_options.root_path.len, build_options.root_path.data);
        compiler_destroy(&compiler);
        build_options_destroy(&build_options);
        return 1;
    }

    HashmapIterator it = hashmap_get_it(&compiler.packages);
    const Package* package = NULL;

    while (hashmap_it_next(&it, NULL, &package)) {
        if (package == NULL) {
            continue;
        }

        HashmapIterator it2 = hashmap_get_it(&package->source_files);
        const SourceFile* source_file = NULL;

        while (hashmap_it_next(&it2, NULL, &source_file)) {
            if (source_file == NULL) {
                continue;
            }

            StringView content = STRING_VIEW_EMPTY;
            FileLoadStatus status = file_content_load(source_file->path, (u8**)&content.data, &content.len);

            if (status != FILE_LOAD_OK) {
                continue;
            }

            printf("file: %.*s\n", (i32)source_file->path.len, (const char*)source_file->path.data);

            TokenStream ts = token_stream_create(0, source_file->path, content);

            while (!ts.done) {
                const Token* token = token_stream_advance(&ts);

                printf("  [%10s][", token_kind_get_name(token->kind));

                if (IS_FLAG_SET(token->flags, TOKEN_FLAG_FIRST_IN_LINE)) {
                    printf(" FIRST");
                } else { printf("      "); }
                if (IS_FLAG_SET(token->flags, TOKEN_FLAG_HAS_LEADING_WS)) {
                    printf(" | LEAD_WS");
                } else { printf(" |        "); }
                if (IS_FLAG_SET(token->flags, TOKEN_FLAG_HAS_LEADING_LBR)) {
                    printf(" | LEAD_LBR");
                } else { printf(" |         "); }
                if (IS_FLAG_SET(token->flags, TOKEN_FLAG_HAS_TRAILING_WS)) {
                    printf(" | TRAIL_WS");
                } else { printf(" |         "); }
                if (IS_FLAG_SET(token->flags, TOKEN_FLAG_HAS_TRAILING_LBR)) {
                    printf(" | TRAIL_LBR ");
                } else { printf(" |           "); }
                //if (IS_FLAG_SET(token.flags, TOKEN_FLAG_HAS_AROUND_LBR)) {
                //    if (f1) { printf(" | "); }
                //
                //    printf("AROUND_LBR");
                //}
                //if (IS_FLAG_SET(token.flags, TOKEN_FLAG_HAS_LEADING_WS_OR_LBR)) {
                //    if (f1) { printf(" | "); }
                //
                //    printf("LEAD_ALL  ");
                //}
                //if (IS_FLAG_SET(token.flags, TOKEN_FLAG_HAS_TRAILING_WS_OR_LBR)) {
                //    if (f1) { printf(" | "); }
                //
                //    printf("TRAIL_ALL ");
                //}
                //if (IS_FLAG_SET(token.flags, TOKEN_FLAG_HAS_AROUND_WS_OR_LBR)) {
                //    if (f1) { printf(" | "); }
                //
                //    printf("AROUND_ALL");
                //}

                printf("] - %.*s\n", (i32)token->value.len, (const char*)token->value.data);
            }
        }
    }

    print_build_options(&build_options);
    print_compiler_info(&compiler);

    compiler_destroy(&compiler);
    build_options_destroy(&build_options);

    return 0;
}
