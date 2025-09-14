#include "vane/dump/scope_dump.h"

#include "vane/compiler/package.h"

#include <stdio.h>

static inline void indent(u32 lvl) {
    for (u32 i = 0; i < lvl; ++i) {
        putc(' ', stdout);
    }
}

static inline void print_loc(TokenLoc loc) {
    printf(SV_FMT ":%d:%d-%d:%d", SV_ARG(loc.path),
        loc.begin.line, loc.begin.column,
        loc.end.line, loc.end.column
    );
}

static inline void print_sv_quoted(StringView sv) {
    printf("\"" SV_FMT "\"", SV_ARG(sv));
}

static inline void dump_symbol_one(const Symbol* symbol, u32 indent_lvl, bool compact) {
    assert(symbol != NULL);

    if (compact) {
        indent(indent_lvl);

        printf("- { kind: %s, name: " SV_FMT, symbol_kind_get_name(symbol->kind), SV_ARG(symbol->name));

        if (symbol->kind == SYMBOL_IMPORT && symbol->as.import.target != NULL) {
            printf(", target: " SV_FMT, SV_ARG(symbol->as.import.target->path));
        }
        if (symbol->ast != NULL) {
            printf(", loc: ");
            print_loc(symbol->ast->loc);
        }

        puts(" }");
        return;
    }

    // multi-line block mapping
    indent(indent_lvl);
    printf("- kind: %s\n", symbol_kind_get_name(symbol->kind));

    indent(indent_lvl + 1);
    printf(" name: " SV_FMT "\n", SV_ARG(symbol->name));

    if (symbol->kind == SYMBOL_IMPORT && symbol->as.import.target != NULL) {
        indent(indent_lvl + 1);
        printf(" target: " SV_FMT "\n", SV_ARG(symbol->as.import.target->path));
    }

    if (symbol->ast != NULL) {
        indent(indent_lvl + 1);
        printf(" loc: ");
        print_loc(symbol->ast->loc);
        putchar('\n');
    }
}

static inline void dump_scope_impl(const Scope* scope, u32 indent_lvl, bool as_list) {
    assert(scope != NULL);

    indent(indent_lvl);

    // heading
    if (as_list) {
        printf("- kind: %s\n", scope_kind_get_name(scope->kind));
    }
    else {
        printf(" kind: %s\n", scope_kind_get_name(scope->kind));
    }


    u32 base = indent_lvl + (as_list ? 1 : 0);

    if (scope->kind == SCOPE_FUNCTION && scope->ast != NULL && scope->ast->symbol != NULL) {
        indent(base);
        printf(" name: " SV_FMT "\n", SV_ARG(scope->ast->symbol->name));
    }

    // symbols
    {
        bool has_symbols = (scope->symbol_sets.size > 0);

        indent(base);
        printf(" symbols:");

        if (!has_symbols) {
            puts(" []");
        }
        else {
            printf("\n");
            bool compact = (scope->kind == SCOPE_FUNCTION);

            HashmapIterator it = hashmap_get_it(&scope->symbol_sets);
            StringView key     = STRING_VIEW_EMPTY;
            SymbolSet set      = { 0 };

            while (hashmap_it_next(&it, &key, &set)) {
                for (u32 i = 0; i < SYMBOL_KIND_COUNT - 1; ++i) {
                    Symbol* symbol = set.by_kind[i];
                    if (symbol == NULL) {
                        continue;
                    }

                    dump_symbol_one(symbol, base + 1, compact);
                }
            }
        }
    }

    // child scopes
    {
        bool has_children = (scope->scopes.size > 0);

        indent(base);
        printf("scopes:");

        if (!has_children) {
            puts(" []");
        }
        else {
            printf("\n");
            for (u32 i = 0; i < scope->scopes.size; ++i) {
                Scope* child = vector_at(scope->scopes, i);
                dump_scope_impl(child, base + 1, true);
            }
        }
    }
}

void dump_scope(const Scope* scope) {
    assert(scope != NULL);
    dump_scope_impl(scope, 1, false);
}