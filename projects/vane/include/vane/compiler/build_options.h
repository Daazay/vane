#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/vector.h"
#include "vane/utils/hashmap.h"

#include "vane/diagnostic/diagnostic.h"

typedef enum BuildCommand BuildCommand;
typedef struct BuildOptions BuildOptions;

#define BUILD_OPTIONS_DEFAULT_COLLECTION_COUNT 4
#define BUILD_OPTIONS_DEFAULT_DEFINE_COUNT     4

enum BuildCommand{
    BUILD_COMMAND_MISSING = 0,
    BUILD_COMMAND_HELP,
    BUILD_COMMAND_PARSE_AST,
    BUILD_COMMAND_BUILD,
};

struct BuildOptions {
    String project_path;
    String vane_root_path;

    // key:   [String, &string_destroy]
    // value: [String, &string_destroy]
    Hashmap collections;

    // key:   [String, &string_destroy]
    // value: [String, &string_destroy]
    Hashmap defines;

    DiagnosticSeverity log_verbosity;
    bool with_color;

    // debug
    bool dump_tokens;
    bool dump_ast;
    bool dump_ast_dot;
    bool dump_symbols;
    bool dump_types;
    bool werror;

    BuildCommand command;
};

void print_usage(const char* argv0);

BuildOptions build_options_create();

void build_options_destroy(BuildOptions* build_options);

bool build_options_parse_args(BuildOptions* build_options, int argc, const char** argv);