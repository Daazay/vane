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

#define SCOPE_DEFAULT_SCOPES_COUNT  4
#define SCOPE_DEFAULT_SYMBOLS_COUNT 4

enum ScopeKind {
    SCOPE_UNKNOWN = 0,
    SCOPE_GLOBAL,
    SCOPE_PACKAGE,
    SCOPE_SOURCE_FILE,
    SCOPE_FUNCTION,
    SCOPE_BASIC,
};

struct Scope {
    ScopeKind kind;

    const ASTNode* ast;
    Scope* parent;

    Hashmap symbols;
    Vector scopes;
};

const char* scope_kind_get_name(ScopeKind kind);

Scope* scope_create(ScopeKind kind, Scope* parent, const ASTNode* ast);

void scope_destroy(Scope* scope);

void scope_add_symbol(Scope* scope, Symbol* symbol);

Symbol* scope_lookup_current(const Scope* scope, StringView name);

Symbol* scope_lookup(const Scope* scope, StringView name);

bool scope_resolve_types(const Scope* scope, struct TypeSystem* ts, struct ReportCollector* rc);