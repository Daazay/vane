#include <stdio.h>
#include <stdlib.h>

#include <vane/compiler/build_options.h>
#include <vane/compiler/compiler.h>
#include <vane/diagnostic/report_collector.h>

static void indent(FILE* out, unsigned n) {
    for (unsigned i = 0; i < n; ++i) fputs("  ", out);
}

static void fprint_loc(FILE* out, TokenLoc loc) {
    fprintf(out, SV_FMT ":%u:%u-%u:%u",
        SV_ARG(loc.path),
        (unsigned)loc.begin.line, (unsigned)loc.begin.column,
        (unsigned)loc.end.line, (unsigned)loc.end.column);
}

static void dump_symbol_one(const Symbol* sym, FILE* out, unsigned indent_lvl) {
    indent(out, indent_lvl);
    fprintf(out, "%-10s " SV_FMT, symbol_kind_get_name(sym->kind), SV_ARG(sym->name));

    if (sym->ast) {
        fputs("  @ ", out);
        fprint_loc(out, sym->ast->loc);
    }

    // a bit of extra info for imports
    if (sym->kind == SYMBOL_IMPORT && sym->as.import.target) {
        fputs("  -> ", out);
        fprintf(out, SV_FMT, SV_ARG(sym->as.import.target->path));
    }

    fputc('\n', out);
}

static void dump_scope(const Scope* scope, void* out, unsigned indent_lvl) {
    if (!scope) {
        indent(out, indent_lvl);
        fputs("<null-scope>\n", out);
        return;
    }

    indent(out, indent_lvl);
    fprintf(out, "%s ", scope_kind_get_name(scope->kind));

    if (scope->ast != NULL && scope->kind == SCOPE_FUNCTION) {
        fprintf(out, SV_FMT" ", SV_ARG(scope->ast->symbol->name));
    }

    fprintf(out, "{\n");

    // symbols
    {
        HashmapIterator it = hashmap_get_it(&scope->symbols);
        StringView key = STRING_VIEW_EMPTY;
        Symbol* sym = NULL;

        while (hashmap_it_next(&it, &key, &sym)) {
            if (!sym) continue;
            dump_symbol_one(sym, out, indent_lvl + 1);
        }
    }

    // child scopes
    for (u32 i = 0; i < scope->scopes.size; ++i) {
        Scope* child = vector_at(scope->scopes, i);
        dump_scope(child, out, indent_lvl + 1);
    }

    indent(out, indent_lvl);
    fputs("}\n", out);
}

// Dump all scopes in a package (package scope + each file scope).
static void dump_package_scopes(const Package* pkg, void* out) {
    if (!pkg) return;

    fprintf(out, "[package " SV_FMT "]\n", SV_ARG(pkg->path));
    dump_scope(pkg->scope, out, 0);

    //for (u32 i = 0; i < pkg->source_files.size; ++i) {
    //    SourceFile* sf = vector_at(pkg->source_files, i);
    //    fprintf(out, "  [source " SV_FMT "]\n", SV_ARG(sf->path));
    //    dump_scope(sf->scope, out, 1);
    //}
}

// Dump scopes for all packages in the compiler.
static void compiler_dump_scopes(const Compiler* compiler, void* out) {
    if (!compiler) return;
    HashmapIterator it = hashmap_get_it(&compiler->packages);
    Package* pkg = NULL;

    while (hashmap_it_next(&it, NULL, &pkg)) {
        if (!pkg) continue;
        dump_package_scopes(pkg, out);
    }
}

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
    Package* root_package = compiler_load_package(&compiler, string_get_view(build_options.root_path));
    if (root_package == NULL) {
        report_collector_print_all(&compiler.rc, build_options.with_color);
        compiler_destroy(&compiler);
        build_options_destroy(&build_options);
        return 1;
    }

    bool parse_status = compiler_parse_source_files(&compiler);

    if (build_options.dump_ast) {
        compiler_dump_ast(&compiler);
    }
    if (build_options.dump_ast_dot) {
        compiler_dump_ast_dot(&compiler);
    }

    if (!parse_status || build_options.command == BUILD_COMMAND_PARSE_AST) {
        report_collector_print_all(&compiler.rc, build_options.with_color);
        compiler_destroy(&compiler);
        build_options_destroy(&build_options);
        return (compiler.rc.sev_count[DIAG_SEV_ERROR] > 0) || (build_options.werror && compiler.rc.sev_count[DIAG_SEV_WARNING] > 0);
    }

    compiler_resolve_imports(&compiler);
    //
    compiler_resolve_symbol_decls(&compiler);

    compiler_bind_symbols(&compiler);

    compiler_dump_scopes(&compiler, stdout);


    report_collector_print_all(&compiler.rc, build_options.with_color);

    compiler_destroy(&compiler);
    build_options_destroy(&build_options);

    return (compiler.rc.sev_count[DIAG_SEV_ERROR] > 0) || (build_options.werror && compiler.rc.sev_count[DIAG_SEV_WARNING] > 0);
}
