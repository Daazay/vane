#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/vector.h"
#include "vane/utils/hashmap.h"

#include "vane/diagnostic/diagnostic.h"

typedef enum BuildCommand BuildCommand;
typedef struct BuildOptions BuildOptions;

enum BuildCommand{
    BUILD_COMMAND_MISSING = 0,
    BUILD_COMMAND_HELP,
    BUILD_COMMAND_BUILD,
};

struct BuildOptions {
    BuildCommand command;

    // key:   [String, &string_destroy]
    // value: [String, &string_destroy]
    Hashmap collections;

    // key:   [String, &string_destroy]
    // value: [String, &string_destroy]
    Hashmap defines;

    String root_path;
    String output_dir;

    DiagnosticSeverity log_verbosity;
};

void print_usage(const char* argv0);

void build_options_init(BuildOptions* build_options);

bool build_options_parse_args(BuildOptions* build_options, int argc, const char** argv);

void build_options_destroy(BuildOptions* build_options);