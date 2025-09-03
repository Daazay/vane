#include "vane/compiler/build_options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vane/utils/hash.h"

#define println(msg, ...)  printf(msg "\n", ##__VA_ARGS__)
#define eprintln(msg, ...) printf("[error]: " msg "\n", ##__VA_ARGS__)

typedef struct ArgParser ArgParser;
typedef struct OptionSpec OptionSpec;
typedef struct CommandSpec CommandSpec;

typedef bool (*OptionHandler)(ArgParser* parser, const char* value);
typedef bool (*CommandHandler)(ArgParser* parser);

struct OptionSpec {
    const char* long_name;
    const char  short_name;
    bool requires_value;
    OptionHandler handler;
    const char* help;
};

struct CommandSpec {
    const char* name;
    BuildCommand command;
    CommandHandler handler;

    OptionSpec* options;
    u32 options_count;

    const char* help;
};

struct ArgParser {
    BuildOptions* options;

    const u32 args_count;
    const char** args;

    u32 idx;
    const char* current_arg;
};

static inline bool is_option(const char* arg) {
    return arg[0] == '-';
}

static inline bool is_long_option(const char* arg) {
    return arg[0] == '-' && arg[1] == '-';
}

static inline bool has_next_arg(const ArgParser* parser) {
    return parser->idx + 1 < parser->args_count;
}

static inline bool is_next_option(const ArgParser* parser) {
    return is_option(parser->args[parser->idx + 1]);
}

static inline const char* advance_arg(ArgParser* parser) {
    return parser->args[++parser->idx];
}

static inline bool match_option(const char* arg, const OptionSpec* spec) {
    if (is_long_option(arg)) {
        return strcmp(arg + 2, spec->long_name) == 0;
    }
    if ((arg[0] == '-') &&
        (spec->short_name != '\0') &&
        (arg[1] == spec->short_name) && (arg[2] == '\0')) {
        return true;
    }
    return false;
}

static inline bool split_kv(const char* in, char sep, String* key, String* value) {
    StringView sv = string_view_from_cstr(in);
    u64 pos = string_view_find_c(sv, sep);

    if (pos == (u64)NPOS || pos == 0 || pos == sv.len) {
        return false;
    }

    *key   = string_from_sv(string_view_subview(sv, 0, pos));
    *value = string_from_sv(string_view_subview(sv, pos + 1, sv.len - pos));

    return true;
}

//

static bool handle_output_dir(ArgParser* parser, const char* value) {
    if (value == NULL) {
        eprintln("missing value for --output_dir");
        return false;
    }

    string_destroy(&parser->options->output_dir);
    parser->options->output_dir = string_from_cstr(value);

    return true;
}

static bool handle_collection(ArgParser* parser, const char* value) {
    if (value == NULL) {
        eprintln("missing value for --collection");
        return false;
    }

    String name = STRING_EMPTY;
    String path = STRING_EMPTY;

    if (!split_kv(value, '=', &name, &path)) {
        eprintln("invalid format for --collection, expected NAME=PATH");
        return false;
    }

    hashmap_insert(&parser->options->collections, &name, &path);
    return true;
}

static bool handle_jobs(ArgParser* parser, const char* value) {
    if (value == NULL) {
        eprintln("missing value for --jobs");
        return false;
    }

    parser->options->jobs = (u32)atoi(value);
    if (parser->options->jobs == 0) {
        parser->options->jobs = 1;
    }

    return true;
}

static bool handle_define(ArgParser* parser, const char* value) {
    if (value == NULL) {
        eprintln("--define requires KEY=VALUE");
        return false;
    }

    String k = STRING_EMPTY;
    String v = STRING_EMPTY;

    if (!split_kv(value, '=', &k, &v)) {
        eprintln("invalid format for --define, expected KEY=VALUE");
        return false;
    }

    hashmap_insert(&parser->options->defines, &k, &v);
    return true;
}

static bool handle_help(ArgParser* parser) {
    (void)parser;
    return true;
}

static bool handle_build(ArgParser* parser) {
    if (!has_next_arg(parser) || is_next_option(parser)) {
        eprintln("no path provided for build command");
        return false;
    }

    parser->options->root_path = string_from_cstr(advance_arg(parser));
    return true;
}

static OptionSpec general_options[] = {
    { "output_dir", 'o', true, &handle_output_dir, "Set output directory path (default: ./build)." },
    { "collection", 'c', true, &handle_collection, "Add collection in NAME=PATH format." },
    { "jobs",       'j', true, &handle_jobs,       "Set number of parallel jobs (default: 1)." },
    { "define",     'D', true, &handle_define,     "Add KEY=VALUE build definitions." },
};

//static OptionSpec build_cmd_options[] = { };

static CommandSpec commands[] = {
    { "help",  BUILD_COMMAND_HELP,  &handle_help, NULL, 0,
      "Show help message" },
    { "build", BUILD_COMMAND_BUILD, &handle_build, NULL, 0,
      "Build a project" },
};

static bool parse_option(ArgParser* parser, OptionSpec* table, u32 count) {
    const char* arg = parser->current_arg;

    for (u32 i = 0; i < count; ++i) {
        OptionSpec* spec = &table[i];

        if (match_option(arg, spec)) {
            const char* value = NULL;
            if (spec->requires_value) {
                if (!has_next_arg(parser) || is_next_option(parser)) {
                    eprintln("option '%s' requires a value.", arg);
                    return false;
                }
                value = advance_arg(parser);
            }
            return spec->handler(parser, value);
        }
    }

    eprintln("unknown option '%s'.", arg);
    return false;
}

static bool parse_command(ArgParser* parser) {
    const char* arg = parser->current_arg;

    for (u32 i = 0; i < ARR_SIZE(commands); ++i) {
        CommandSpec* cmd = &commands[i];

        if (strcmp(arg, cmd->name) == 0) {
            parser->options->command = cmd->command;

            if (cmd->handler != NULL && !cmd->handler(parser)) {
                return false;
            }

            while (has_next_arg(parser)) {
                const char* lookahead = parser->args[parser->idx + 1];
                if (!is_option(lookahead)) {
                    break;
                }

                parser->current_arg = advance_arg(parser);
                if (!parse_option(parser, cmd->options, cmd->options_count)) {
                    return false;
                }
            }
            return true;
        }
    }

    eprintln("unknown command '%s'.", arg);
    return false;
}

void print_usage(const char* argv0) {
    println("Usage: %s [GENERAL OPTIONS] <COMMAND> [ARGS] [COMMAND OPTIONS]", argv0);
    println("");

    println("General options:");
    for (u32 i = 0; i < ARR_SIZE(general_options); ++i) {
        const OptionSpec* op = &general_options[i];
        println("  -%c, --%-16s %s",
            op->short_name ? op->short_name : ' ',
            op->long_name, op->help);
    }

    println("");
    println("Commands:");
    for (u32 i = 0; i < ARR_SIZE(commands); ++i) {
        const CommandSpec* cmd = &commands[i];
        println("  %-20s %s", cmd->name, cmd->help);

        for (u32 j = 0; j < cmd->options_count; ++j) {
            const OptionSpec* op = &cmd->options[j];
            println("    -%c, --%-14s %s",
                op->short_name ? op->short_name : ' ',
                op->long_name, op->help);
        }
    }
}

void build_options_init(BuildOptions* build_options) {
    assert(build_options != NULL);

    build_options->command = BUILD_COMMAND_MISSING;
    build_options->root_path = (String){ 0 };

    build_options->collections = hashmap_create(4,
        HASHMAP_KEY_SPECS(String, &__string_get_hash, &__string_eq_str, &string_destroy),
        HASHMAP_VALUE_SPECS(String, &string_destroy)
    );

    build_options->output_dir = string_from_cstr("./build");
    build_options->jobs = 1;
    build_options->defines = hashmap_create(4,
        HASHMAP_KEY_SPECS(String, &__string_get_hash, &__string_eq_str, &string_destroy),
        HASHMAP_VALUE_SPECS(String, &string_destroy)
    );
}

bool build_options_parse_args(BuildOptions* build_options, int argc, const char** argv) {
    assert(build_options != NULL);

    if (argc < 2) {
        build_options->command = BUILD_COMMAND_HELP;
        return true;
    }

    ArgParser arg_parser = {
        .options = build_options,
        .args = argv,
        .args_count = argc,
        .idx = 1,
        .current_arg = NULL,
    };

    for (; arg_parser.idx < arg_parser.args_count; ++arg_parser.idx) {
        arg_parser.current_arg = arg_parser.args[arg_parser.idx];

        if (is_option(arg_parser.current_arg)) {
            if (!parse_option(&arg_parser, general_options, ARR_SIZE(general_options))) {
                return false;
            }
        }
        else if (build_options->command == BUILD_COMMAND_MISSING) {
            if (!parse_command(&arg_parser)) {
                return false;
            }
            continue;
        }
        else {
            eprintln("Unexpected argument '%s'.", arg_parser.current_arg);
            return false;
        }
    }

    if (build_options->command == BUILD_COMMAND_MISSING) {
        eprintln("no command provided. Use 'help' to see available commands.");
        return false;
    }

    return true;
}

void build_options_destroy(BuildOptions* build_options) {
    if (build_options == NULL) {
        return;
    }

    string_destroy(&build_options->root_path);
    hashmap_destroy(&build_options->collections);
    hashmap_destroy(&build_options->defines);
    string_destroy(&build_options->output_dir);
}