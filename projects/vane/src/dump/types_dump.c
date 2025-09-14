#include "vane/dump/types_dump.h"

#include "vane/compiler/package.h"
#include "vane/sema/type.h"

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

static inline void print_type_inline_canonical(const Type* type);
static inline void print_type_inline_alias(const Type* type);

/* Print function type canonical: params/ret are canonical too. */
static inline  void print_fun_type_canonical(const Type* t) {
    putchar('(');
    for (u32 i = 0; i < t->as.fun.param_count; ++i) {
        if (i) printf(", ");
        print_type_inline_canonical(t->as.fun.params[i]);
    }
    printf(") -> ");
    print_type_inline_canonical(t->as.fun.ret);
}

/* Print function type alias-friendly: keep aliases on the LHS view. */
static inline  void print_fun_type_alias(const Type* t) {
    putchar('(');
    for (u32 i = 0; i < t->as.fun.param_count; ++i) {
        if (i) printf(", ");
        print_type_inline_alias(t->as.fun.params[i]);
    }
    printf(") -> ");
    print_type_inline_alias(t->as.fun.ret);
}

/* Canonical printer: expands aliases entirely. */
static inline  void print_type_inline_canonical(const Type* t) {
    const Type* c = type_unwrap(t);
    if (!c) { printf("<null>"); return; }

    switch (c->kind) {
    case TYPE_UNRESOLVED:
        printf("<unresolved " SV_FMT ">", SV_ARG(c->as.unresolved.name));
        return;

    case TYPE_BUILTIN: {
        printf("%s", type_builtin_kind_get_name(c->as.builtin.kind));
        return;
    }

    case TYPE_POINTER:
        putchar('^'); print_type_inline_canonical(c->as.pointer.base); return;

    case TYPE_ARRAY:
        printf("[%u]", (unsigned)c->as.array.size);
        print_type_inline_canonical(c->as.array.elem);
        return;

    case TYPE_SLICE:
        printf("[]");
        print_type_inline_canonical(c->as.slice.elem);
        return;

    case TYPE_FUNCTION:
        print_fun_type_canonical(c);
        return;

    case TYPE_ALIAS:
        // Canonical view should never end here, but handle gracefully:
        print_type_inline_canonical(c->as.alias.target);
        return;

    default:
        unreachable();
        return;
    }
}

static inline  void print_type_inline_alias(const Type* t) {
    if (!t) { printf("<null>"); return; }

    switch (t->kind) {
    case TYPE_UNRESOLVED:
        printf("<unresolved " SV_FMT ">", SV_ARG(t->as.unresolved.name));
        return;

    case TYPE_BUILTIN: {
        printf("%s", type_builtin_kind_get_name(t->as.builtin.kind));
        return;
    }

    case TYPE_POINTER:
        putchar('^'); print_type_inline_alias(t->as.pointer.base); return;

    case TYPE_ARRAY:
        printf("[%u]", (unsigned)t->as.array.size);
        print_type_inline_alias(t->as.array.elem);
        return;

    case TYPE_SLICE:
        printf("[]");
        print_type_inline_alias(t->as.slice.elem);
        return;

    case TYPE_FUNCTION:
        print_fun_type_alias(t);
        return;

    case TYPE_ALIAS:
        // Print alias name as-is.
        printf(SV_FMT, SV_ARG(t->as.alias.name));

        // If alias is effectively a self-alias (e.g., "u8 = u8"), suppress " (= ...)" noise.
        const Type* tgt = t->as.alias.target;
        const char* tgt_builtin = type_kind_get_name(type_unwrap(tgt)->kind);

        // Compare alias name with canonical builtin name if any.
        bool self_like = false;
        if (tgt_builtin) {
            StringView alias_name = t->as.alias.name;
            if (string_view_eq_sv(alias_name, string_view_from_cstr(tgt_builtin))) {
                self_like = true;
            }
        }
        if (!self_like) {
            printf(" (= ");
            print_type_inline_canonical(tgt);   // de-alias RHS
            putchar(')');
        }
        return;

    default:
        unreachable();
        return;
    }
}

static inline void dump_types_impl(const Scope* scope, u32 indent_lvl, bool as_list) {
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

    // optional function name
    if (scope->kind == SCOPE_FUNCTION && scope->ast != NULL && scope->ast->symbol != NULL) {
        indent(base);
        printf(" name: " SV_FMT "\n", SV_ARG(scope->ast->symbol->name));
    }

    // print symbol types
    {
        bool has_symbols = (scope->symbol_sets.size > 0);

        indent(base);
        printf(" symbol types:");

        if (!has_symbols) {
            puts(" []");
        }
        else {
            printf("\n");

            HashmapIterator it = hashmap_get_it(&scope->symbol_sets);
            StringView key = STRING_VIEW_EMPTY;
            SymbolSet set = { 0 };

            while (hashmap_it_next(&it, &key, &set)) {
                for (u32 i = 0; i < SYMBOL_KIND_COUNT - 1; ++i) {
                    Symbol* symbol = set.by_kind[i];
                    if (symbol == NULL) {
                        continue;
                    }

                    indent(base + 1);
                    printf("- { kind: %s, name: " SV_FMT, symbol_kind_get_name(symbol->kind), SV_ARG(symbol->name));

                    if (symbol->ast != NULL) {
                        printf(", loc: ");
                        print_loc(symbol->ast->loc);
                    }

                    if (symbol->kind != SYMBOL_IMPORT) {
                        printf(", type: ");
                        print_type_inline_alias(symbol->as.typed.type);
                    }

                    puts(" }");
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
                dump_types_impl(child, base + 1, true);
            }
        }
    }
}

void dump_types(const Scope* scope) {
    dump_types_impl(scope, 1, false);
}