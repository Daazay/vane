#include "vane/sema/scope.h"

#include <stdlib.h>

typedef struct SymbolSet SymbolSet;

static inline void symbol_set_destroy(SymbolSet* set) {
    if (set == NULL) {
        return;
    }

    for (u32 i = 0; i < SYMBOL_KIND_COUNT - 1; ++i) {
        if (set->by_kind[i] != NULL) {
            symbol_destroy(set->by_kind[i]);
            set->by_kind[i] = NULL;
        }
    }
}

SymbolNamespaceMask symbol_namespace_for_symbol_kind(SymbolKind kind) {
    switch (kind) {
    case SYMBOL_VARIABLE:
    case SYMBOL_PARAMETER: return SYMBOL_NS_VALUE;
    case SYMBOL_FUNCTION:  return SYMBOL_NS_FUNC;
    case SYMBOL_TYPEALIAS: return SYMBOL_NS_TYPE;
    case SYMBOL_IMPORT:    return SYMBOL_NS_MODULE;
    default: unreachable();
    }
    return 0;
}

const char* scope_kind_get_name(ScopeKind kind) {
    switch (kind) {
    case SCOPE_GLOBAL:      return "global";
    case SCOPE_PRELUDE:     return "prelude";
    case SCOPE_PACKAGE:     return "package";
    case SCOPE_SOURCE_FILE: return "source_file";
    case SCOPE_FUNCTION:    return "function";
    case SCOPE_BASIC:       return "block";
    default:
        unreachable();
        return NULL;
    }
}

Scope* scope_create(ScopeKind kind, Scope* parent, const ASTNode* ast) {
    Scope* scope = malloc(sizeof(Scope));
    assert(scope != NULL);

    scope->kind         = kind;
    scope->parent       = parent;
    scope->ast          = ast;
    scope->symbol_sets  = (Hashmap) { 0 };
    scope->open_imports = (Vector) { 0 };
    scope->scopes       = (Vector) { 0 };

    if (parent != NULL) {
        if (parent->scopes.raw == NULL) {
            parent->scopes = vector_create(
                SCOPE_DEFAULT_SCOPES_COUNT,
                VECTOR_SPECS(Scope*, &scope_destroy)
            );
        }
        vector_push_back(&parent->scopes, &scope);
    }

    return scope;
}

void scope_destroy(Scope* scope) {
    if (scope == NULL) {
        return;
    }

    vector_destroy(&scope->scopes);
    hashmap_destroy(&scope->symbol_sets);
    vector_destroy(&scope->open_imports);

    free(scope);
}

void scope_add_symbol(Scope* scope, Symbol* symbol) {
    assert(scope != NULL && symbol != NULL);

    if (scope->symbol_sets.buckets == NULL) {
        scope->symbol_sets = hashmap_create(
            SCOPE_DEFAULT_SYMBOLS_COUNT,
            HASHMAP_KEY_SPECS(StringView, &string_view_item_hash, &string_view_item_eq, NULL),
            HASHMAP_VALUE_SPECS(SymbolSet, &symbol_set_destroy)
        );
    }

    // Get or create the SymbolSet for this name
    SymbolSet* set = hashmap_get(&scope->symbol_sets, &symbol->name);
    if (set == NULL) {
        SymbolSet fresh_set = { 0 };
        hashmap_insert(&scope->symbol_sets, &symbol->name, &fresh_set);
        set = hashmap_get(&scope->symbol_sets, &symbol->name);
        assert(set != NULL);
    }

    // Binder should have validated duplicates already; assert here for safety
    assert(symbol->kind > SYMBOL_UNKNOWN && symbol->kind < SYMBOL_KIND_COUNT && "invalid SymbolKind");
    SymbolKind k = symbol->kind - 1;
    assert(set->by_kind[k] == NULL && "duplicate symbol of same kind in this scope");

    symbol->scope = scope;
    set->by_kind[(u64)k] = symbol;
}

void scope_add_open_import(Scope* scope, Scope* imported_pkg_scope) {
    assert(scope != NULL && imported_pkg_scope != NULL);
    assert(scope->kind == SCOPE_SOURCE_FILE);

    if (scope->open_imports.raw == NULL) {
        scope->open_imports = vector_create(
            SCOPE_DEFAULT_OPEN_IMPORTS_COUNT,
            VECTOR_SPECS(Scope*, NULL)
        );
    }

    vector_push_back(&scope->open_imports, &imported_pkg_scope);
}

static inline Symbol* scope_pick_from_set_with_mask(const SymbolSet* set, SymbolNamespaceMask mask) {
    if (set == NULL) {
        return NULL;
    }

    // Deterministic priority when mask spans multiple namespaces
    const SymbolNamespaceMask lookup_order[] = {
        SYMBOL_NS_TYPE,   // prefer types
        SYMBOL_NS_FUNC,   // then functions
        SYMBOL_NS_VALUE,  // then values
        SYMBOL_NS_MODULE, // then modules/imports
    };

    for (u32 pri = 0; pri < ARR_SIZE(lookup_order); ++pri) {
        SymbolNamespaceMask want = lookup_order[pri];
        if ((want & mask) == 0) {
            continue;
        }

        for (u32 i = 0; i < SYMBOL_KIND_COUNT - 1; ++i) {
            Symbol* sym = set->by_kind[i];
            if (sym == NULL) {
                continue;
            }

            SymbolNamespaceMask ns = symbol_namespace_for_symbol_kind(sym->kind);
            if ((ns & want) != 0) {
                return sym;
            }
        }
    }

    return NULL;
}

Symbol* scope_lookup_current(const Scope* scope, StringView name, SymbolNamespaceMask mask) {
    assert(scope != NULL);

    if (is_hashmap_empty(&scope->symbol_sets)) {
        return NULL;
    }

    const SymbolSet* set = hashmap_get(&scope->symbol_sets, &name);
    if (set == NULL) {
        return NULL;
    }


    return scope_pick_from_set_with_mask(set, mask);
}

Symbol* scope_lookup(const Scope* scope, StringView name, SymbolNamespaceMask mask) {
    assert(scope != NULL);

    for (const Scope* curr = scope; curr != NULL ; curr = curr->parent) {
        Symbol* own = scope_lookup_current(curr, name, mask);
        if (own != NULL) {
            return own;
        }

        // unqualified open imports visible in this scope
        for (u32 i = 0; i < curr->open_imports.size; ++i) {
            const Scope* it = vector_at(curr->open_imports, i);
            Symbol* hit = scope_lookup_current(it, name, mask);
            if (hit != NULL) {
                return hit;
            }
        }
    }

    return NULL;
}

Symbol* scope_lookup_current_any(const Scope* scope, StringView name) {
    assert(scope != NULL);
    return scope_lookup_current(scope, name, SYMBOL_NS_VALUE | SYMBOL_NS_FUNC | SYMBOL_NS_TYPE | SYMBOL_NS_MODULE);
}

Symbol* scope_lookup_any(const Scope* scope, StringView name) {
    assert(scope != NULL);
    return scope_lookup(scope, name, SYMBOL_NS_VALUE | SYMBOL_NS_FUNC | SYMBOL_NS_TYPE | SYMBOL_NS_MODULE);
}

bool scope_resolve_types(const Scope* scope, struct TypeSystem* ts, struct ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && rc != NULL);

    bool status = true;

    HashmapIterator it = hashmap_get_it(&scope->symbol_sets);
    StringView key = STRING_VIEW_EMPTY;
    SymbolSet set = { 0 };

    while (hashmap_it_next(&it, &key, &set)) {
        for (u32 i = 1; i < SYMBOL_KIND_COUNT; ++i) {
            Symbol* sym = set.by_kind[i - 1];

            if (sym == NULL) {
                continue;
            }

            if (!symbol_resolve_type(sym, ts, rc)) {
                status = false;
            }
        }
    }

    for (u32 i = 0; i < scope->scopes.size; ++i) {
        Scope* child = vector_at(scope->scopes, i);
        if (!scope_resolve_types(child, ts, rc)) {
            status = false;
        }
    }

    return status;
}