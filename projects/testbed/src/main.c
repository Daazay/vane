#include <stdio.h>
#include <stdlib.h>

#include <vane/compiler/build_options.h>
#include <vane/compiler/compiler.h>
#include <vane/diagnostic/report_collector.h>

int main(int argc, const char **argv) {
    BuildOptions build_options = { 0 };
    build_options_init(&build_options);

    if (!build_options_parse_args(&build_options, argc, argv)) {
        build_options_destroy(&build_options);
        return 1;
    }

    if (build_options.command == BUILD_COMMAND_HELP) {
        print_usage(argv[0]);
        build_options_destroy(&build_options);
        return 0;
    }

    Compiler compiler = compiler_create(&build_options);

    // Discover & parse
    Package* root_package = compiler_discover_packages(&compiler, string_get_view(build_options.root_path));
    if (root_package == NULL) {
        report_collector_print_all(&compiler.rc, build_options.with_color);
        compiler_destroy(&compiler);
        build_options_destroy(&build_options);
        return 1;
    }

    compiler_parse_source_files(&compiler);

    if (build_options.dump_ast) {
        compiler_dump_ast(&compiler);
    }
    if (build_options.dump_ast_dot) {
        compiler_dump_ast_dot(&compiler);
    }

    report_collector_print_all(&compiler.rc, build_options.with_color);

    int exit_code =
        (compiler.rc.sev_count[DIAG_SEV_ERROR] > 0) ||
        (build_options.werror && compiler.rc.sev_count[DIAG_SEV_WARNING] > 0);

    compiler_destroy(&compiler);
    build_options_destroy(&build_options);

    return exit_code;
}
