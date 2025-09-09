#include <stdio.h>
#include <stdlib.h>

#include <vane/compiler/build_options.h>
#include <vane/compiler/compiler.h>
#include <vane/diagnostic/report_collector.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int main(int argc, const char** argv) {
    BuildOptions build_options = build_options_create();
    if (!build_options_parse_args(&build_options, argc, argv)) {
        build_options_destroy(&build_options);
        return EXIT_FAILURE;
    }

    if (build_options.command == BUILD_COMMAND_HELP) {
        print_usage(argv[0]);
        build_options_destroy(&build_options);
        return EXIT_FAILURE;
    }

    Compiler compiler = compiler_create(&build_options);
    compiler_run_command(&compiler);

    report_collector_print_all(&compiler.rc, build_options.with_color);

    int exit_code = (compiler.rc.sev_count[DIAG_SEV_ERROR] > 0) ||
        (build_options.werror && compiler.rc.sev_count[DIAG_SEV_WARNING] > 0);

    compiler_destroy(&compiler);
    build_options_destroy(&build_options);
    return exit_code;
}
