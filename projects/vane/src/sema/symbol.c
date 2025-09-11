#include "vane/sema/symbol.h"

#include <stdlib.h>

#include "vane/ast/ast_node.h"

#include "vane/sema/type.h"
#include "vane/sema/type_system.h"
#include "vane/sema/scope.h"

const char* symbol_kind_get_name(SymbolKind kind) {
    switch (kind) {
    case SYMBOL_IMPORT:    return "import";
    case SYMBOL_TYPEALIAS: return "type";
    case SYMBOL_FUNCTION:  return "func";
    case SYMBOL_PARAMETER: return "param";
    case SYMBOL_VARIABLE:  return "var";
    default:
        unreachable();
        return NULL;
    }
}

Symbol* symbol_create(SymbolKind kind, StringView name, struct ASTNode* ast) {
    Symbol* symbol = malloc(sizeof(Symbol));
    assert(symbol != NULL);

    symbol->kind = kind;
    symbol->name = name;
    symbol->ast = ast;

    symbol->as.import.target = NULL;
    symbol->as.typed.type = NULL;
    symbol->as.typed.type_state = 0;

    return symbol;
}

void symbol_destroy(Symbol* symbol) {
    if (symbol == NULL) {
        return;
    }

    free(symbol);
}

static inline Type* try_to_get_type_from_ast(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc);

static inline Type* try_to_get_type_from_typeref_custom(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    StringView id = string_get_view(node->as.typeref_custom.value);

    // Fast path
    Type* builtin_type = type_system_get_builtin(ts, id);
    if (builtin_type != NULL) {
        return builtin_type;
    }

    Symbol* sym = scope_lookup_any(scope, id);
    if (sym == NULL) {
        REPORT_ERROR_LOC(rc, "sema", node->loc, "unknown type '" SV_FMT "'.", SV_ARG(id));
        return NULL;
    }
    if (!symbol_resolve_type(sym, ts, rc)) {
        return NULL;
    }

    return sym->as.typed.type;
}

static inline Type* try_to_get_type_from_typeref_qualified(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    if (node->symbol == NULL) {
        REPORT_ERROR_LOC(rc, "sema", node->loc, "unresolved qualified type.");
        return type_system_get_unresolved_or_create(ts, STR_LIT("<unresolved>"));
    }

    // Ensure the symbol’s type is resolved (handles aliases)
    if (!symbol_resolve_type(node->symbol, ts, rc)) {
        return type_system_get_unresolved_or_create(ts, node->symbol->name);
    }

    return node->symbol->as.typed.type;
}

static inline Type* try_to_get_type_from_typeref_ptr(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    Type* base = try_to_get_type_from_ast(scope, node->as.typeref_ptr.typeref, ts, rc);
    return type_system_get_pointer_or_create(ts, base);
}

static inline Type* try_to_get_type_from_typeref_arr(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    Type* elem_type = try_to_get_type_from_ast(scope, node->as.typeref_arr.typeref, ts, rc);
    if (node->as.typeref_arr.size_expr != NULL) {
        return type_system_get_array_or_create(ts, elem_type, 0);
    }
    return type_system_get_slice_or_create(ts, elem_type);
}

static inline Type* try_to_get_type_from_typeref_fun(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    const Vector* ast_params = &node->as.typeref_fun.params;

    Type** param_types = NULL;
    u32 param_count = ast_params->size;

    if (param_count > 0) {
        param_types = malloc(sizeof(Type*) * param_count);
        assert(param_types != NULL);

        for (u32 i = 0; i < param_count; ++i) {
            ASTNode* ast_param = vector_at(*ast_params, i);
            ASTNode* ast_param_type = ast_param->as.fun_param.typeref;

            if (ast_param_type == NULL) {
                param_types[i] = type_system_get_builtin(ts, STR_LIT("any"));
            }
            else {
                param_types[i] = try_to_get_type_from_ast(scope, ast_param_type, ts, rc);
            }

            ast_param->symbol->as.typed.type = param_types[i];
        }
    }

    Type* rt = NULL;
    if (node->as.typeref_fun.typeref == NULL) {
        rt = type_system_get_builtin(ts, STR_LIT("any"));
    }
    else {
        rt = try_to_get_type_from_ast(scope, node->as.typeref_fun.typeref, ts, rc);
    }

    return type_system_get_fun_or_create(ts, (const Type**)param_types, param_count, rt);
}

static inline Type* try_to_get_type_from_stmt_var_item(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    // TODO:s
    return NULL;
}

static inline Type* try_to_get_type_from_ast(Scope* scope, ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(scope != NULL && ts != NULL && node != NULL && rc != NULL);

    switch (node->kind) {
    case AST_NODE_TYPEREF_CUSTOM:    return try_to_get_type_from_typeref_custom(scope, node, ts, rc);
    case AST_NODE_TYPEREF_QUALIFIED: return try_to_get_type_from_typeref_qualified(scope, node, ts, rc);
    case AST_NODE_TYPEREF_PTR:       return try_to_get_type_from_typeref_ptr(scope, node, ts, rc);
    case AST_NODE_TYPEREF_ARR:       return try_to_get_type_from_typeref_arr(scope, node, ts, rc);
    case AST_NODE_TYPEREF_FUN:       return try_to_get_type_from_typeref_fun(scope, node, ts, rc);
    case AST_NODE_STMT_VAR_ITEM:     return try_to_get_type_from_stmt_var_item(scope, node, ts, rc);
    default:
        unreachable();
        return NULL;
    }
}

static inline bool symbol_resolve_typealias_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    ASTNode* ast_type = symbol->ast->as.stmt_typealias_decl.typeref;
    StringView name = string_get_view(symbol->ast->as.stmt_typealias_decl.id->as.id.value);

    Type* target_type = try_to_get_type_from_ast(symbol->scope, ast_type, ts, rc);
    symbol->as.typed.type = type_system_get_alias_or_create(ts, name, target_type);

    return true;
}

static inline bool symbol_resolve_function_param_type(ASTNode* node, TypeSystem* ts, ReportCollector* rc) {
    assert(node != NULL && ts != NULL && rc != NULL);
    assert(node->symbol != NULL);

    Symbol* symbol = node->symbol;

    ASTNode* typeref = node->as.fun_param.typeref;

    Type* type = NULL;
    if (typeref == NULL) {
        type = type_system_get_builtin(ts, STR_LIT("any"));
    }
    else {
        type = try_to_get_type_from_ast(symbol->scope, typeref, ts, rc);
    }

    symbol->as.typed.type = type;
    return true;
}

static inline bool symbol_resolve_function_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    ASTNode* sign = symbol->ast->as.fun_decl.sign;
    Vector* params = &sign->as.fun_sign.params;

    const Type** param_types = NULL;
    u32 param_type_count = params->size;

    if (param_type_count > 0) {
        param_types = malloc(sizeof(Type*) * param_type_count);
        assert(param_types != NULL);

        for (u32 i = 0; i < param_type_count; ++i) {
            ASTNode* param = vector_at(*params, i);
            symbol_resolve_function_param_type(param, ts, rc);
            param_types[i] = param->symbol->as.typed.type;
        }
    }

    Type* ret_type = NULL;

    ASTNode* ast_ret_type = sign->as.fun_sign.typeref;
    if (ast_ret_type == NULL) {
        ret_type = type_system_get_builtin(ts, STR_LIT("any"));
    }
    else {
        ret_type = try_to_get_type_from_ast(symbol->scope, ast_ret_type, ts, rc);
    }

    symbol->as.typed.type = type_system_get_fun_or_create(ts, param_types, param_type_count, ret_type);
    return true;
}

static inline bool symbol_resolve_variable_type(Symbol* symbol, TypeSystem* ts, ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    symbol->as.typed.type = try_to_get_type_from_ast(symbol->scope, symbol->ast, ts, rc);
    return true;
}

bool symbol_resolve_type(Symbol* symbol, struct TypeSystem* ts, struct ReportCollector* rc) {
    assert(symbol != NULL && ts != NULL && rc != NULL);

    if (symbol->as.typed.type_state == TYPE_STATE_RESOLVED) {
        return true;
    }

    if (symbol->as.typed.type_state == TYPE_STATE_RESOLVING) {
        REPORT_ERROR_LOC(rc, "sema", symbol->ast ? symbol->ast->loc : (TokenLoc) { 0 },
            "cyclic type alias for '" SV_FMT "'.", SV_ARG(symbol->name)
        );
        symbol->as.typed.type = type_system_get_unresolved_or_create(ts, symbol->name);
        symbol->as.typed.type_state = TYPE_STATE_RESOLVED;
        return false;
    }

    symbol->as.typed.type_state = TYPE_STATE_RESOLVING;

    bool is_good = false;

    switch (symbol->kind) {
    case SYMBOL_IMPORT:    is_good = true; break;
    case SYMBOL_TYPEALIAS: is_good = symbol_resolve_typealias_type(symbol, ts, rc); break;
    case SYMBOL_FUNCTION:  is_good = symbol_resolve_function_type(symbol, ts, rc); break;
    case SYMBOL_PARAMETER: is_good = true; break; // resolved in function
    case SYMBOL_VARIABLE:  is_good = symbol_resolve_variable_type(symbol, ts, rc); break;
    default: {
        unreachable();
        is_good = false;
        break;
    }
    }

    if (!is_good && symbol->as.typed.type == NULL) {

        symbol->as.typed.type = type_system_get_unresolved_or_create(ts, symbol->name);
    }
    symbol->as.typed.type_state = TYPE_STATE_RESOLVED;

    return is_good;
}