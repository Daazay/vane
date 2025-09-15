#include "vane/compiler/build_options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vane/utils/terminal.h"
#include "vane/utils/hash.h"
#include "vane/utils/string_utils.h"

#define println(msg, ...)  printf(msg "\n", ##__VA_ARGS__)
#define eprintln(msg, ...) printf("[error]: " msg "\n", ##__VA_ARGS__)

typedef struct ArgParser   ArgParser;
typedef struct OptionSpec  OptionSpec;
typedef struct CommandSpec CommandSpec;

typedef bool (*OptionHandler)(ArgParser* parser, const char* value);
typedef bool (*CommandHandler)(ArgParser* parser);
typedef bool (*CommandArgsHandler)(ArgParser* parser, const char* value);

typedef bool (*ProcessArgFromSeqFn)(ArgParser* parser, StringView item, void* data);

struct OptionSpec {
    const char* long_name;
    const char    short_name;     // '\0' if nones
    bool          requires_value;
    OptionHandler handler;
    const char* help;
};

struct CommandSpec {
    const char* name;
    BuildCommand       command;
    CommandArgsHandler arg_handler;   // called for each positional arg

    OptionSpec* options;       // command-specific options
    u32                options_count;

    const char* arg_usage;
    const char* help;
};

struct ArgParser {
    BuildOptions* options;

    const u32     args_count;
    const char** args;

    u32           idx;         // current index into args
    const char* current_arg; // shorthand to args[idx]
};

static inline bool is_option(const char* arg) {
    assert(arg != NULL);
    return arg[0] == '-';
}

static inline bool is_long_option(const char* arg) {
    assert(arg != NULL);
    return arg[0] == '-' && arg[1] == '-';
}

static inline bool has_next_arg(const ArgParser* parser) {
    assert(parser != NULL);
    return parser->idx + 1 < parser->args_count;
}

static inline bool is_next_option(const ArgParser* parser) {
    assert(parser != NULL);
    return is_option(parser->args[parser->idx + 1]);
}

static inline const char* advance_arg(ArgParser* parser) {
    assert(parser != NULL);
    return parser->args[++parser->idx];
}

static inline bool match_option(const char* arg, const OptionSpec* spec) {
    assert(arg != NULL && spec != NULL);

    if (is_long_option(arg)) {
        return strcmp(arg + 2, spec->long_name) == 0;
    }
    if ((arg[0] == '-') && (spec->short_name != '\0') &&
        (arg[1] == spec->short_name) && (arg[2] == '\0')) {
        return true;
    }
    return false;
}

static inline bool split_kv(StringView in, char sep, StringView* key, StringView* value) {
    assert(key != NULL && value != NULL);

    if (is_string_view_empty(in)) {
        return false;
    }

    u64 pos = string_view_find_c(in, sep);
    if (pos == (u64)NPOS || pos == 0 || pos + 1 >= in.len) {
        return false;
    }

    StringView k = string_view_trim(string_view_subview(in, 0, pos));
    StringView v = string_view_trim(string_view_subview(in, pos + 1, in.len - pos));

    if (is_string_view_empty(k) || is_string_view_empty(v)) {
        return false;
    }

    *key = k;
    *value = v;

    return true;
}

static inline bool parse_arg_seq(ArgParser* parser, StringView seq, char sep, ProcessArgFromSeqFn process_fn, void* data) {
    assert(parser != NULL && process_fn != NULL);

    if (is_string_view_empty(seq)) {
        eprintln("invalid sequence: empty");
        return false;
    }

    u64 start = 0;

    while (start < seq.len) {
        u64 pos = string_view_find_c_with_offset(seq, start, sep);
        u64 end = (pos == (u64)NPOS) ? seq.len : pos;

        if (end == start) {
            if (start == 0) {
                eprintln("invalid sequence: empty leading item");
            }
            else if (end == seq.len) {
                eprintln("invalid sequence: empty trailing item");
            }
            else {
                eprintln("invalid sequence: empty item (consecutive separators)");
            }
            return false;
        }

        StringView item = string_view_trim(string_view_subview(seq, start, end - start));
        if (is_string_view_empty(item)) {
            eprintln("invalid sequence: empty item");
            return false;
        }

        if (!process_fn(parser, item, data)) {
            return false;
        }

        // final segment processed
        if (pos == (u64)NPOS) {
            return true;
        }

        start = pos + 1;

        // trailing sep -> empty trailing item
        if (start == seq.len) {
            eprintln("invalid sequence: empty trailing item");
            return false;
        }
    }

    unreachable();
    return false;
}

static inline bool set_dump_flag(EmitFlags* mask, StringView token) {
    assert(mask != NULL);

    if (string_view_eq_sv(token, STR_LIT("all"))) {
        SET_FLAG(*mask,
            EMIT_FLAG_AST_TXT        | EMIT_FLAG_AST_DOT |
            EMIT_FLAG_SYMBOLS        |
            EMIT_FLAG_TYPES          |
            EMIT_FLAG_CFG_DOT        |
            EMIT_FLAG_CALL_GRAPH_TXT | EMIT_FLAG_CALL_GRAPH_DOT
        );
        return true;
    }
    if (string_view_eq_sv(token, STR_LIT("ast")) || string_view_eq_sv(token, STR_LIT("ast-text"))) {
        SET_FLAG(*mask, EMIT_FLAG_AST_TXT);
        return true;
    }
    else if (string_view_eq_sv(token, STR_LIT("ast-dot"))) {
        SET_FLAG(*mask, EMIT_FLAG_AST_DOT);
        return true;
    }
    else if (string_view_eq_sv(token, STR_LIT("symbols"))) {
        SET_FLAG(*mask, EMIT_FLAG_SYMBOLS);
        return true;
    }
    else if (string_view_eq_sv(token, STR_LIT("types"))) {
        SET_FLAG(*mask, EMIT_FLAG_TYPES);
        return true;
    }
    else if (string_view_eq_sv(token, STR_LIT("cfg-dot"))) {
        SET_FLAG(*mask, EMIT_FLAG_CFG_DOT);
        return true;
    }
    else if (string_view_eq_sv(token, STR_LIT("call-graph"))) {
        SET_FLAG(*mask, EMIT_FLAG_CALL_GRAPH_TXT);
        return true;
    }
    else if (string_view_eq_sv(token, STR_LIT("call-graph-dot"))) {
        SET_FLAG(*mask, EMIT_FLAG_CALL_GRAPH_DOT);
        return true;
    }

    eprintln("unknown dump item '"SV_FMT"' (valid: ast, ast-dot, symbols, types, cfg-dot, call-graph, call-graph-dot, all)", SV_ARG(token));
    return false;
}

static inline bool process_dump_item(ArgParser* parser, StringView item, void* data) {
    assert(parser != NULL);

    (void)data;
    return set_dump_flag(&parser->options->emit_mask, item);
}

static inline bool handle_emit(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    if (value == NULL) {
        eprintln("--emit requires a comma-separated value (e.g. --emit ast,types)");
        return false;
    }
    return parse_arg_seq(parser, string_view_from_cstr(value), ',', &process_dump_item, &parser->options->emit_mask);
}

static inline bool handle_emit_out(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    if (value == NULL) {
        eprintln("missing value for --emit-output (expected: console | file | both)");
        return false;
    }

    StringView v = string_view_from_cstr(value);
    if (string_view_eq_sv(v, STR_LIT("console")) || string_view_eq_sv(v, STR_LIT("stdout"))) {
        parser->options->emit_out_mode = EMIT_OUT_CONSOLE;
        return true;
    }
    if (string_view_eq_sv(v, STR_LIT("file"))) {
        parser->options->emit_out_mode = EMIT_OUT_FILE;
        return true;
    }
    if (string_view_eq_sv(v, STR_LIT("both"))) {
        parser->options->emit_out_mode = EMIT_OUT_BOTH;
        return true;
    }
    eprintln("invalid --emit-output value '"SV_FMT"' (expected: console | file | both)", SV_ARG(v));
    return false;
}

static inline bool handle_emit_dir(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    if (value == NULL) {
        eprintln("missing value for --emit-dir (expected a directory path)");
        return false;
    }
    if (!is_string_empty(parser->options->emit_dir)) {
        string_destroy(&parser->options->emit_dir);
    }

    // if user wants to emit in both (file and console), he must specify if with flag (--emit-out BOTH)
    if (parser->options->emit_out_mode == EMIT_OUT_CONSOLE) {
        parser->options->emit_out_mode = EMIT_OUT_FILE;
    }

    parser->options->emit_dir = string_from_cstr(value);
    return true;
}

static inline bool handle_entry_symbol(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    if (value == NULL) {
        eprintln("missing value for --entry (expected a name)");
        return false;
    }
    if (!is_string_empty(parser->options->entry_symbol)) {
        string_destroy(&parser->options->entry_symbol);
    }

    parser->options->entry_symbol = string_from_cstr(value);
    return true;
}

static inline bool process_collection_item(ArgParser* parser, StringView item, void* data) {
    assert(parser != NULL);

    (void)data;

    StringView k = STRING_VIEW_EMPTY;
    StringView v = STRING_VIEW_EMPTY;

    if (!split_kv(item, '=', &k, &v)) {
        eprintln("invalid --collection item '"SV_FMT"' (expected NAME=PATH)", SV_ARG(item));
        return false;
    }

    String k_s = string_from_sv(k);
    String v_s = string_from_sv(v);

    hashmap_insert(&parser->options->collections, &k_s, &v_s);
    return true;
}

static bool handle_collection(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    if (value == NULL) {
        eprintln("missing value for --collection");
        return false;
    }

    StringView seq = string_view_from_cstr(value);
    return parse_arg_seq(parser, seq, ',', &process_collection_item, NULL);
}

static inline bool process_define_item(ArgParser* parser, StringView item, void* data) {
    assert(parser != NULL);

    (void)data;

    StringView k = STRING_VIEW_EMPTY;
    StringView v = STRING_VIEW_EMPTY;

    if (!split_kv(item, '=', &k, &v)) {
        eprintln("invalid --define item '"SV_FMT"' (expected KEY=VALUE)", SV_ARG(item));
        return false;
    }

    String k_s = string_from_sv(k);
    String v_s = string_from_sv(v);

    hashmap_insert(&parser->options->defines, &k_s, &v_s);
    return true;
}

static bool handle_define(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    if (value == NULL) {
        eprintln("missing value for --define");
        return false;
    }

    return parse_arg_seq(parser, string_view_from_cstr(value), ',', &process_define_item, NULL);
}

static bool handle_verbosity(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    if (value == NULL) {
        eprintln("missing value for --verbosity (expected a number)");
        return false;
    }

    char* endptr = NULL;
    long v = strtol(value, &endptr, 10);

    if (*endptr != '\0' || v < 0) {
        eprintln("invalid verbosity level '%s' (must be non-negative integer)", value);
        return false;
    }

    parser->options->log_verbosity = (u32)v;
    return true;
}

static bool handle_werror(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    (void)parser; (void)value;
    parser->options->werror = true;
    return true;
}

static bool handle_no_color(ArgParser* parser, const char* value) {
    assert(parser != NULL);

    (void)parser; (void)value;
    parser->options->with_color = false;
    return true;
}

static inline bool handle_project_path_arg(ArgParser* parser, const char* value) {
    assert(parser != NULL && parser->current_arg != NULL);

    // expect and arg
    if (value == NULL) {
        eprintln("missing required argument: <path>");
        return false;
    }

    StringView existing_sv = string_get_view(parser->options->project_path);
    if (!is_string_view_empty(existing_sv)) {
        eprintln("duplicate project path '%s' (path already set to '"SV_FMT"')", value, SV_ARG(existing_sv));
        return false;
    }

    parser->options->project_path = string_from_cstr(value);
    return true;
}

static OptionSpec general_options[] = {
    { "collection",  'I', true,  &handle_collection,   "Add import collection(s): NAME=PATH[,NAME=PATH...] (like an include path).",                                                 },
    { "define",      'D', true,  &handle_define,       "Define compile-time constants(s): KEY=VALUE[,KEY=VALUE...] (not yet implemented).",                                          },

    { "emit",        'E', true,  &handle_emit,         "Select IR/analysis outputs (comma-separated): ast, ast-dot, symbols, types, cfg, cfg-dot, call-graph, call-graph-dot, all.", },
    { "emit-output", 'o', true,  &handle_emit_out,     "Output destination for --emit results: 'console', 'file', or 'both'.",                                                       },
    { "emit-dir",    'o', true,  &handle_emit_dir,     "Directory to place files when --emit-output=file or both (default: dumps).",                                                 },

    { "entry",       'm', true,  &handle_entry_symbol, "Set entry point function (default: main).",                                                                                  },

    { "verbosity",   'v', true,  &handle_verbosity,    "Set log verbosity (default: 3): 0=errors, 1=warnings, 2=info, 3=notes, 4=debug.",                                            },
    { "Werror",       0,  false, &handle_werror,       "Treat all warnings as errors.",                                                                                              },
    { "no-color",     0,  false, &handle_no_color,     "Disable ANSI color codes in diagnostics.",                                                                                   },
};

static CommandSpec commands[] = {
    { "help",  BUILD_COMMAND_HELP,  NULL,                     NULL, 0, NULL,     "Show this help message and exit."                                                                  },
    { "parse", BUILD_COMMAND_PARSE, &handle_project_path_arg, NULL, 0, "<path>", "Parse source files into an AST.",                                                                  },
    { "check", BUILD_COMMAND_CHECK, &handle_project_path_arg, NULL, 0, "<path>", "Run semantic analysis (imports, symbols, type checking, validation).",                             },
    { "cfg",   BUILD_COMMAND_CFG,   &handle_project_path_arg, NULL, 0, "<path>", "Build and dump control-flow-graph (CFG) for all functions.",                                       },
    { "build", BUILD_COMMAND_BUILD, &handle_project_path_arg, NULL, 0, "<path>", "Run the full compilation pipeline and emit the final program.",                                    },
};

static inline bool parse_one_general_option(ArgParser* parser) {
    assert(parser != NULL && parser->current_arg != NULL);

    for (u32 i = 0; i < ARR_SIZE(general_options); ++i) {
        OptionSpec* spec = &general_options[i];

        if (!match_option(parser->current_arg, spec)) {
            continue;
        }

        const char* value = NULL;
        if (spec->requires_value) {
            if (!has_next_arg(parser) || is_next_option(parser)) {
                eprintln("option '%s' requires a value.", parser->current_arg);
                return false;
            }
            value = advance_arg(parser);
        }
        return spec->handler(parser, value);
    }

    eprintln("unknown general option '%s'.", parser->current_arg);
    return false;
}

static bool parse_one_option(ArgParser* parser, OptionSpec* table, u32 count, bool include_general) {
    assert(parser != NULL && parser->current_arg != NULL);

    if (include_general) {
        for (u32 i = 0; i < ARR_SIZE(general_options); ++i) {
            OptionSpec* spec = &general_options[i];

            if (!match_option(parser->current_arg, spec)) {
                continue;
            }

            const char* value = NULL;
            if (spec->requires_value) {
                if (!has_next_arg(parser) || is_next_option(parser)) {
                    eprintln("option '%s' requires a value.", parser->current_arg);
                    return false;
                }
                value = advance_arg(parser);
            }
            return spec->handler(parser, value);
        }
    }

    for (u32 i = 0; i < count; ++i) {
        OptionSpec* spec = &table[i];

        if (!match_option(parser->current_arg, spec)) {
            continue;
        }

        const char* value = NULL;
        if (spec->requires_value) {
            if (!has_next_arg(parser) || is_next_option(parser)) {
                eprintln("option '%s' requires a value.", parser->current_arg);
                return false;
            }
            value = advance_arg(parser);
        }
        return spec->handler(parser, value);
    }

    eprintln("unknown option '%s'.", parser->current_arg);
    return false;
}

static bool parse_command(ArgParser* parser) {
    assert(parser != NULL && parser->current_arg != NULL);

    for (u32 i = 0; i < ARR_SIZE(commands); ++i) {
        CommandSpec* cmd = &commands[i];

        if (strcmp(parser->current_arg, cmd->name) != 0) {
            continue;
        }

        parser->options->command = cmd->command;
        bool saw_positional = false;

        // position args (must come before any options)
        while (has_next_arg(parser) && !is_next_option(parser)) {
            parser->current_arg = advance_arg(parser);

            if (cmd->arg_handler == NULL) {
                eprintln("unexpected argument '%s' for command '%s'", parser->current_arg, cmd->name);
                return false;
            }

            saw_positional = true;
            if (!cmd->arg_handler(parser, parser->current_arg)) {
                return false;
            }
        }

        // If the command expects positionals but none were given, let the handler report.
        if (cmd->arg_handler != NULL && !saw_positional) {
            if (!cmd->arg_handler(parser, NULL)) {
                return false;
            }
        }

        // Options (command specific + general)
        while (has_next_arg(parser) && is_next_option(parser)) {
            parser->current_arg = advance_arg(parser);
            if (!parse_one_option(parser, cmd->options, cmd->options_count, true)) {
                return false;
            }
        }

        // After options, no more arguments are allowed
        if (has_next_arg(parser) && !is_next_option(parser)) {
            eprintln("arguments must precede options for command '%s'", cmd->name);
            return false;
        }

        return true;
    }

    eprintln("unknown command '%s'.", parser->current_arg);
    return false;
}

void print_usage(const char* argv0) {
    if (argv0 == NULL) {
        argv0 = "vane";
    }

    println("Usage: %s [general options] <command> [args] [options]", argv0);
    println("");

    println("General options (may appear before or after the command):");
    for (u32 i = 0; i < ARR_SIZE(general_options); ++i) {
        const OptionSpec* op = &general_options[i];
        if (op->short_name != '\0') {
            println("    -%c, --%-14s %s", op->short_name, op->long_name, op->help);
        }
        else {
            println("        --%-14s %s", op->long_name, op->help);
        }
    }
    println("");

    println("Commands:");
    for (u32 i = 0; i < ARR_SIZE(commands); ++i) {
        const CommandSpec* cmd = &commands[i];
        const char* arg_usage = (cmd->arg_usage != NULL)
            ? cmd->arg_usage
            : "";

        // columns: name (10), args (12), help (rest)
        println("  %-5s %-12s %s", cmd->name, arg_usage, cmd->help);

        // list command-specific options (if any)
        for (u32 j = 0; j < cmd->options_count; ++j) {
            const OptionSpec* op = &cmd->options[j];
            if (op->short_name != '\0') {
                println("    -%c, --%-12s %s", op->short_name, op->long_name, op->help);
            }
            else {
                println("        --%-12s %s", op->long_name, op->help);
            }
        }
    }
}

BuildOptions build_options_create() {
    BuildOptions build_options = { 0 };

    build_options.project_path = STRING_EMPTY;
    build_options.vane_root_path = STRING_EMPTY;

    build_options.collections = hashmap_create(BUILD_OPTIONS_DEFAULT_COLLECTION_COUNT,
        HASHMAP_KEY_SPECS(String, &string_view_item_hash, &string_view_item_eq, &string_destroy),
        HASHMAP_VALUE_SPECS(String, &string_destroy)
    );

    build_options.defines = hashmap_create(BUILD_OPTIONS_DEFAULT_DEFINE_COUNT,
        HASHMAP_KEY_SPECS(String, &string_view_item_hash, &string_view_item_eq, &string_destroy),
        HASHMAP_VALUE_SPECS(String, &string_destroy)
    );

    build_options.log_verbosity = 3;
    build_options.with_color = is_terminal_support_colors();
    build_options.werror = false;

    build_options.emit_mask     = EMIT_FLAG_NONE;
    build_options.emit_out_mode = EMIT_OUT_CONSOLE;
    build_options.emit_dir      = string_from_cstr("dumps");

    build_options.entry_symbol  = STRING_EMPTY;

    build_options.command = BUILD_COMMAND_MISSING;

    return build_options;
}

void build_options_destroy(BuildOptions* build_options) {
    if (build_options == NULL) {
        return;
    }

    string_destroy(&build_options->project_path);
    string_destroy(&build_options->vane_root_path);

    string_destroy(&build_options->emit_dir);
    string_destroy(&build_options->entry_symbol);

    hashmap_destroy(&build_options->collections);
    hashmap_destroy(&build_options->defines);
}

bool build_options_parse_cli(BuildOptions* build_options, int argc, const char** argv) {
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

    // parse general option before the command
    for (; arg_parser.idx < arg_parser.args_count; ++arg_parser.idx) {
        arg_parser.current_arg = arg_parser.args[arg_parser.idx];

        if (!is_option(arg_parser.current_arg)) {
            break;
        }
        if (!parse_one_general_option(&arg_parser)) {
            return false;
        }
    }

    if (arg_parser.idx >= arg_parser.args_count) {
        eprintln("no command provided. Use 'help' to see available commands.");
        return false;
    }

    arg_parser.current_arg = arg_parser.args[arg_parser.idx];
    if (!parse_command(&arg_parser)) {
        return false;
    }

    // no trailing garbage
    if (arg_parser.idx + 1 < arg_parser.args_count) {
        eprintln("unexpected argument '%s'", arg_parser.args[arg_parser.idx + 1]);
        return false;
    }

    return true;
}