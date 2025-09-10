#include "vane/sema/scope.h"

#include <stdlib.h>

const char* scope_kind_get_name(ScopeKind kind) {
    switch (kind) {
    case SCOPE_GLOBAL:      return "global";
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

    scope->kind = kind;

    scope->parent = parent;
    scope->ast = ast;

    scope->scopes = (Vector) { 0 };
    scope->symbols = (Hashmap) { 0 };

    if (parent != NULL) {
        if (parent->scopes.raw == NULL) {
            parent->scopes = vector_create(SCOPE_DEFAULT_SCOPES_COUNT, VECTOR_SPECS(Scope*, &scope_destroy));
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
    hashmap_destroy(&scope->symbols);

    free(scope);
}

void scope_add_symbol(Scope* scope, Symbol* symbol) {
    assert(scope != NULL && symbol != NULL);

    if (scope->symbols.buckets == NULL) {
        scope->symbols = hashmap_create(SCOPE_DEFAULT_SYMBOLS_COUNT,
            HASHMAP_KEY_SPECS(StringView, &string_view_item_hash, &string_view_item_eq, NULL),
            HASHMAP_VALUE_SPECS(Symbol*, &symbol_destroy)
        );
    }

    symbol->scope = scope;
    hashmap_insert(&scope->symbols, &symbol->name, &symbol);
}

Symbol* scope_lookup_current(const Scope* scope, StringView name) {
    assert(scope != NULL);

    if (is_hashmap_empty(&scope->symbols)) {
        return NULL;
    }

    return hashmap_get(&scope->symbols, &name);
}

Symbol* scope_lookup(const Scope* scope, StringView name) {
    assert(scope != NULL);

    Symbol* symbol = NULL;

    const Scope* it = scope;
    while (it != NULL) {
        symbol = scope_lookup_current(it, name);

        if (symbol != NULL) {
            return symbol;
        }

        it = it->parent;
    }

    return NULL;
}

bool scope_resolve_types(const Scope* scope, struct TypeSystem* ts, struct ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && rc != NULL);

    bool status = true;

    HashmapIterator it = hashmap_get_it(&scope->symbols);
    StringView key = STRING_VIEW_EMPTY;
    Symbol* sym = NULL;

    while (hashmap_it_next(&it, &key, &sym)) {
        if (sym == NULL) {
            continue;
        }
        if (!symbol_resolve_type(sym, ts, rc)) {
            status = false;
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