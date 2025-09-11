#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string_view.h"

#include "vane/scanner/token_loc.h"

#include "vane/sema/type.h"

struct ReportCollector;
struct ASTNode;

typedef enum SymbolKind SymbolKind;
typedef struct Symbol Symbol;
typedef struct SymbolSet SymbolSet;

enum SymbolKind {
    SYMBOL_UNKNOWN = 0,

    SYMBOL_IMPORT,
    SYMBOL_TYPEALIAS,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER,
    SYMBOL_VARIABLE,

    SYMBOL_KIND_COUNT,
};

struct SymbolSet {
    Symbol* by_kind[SYMBOL_KIND_COUNT - 1];
};

struct Symbol {
    SymbolKind kind;

    StringView name;

    struct Scope* scope;
    struct ASTNode* ast;

    union {
        struct {
            struct Package* target;
        } import;
        struct {
            Type* type;
            TypeResolveState type_state;
        } typed;
    } as;
};

const char* symbol_kind_get_name(SymbolKind kind);

Symbol* symbol_create(SymbolKind kind, StringView name, struct ASTNode* ast);

void symbol_destroy(Symbol* symbol);

bool symbol_resolve_type(Symbol* symbol, struct TypeSystem* ts, struct ReportCollector* rc);