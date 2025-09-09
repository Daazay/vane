#include "vane/sema/symbol.h"

#include <stdlib.h>

const char* symbol_kind_get_name(SymbolKind kind) {
    switch (kind) {
    case SYMBOL_IMPORT:    return "import";
    case SYMBOL_TYPEALIAS: return "typealias";
    case SYMBOL_FUNCTION:  return "function";
    case SYMBOL_PARAMETER: return "param";
    case SYMBOL_VARIABLE:  return "var";
    default:
        unreachable();
        return NULL;
    }
}

Symbol* symbol_create(SymbolKind kind, StringView name, const struct ASTNode* ast) {
    Symbol* symbol = malloc(sizeof(Symbol));
    assert(symbol != NULL);

    symbol->kind = kind;
    symbol->name = name;
    symbol->as.typed.type = NULL;
    symbol->ast = ast;

    return symbol;
}

void symbol_destroy(Symbol* symbol) {
    if (symbol == NULL) {
        return;
    }

    free(symbol);
}