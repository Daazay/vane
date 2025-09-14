#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/vector.h"
#include "vane/utils/hashmap.h"

#include "vane/diagnostic/report_collector.h"

#include "vane/ast/ast_node.h"

#include "vane/sema/symbol.h"

struct TypeSystem;
struct ReportCollector;

typedef struct Scope Scope;
typedef enum ScopeKind ScopeKind;
typedef enum SymbolNamespace SymbolNamespace;
typedef u32 SymbolNamespaceMask;

#define SCOPE_DEFAULT_SCOPES_COUNT       2
#define SCOPE_DEFAULT_SYMBOLS_COUNT      8
#define SCOPE_DEFAULT_OPEN_IMPORTS_COUNT 2

enum ScopeKind {
    SCOPE_UNKNOWN = 0,
    SCOPE_GLOBAL,
    SCOPE_PRELUDE,
    SCOPE_PACKAGE,
    SCOPE_SOURCE_FILE,
    SCOPE_FUNCTION,
    SCOPE_BASIC,
    SCOPE_BRANCH,
    SCOPE_LOOP,
};


enum SymbolNamespace {
    SYMBOL_NS_UNKNOWN = 0,
    SYMBOL_NS_VALUE   = 1 << 0,
    SYMBOL_NS_FUNC    = 1 << 1,
    SYMBOL_NS_TYPE    = 1 << 2,
    SYMBOL_NS_MODULE  = 1 << 3,
};

struct Scope {
    ScopeKind kind;
    Scope* parent;
    const ASTNode* ast;

    // k: [StringView, NULL]
    // v: [Vector, &vector_destroy]
    Hashmap symbol_sets;
    Vector open_imports;

    Vector scopes;
};

SymbolNamespaceMask symbol_namespace_for_symbol_kind(SymbolKind kind);

const char* scope_kind_get_name(ScopeKind kind);

Scope* scope_create(ScopeKind kind, Scope* parent, const ASTNode* ast);

void scope_destroy(Scope* scope);

void scope_add_symbol(Scope* scope, Symbol* symbol);

void scope_add_open_import(Scope* scope, Scope* imported_pkg_scope);

Symbol* scope_lookup_current(const Scope* scope, StringView name, SymbolNamespaceMask mask);

Symbol* scope_lookup(const Scope* scope, StringView name, SymbolNamespaceMask mask);

Symbol* scope_lookup_current_any(const Scope* scope, StringView name);

Symbol* scope_lookup_any(const Scope* scope, StringView name);

bool scope_resolve_types(const Scope* scope, struct TypeSystem* ts, struct ReportCollector* rc);