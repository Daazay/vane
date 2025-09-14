#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/vector.h"
#include "vane/utils/hashmap.h"

#include "vane/diagnostic/diagnostic.h"

typedef enum   BuildCommand BuildCommand;
typedef enum   EmitFlags    EmitFlags;
typedef enum   EmitMode     EmitMode;
typedef struct BuildOptions BuildOptions;

#define BUILD_OPTIONS_DEFAULT_COLLECTION_COUNT 4
#define BUILD_OPTIONS_DEFAULT_DEFINE_COUNT     4

enum BuildCommand {
    BUILD_COMMAND_MISSING = 0,
    BUILD_COMMAND_HELP,
    BUILD_COMMAND_PARSE,
    BUILD_COMMAND_CHECK,
    BUILD_COMMAND_CFG,
    BUILD_COMMAND_BUILD,
};

enum EmitFlags {
    EMIT_FLAG_NONE     = 0,
    EMIT_FLAG_AST_TEXT = 1 << 0,
    EMIT_FLAG_AST_DOT  = 1 << 1,
    EMIT_FLAG_SYMBOLS  = 1 << 2,
    EMIT_FLAG_TYPES    = 1 << 3,
    EMIT_FLAG_CFG_TEXT = 1 << 4,
    EMIT_FLAG_CFG_DOT  = 1 << 5,
};

enum EmitMode {
    EMIT_OUT_CONSOLE = 0,
    EMIT_OUT_FILE,
    EMIT_OUT_BOTH,
};

struct BuildOptions {
    String project_path;   // required for parse/check/cfg/build
    String vane_root_path; // optional

    // k: [String, &string_destroy]
    // v: [String, &string_destroy]
    Hashmap collections;

    // k: [String, &string_destroy]
    // v: [String, &string_destroy]
    Hashmap defines;

    DiagnosticSeverity log_verbosity;
    bool with_color;
    bool werror;

    EmitFlags emit_mask;
    EmitMode  emit_out_mode;
    String    emit_dir;

    BuildCommand command;
};

void print_usage(const char* argv0);

BuildOptions build_options_create();

void build_options_destroy(BuildOptions* build_options);

bool build_options_parse_cli(BuildOptions* build_options, int argc, const char** argv);