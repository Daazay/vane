#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string_view.h"

#include "vane/scanner/token_loc.h"

struct ASTNode;
typedef enum SymbolKind SymbolKind;
typedef struct Symbol Symbol;

enum SymbolKind {
    SYMBOL_UNKNOWN = 0,
    SYMBOL_IMPORT,
    SYMBOL_TYPEALIAS,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER,
    SYMBOL_VARIABLE,
};

struct Symbol {
    SymbolKind kind;

    StringView name;

    struct Scope* scope;
    const struct ASTNode* ast;

    union {
        struct {
            struct Package* target;
        } import;
        struct {
            struct Type* type;
        } typed;
    } as;
};

const char* symbol_kind_get_name(SymbolKind kind);

Symbol* symbol_create(SymbolKind kind, StringView name, const struct ASTNode* ast);

void symbol_destroy(Symbol* symbol);