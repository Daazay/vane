#include "vane/ast/ast_parser.h"

#define PARSE_ERROR_LOC_AP(LOC, FMT, ...)  REPORT_ERROR_LOC((ast_parser)->ts->scanner.rc,  "parser", (LOC), FMT, ##__VA_ARGS__)
#define PARSE_INFO_LOC_AP(LOC, FMT, ...)   REPORT_INFO_LOC((ast_parser)->ts->scanner.rc,   "parser", (LOC), FMT, ##__VA_ARGS__)
#define PARSE_NOTE_LOC_AP(LOC, FMT, ...)   REPORT_NOTE_LOC((ast_parser)->ts->scanner.rc,   "parser", (LOC), FMT, ##__VA_ARGS__)
#define PARSE_DEBUG_LOC_AP(LOC, FMT, ...)  REPORT_DEBUG_LOC((ast_parser)->ts->scanner.rc,  "parser", (LOC), FMT, ##__VA_ARGS__)

#define PARSE_ERROR_LOC_TS(TS, LOC, FMT, ...) REPORT_ERROR_LOC((TS)->scanner.rc, "parser", (LOC), FMT, ##__VA_ARGS__)
#define PARSE_INFO_LOC_TS(TS, LOC, FMT, ...)  REPORT_INFO_LOC((TS)->scanner.rc,  "parser", (LOC), FMT, ##__VA_ARGS__)
#define PARSE_NOTE_LOC_TS(TS, LOC, FMT, ...)  REPORT_NOTE_LOC((TS)->scanner.rc,  "parser", (LOC), FMT, ##__VA_ARGS__)
#define PARSE_DEBUG_LOC_TS(TS, LOC, FMT, ...) REPORT_DEBUG_LOC((TS)->scanner.rc, "parser", (LOC), FMT, ##__VA_ARGS__)


static inline bool is_token_kind_beginning_of_expr(TokenKind kind) {
    return
        (kind == TOKEN_IDENTIFIER) ||
        (kind == TOKEN_L_BRACE) ||
        is_token_kind_literal(kind) ||
        is_token_kind_prefix_unop(kind);
}

static inline bool is_token_kind_beginning_of_stmt(TokenKind kind) {
    return
        (kind == TOKEN_KEYWORD_BEGIN) ||
        (kind == TOKEN_KEYWORD_TYPE) ||
        (kind == TOKEN_KEYWORD_VAR) ||
        (kind == TOKEN_KEYWORD_IF) ||
        (kind == TOKEN_KEYWORD_WHILE) ||
        (kind == TOKEN_KEYWORD_DO) ||
        (kind == TOKEN_KEYWORD_BREAK) ||
        (kind == TOKEN_KEYWORD_CONTINUE) ||
        (kind == TOKEN_KEYWORD_RETURN) ||
        (kind == TOKEN_SEMICOLON) ||
        is_token_kind_beginning_of_expr(kind);
}

static inline ASTNode* ast_node_error_create(ASTNodeKind failed, ASTNode* prev, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_ERROR, loc);
    node->as.error.failed = failed;
    node->as.error.prev = prev;
    return node;
}

static inline ASTNode* ast_node_identifier_create(String value, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_IDENTIFIER, loc);
    node->as.id.value = value;
    return node;
}

static inline ASTNode* ast_node_typeref_builtin_create(TokenKind token_kind, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_TYPEREF_BUILTIN, loc);
    node->as.typeref_builtin.kind = token_kind;
    return node;
}

static inline ASTNode* ast_node_typeref_custom_create(String value, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_TYPEREF_CUSTOM, loc);
    node->as.typeref_custom.value = value;
    return node;
}

static inline ASTNode* ast_node_typeref_ptr_create(ASTNode* typeref, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_TYPEREF_PTR, loc);
    node->as.typeref_ptr.typeref = typeref;
    return node;
}

static inline ASTNode* ast_node_typeref_arr_create(ASTNode* size_expr, ASTNode* typeref, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_TYPEREF_ARR, loc);
    node->as.typeref_arr.size_expr = size_expr;
    node->as.typeref_arr.typeref = typeref;
    return node;
}

static inline ASTNode* ast_node_typeref_fun_param_create(ASTNode* id, ASTNode* typeref, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_TYPEREF_FUN_PARAM, loc);
    node->as.typeref_fun_param.id = id;
    node->as.typeref_fun_param.typeref = typeref;
    return node;
}

static inline ASTNode* ast_node_typeref_fun_create(Vector params, ASTNode* typeref, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_TYPEREF_FUN, loc);
    node->as.typeref_fun.params = params;
    node->as.typeref_fun.typeref = typeref;
    return node;
}

static inline ASTNode* ast_node_import_decl_create(ASTNode* path, ASTNode* alias, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_IMPORT_DECL, loc);
    node->as.import_decl.path = path;
    node->as.import_decl.alias = alias;
    return node;
}

static inline ASTNode* ast_node_fun_param_create(ASTNode* id, ASTNode* typeref, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_FUN_PARAM, loc);
    node->as.fun_param.id = id;
    node->as.fun_param.typeref = typeref;
    return node;
}

static inline ASTNode* ast_node_fun_sign_create(ASTNode* id, Vector params, ASTNode* typeref, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_FUN_SIGN, loc);
    node->as.fun_sign.id = id;
    node->as.fun_sign.params = params;
    node->as.fun_sign.typeref = typeref;
    return node;
}

static inline ASTNode* ast_node_fun_decl_create(ASTNode* sign, Vector block, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_FUN_DECL, loc);
    node->as.fun_decl.sign = sign;
    node->as.fun_decl.block = block;
    return node;
}

static inline ASTNode* ast_node_stmt_empty_create(TokenLoc loc) {
    return ast_node_create(AST_NODE_STMT_EMPTY, loc);
}

static inline ASTNode* ast_node_typerefalias_decl_create(ASTNode* id, ASTNode* typeref, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_TYPEALIAS_DECL, loc);
    node->as.stmt_typealias_decl.id = id;
    node->as.stmt_typealias_decl.typeref = typeref;
    return node;
}

static inline ASTNode* ast_node_stmt_block_create(Vector block, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_BLOCK, loc);
    node->as.stmt_block.block = block;
    return node;
}

static inline ASTNode* ast_node_stmt_var_item_create(ASTNode* id, ASTNode* typeref, ASTNode* expr, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_VAR_ITEM, loc);
    node->as.stmt_var_item.id = id;
    node->as.stmt_var_item.typeref = typeref;
    node->as.stmt_var_item.expr = expr;
    return node;
}

static inline ASTNode* ast_node_stmt_var_decl_create(Vector items, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_VAR_DECL, loc);
    node->as.stmt_var_decl.items = items;
    return node;
}

static inline ASTNode* ast_node_stmt_branch_create(ASTNode* expr, Vector block, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_BRANCH, loc);
    node->as.stmt_branch.expr = expr;
    node->as.stmt_branch.block = block;
    return node;
}

static inline ASTNode* ast_node_stmt_condition_create(Vector branches, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_CONDITION, loc);
    node->as.stmt_condition.branches = branches;
    return node;
}

static inline ASTNode* ast_node_stmt_while_create(ASTNode* expr, Vector block, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_WHILE, loc);
    node->as.stmt_while.expr = expr;
    node->as.stmt_while.block = block;
    return node;
}

static inline ASTNode* ast_node_stmt_do_create(ASTNode* expr, Vector block, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_DO, loc);
    node->as.stmt_do.expr = expr;
    node->as.stmt_do.block = block;
    return node;
}

static inline ASTNode* ast_node_stmt_break_create(TokenLoc loc) {
    return ast_node_create(AST_NODE_STMT_BREAK, loc);
}

static inline ASTNode* ast_node_stmt_continue_create(TokenLoc loc) {
    return ast_node_create(AST_NODE_STMT_CONTINUE, loc);
}

static inline ASTNode* ast_node_stmt_return_create(ASTNode* expr, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_RETURN, loc);
    node->as.stmt_return.expr = expr;
    return node;
}

static inline ASTNode* ast_node_stmt_expr_create(ASTNode* expr, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_STMT_EXPR, loc);
    node->as.stmt_expr.expr = expr;
    return node;
}

static inline ASTNode* ast_node_expr_binary_create(TokenKind op, ASTNode* lhs, ASTNode* rhs, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_BINARY, loc);
    node->as.expr_binary.op = op;
    node->as.expr_binary.lhs = lhs;
    node->as.expr_binary.rhs = rhs;
    return node;
}

static inline ASTNode* ast_node_expr_prefix_unary_create(TokenKind op, ASTNode* rhs, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_PREFIX_UNARY, loc);
    node->as.expr_prefix_unary.op = op;
    node->as.expr_prefix_unary.rhs = rhs;
    return node;
}

static inline ASTNode* ast_node_expr_postfix_unary_create(TokenKind op, ASTNode* lhs, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_POSTFIX_UNARY, loc);
    node->as.expr_postfix_unary.op = op;
    node->as.expr_postfix_unary.lhs = lhs;
    return node;
}

static inline ASTNode* ast_node_expr_braces_create(ASTNode* expr, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_BRACES, loc);
    node->as.expr_braces.expr = expr;
    return node;
}

static inline ASTNode* ast_node_expr_place_create(String value, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_PLACE, loc);
    node->as.expr_place.value = value;
    return node;
}

static inline ASTNode* ast_node_expr_call_create(ASTNode* callee, Vector args, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_CALL, loc);
    node->as.expr_call.callee = callee;
    node->as.expr_call.args = args;
    return node;
}

static inline ASTNode* ast_node_expr_index_create(ASTNode* callee, ASTNode* index, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_INDEX, loc);
    node->as.expr_index.callee = callee;
    node->as.expr_index.index = index;
    return node;
}

static inline ASTNode* ast_node_expr_member_create(ASTNode* object, ASTNode* member, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_MEMBER, loc);
    node->as.expr_member.object = object;
    node->as.expr_member.member = member;
    return node;
}

static inline ASTNode* ast_node_expr_literal_create(TokenKind token_kind, String value, TokenLoc loc) {
    ASTNode* node = ast_node_create(AST_NODE_EXPR_LITERAL, loc);
    node->as.expr_literal.kind = token_kind;
    node->as.expr_literal.value = value;
    return node;
}


static inline const Token* advance_if(TokenStream* ts, TokenKind expected) {
    const Token* token = token_stream_peek_next(ts);
    if (token->kind == expected) {
        token_stream_move_forward(ts);
        return token_stream_get_curr(ts);
    }

    REPORT_ERROR_LOC(ts->scanner.rc, "parser", token->loc, "expected '%s', but got '%s'", token_kind_get_value(expected), token_kind_get_value(token->kind));
    return token;
}

static inline const Token* expect(TokenStream* ts, TokenKind expected) {
    const Token* token = token_stream_peek_next(ts);
    if (token->kind == expected) {
        return token;
    }

    REPORT_ERROR_LOC(ts->scanner.rc, "parser", token->loc, "expected '%s', but got '%s'", token_kind_get_value(expected), token_kind_get_value(token->kind));
    return token;
}

ASTParser ast_parser_create(TokenStream* ts) {
    assert(ts != NULL);

    return (ASTParser) {
        .ts = ts,
    };
}

/*...............................MISC...............................*/

ASTNode* ast_parser_parse_identifier(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_IDENTIFIER);
    if (token->kind != TOKEN_IDENTIFIER) {
        return ast_node_error_create(AST_NODE_IDENTIFIER, NULL, token->loc);
    }

    return ast_node_identifier_create(string_from_sv(token->value), token->loc);
}

/*...............................TYPEREF............................*/

ASTNode* ast_parser_parse_typeref(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = token_stream_peek_next(ast_parser->ts);
    switch (token->kind) {
    case TOKEN_IDENTIFIER: return ast_parser_parse_typeref_custom(ast_parser);
    case TOKEN_CARET:      return ast_parser_parse_typeref_ptr(ast_parser);
    case TOKEN_L_BRACKET:  return ast_parser_parse_typeref_arr(ast_parser);
    case TOKEN_L_BRACE:    return ast_parser_parse_typeref_fun(ast_parser);
    default: break;
    }

    PARSE_ERROR_LOC_AP(token->loc, "unexpected token '%s' in type reference", token_kind_get_value(token->kind));
    PARSE_NOTE_LOC_AP(token->loc, "expected identifier, '^', '[', or '(' to start a type");
    return ast_node_error_create(AST_NODE_GROUP_TYPEREF, NULL, token->loc);
}

ASTNode* ast_parser_parse_typeref_custom(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_IDENTIFIER);
    if (token->kind != TOKEN_IDENTIFIER) {
        return ast_node_error_create(AST_NODE_TYPEREF_CUSTOM, NULL, token->loc);
    }

    return ast_node_typeref_custom_create(string_from_sv(token->value), token->loc);
}

ASTNode* ast_parser_parse_typeref_ptr(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_CARET);
    if (token->kind != TOKEN_CARET) {
        return ast_node_error_create(AST_NODE_TYPEREF_PTR, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    ASTNode* typeref = ast_parser_parse_typeref(ast_parser);
    loc.end = typeref->loc.end;

    if (typeref->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_TYPEREF_PTR, typeref, loc);
    }

    return ast_node_typeref_ptr_create(typeref, loc);
}

ASTNode* ast_parser_parse_typeref_arr(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_L_BRACKET);
    if (token->kind != TOKEN_L_BRACKET) {
        return ast_node_error_create(AST_NODE_TYPEREF_ARR, NULL, token->loc);
    }

    TokenLoc loc = token->loc;
    ASTNode* size_expr = NULL;

    token = token_stream_peek_next(ast_parser->ts);
    if (token->kind != TOKEN_R_BRACKET) {
        size_expr = ast_parser_parse_expr(ast_parser);
        loc.end = size_expr->loc.end;

        if (size_expr->kind == AST_NODE_ERROR) {
            return ast_node_error_create(AST_NODE_TYPEREF_ARR, size_expr, loc);
        }
    }

    token = advance_if(ast_parser->ts, TOKEN_R_BRACKET);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_R_BRACKET) {
        ast_node_destroy(size_expr);
        return ast_node_error_create(AST_NODE_TYPEREF_ARR, NULL, loc);
    }

    ASTNode* typeref = ast_parser_parse_typeref(ast_parser);
    loc.end = typeref->loc.end;

    if (typeref->kind == AST_NODE_ERROR) {
        ast_node_destroy(size_expr);
        return ast_node_error_create(AST_NODE_TYPEREF_ARR, typeref, loc);
    }

    return ast_node_typeref_arr_create(size_expr, typeref, loc);
}

ASTNode* ast_parser_parse_typeref_fun_param(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = token_stream_peek_next(ast_parser->ts);
    TokenLoc loc = token->loc;

    ASTNode* id = NULL;
    ASTNode* typeref = NULL;

    if (token->kind == TOKEN_IDENTIFIER) {
        // Look ahead to check for colon (named param)
        token_stream_move_forward(ast_parser->ts);
        token = token_stream_peek_next(ast_parser->ts);

        if (token->kind == TOKEN_COLON) {
            token_stream_move_back(ast_parser->ts);

            id = ast_parser_parse_identifier(ast_parser);

            // skip colon token
            token_stream_move_forward(ast_parser->ts);

            typeref = ast_parser_parse_typeref(ast_parser);
            loc.end = typeref->loc.end;
        }
        else {
            token_stream_move_back(ast_parser->ts);

            typeref = ast_parser_parse_typeref(ast_parser);
            loc.end = typeref->loc.end;
        }
    }
    else {
        typeref = ast_parser_parse_typeref(ast_parser);
        loc.end = typeref->loc.end;
    }

    if (typeref->kind == AST_NODE_ERROR) {
        ast_node_destroy(id);
        return ast_node_error_create(AST_NODE_TYPEREF_FUN_PARAM, typeref, loc);
    }

    return ast_node_typeref_fun_param_create(id, typeref, loc);
}

ASTNode* ast_parser_parse_typeref_fun(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_L_BRACE);
    if (token->kind != TOKEN_L_BRACE) {
        return ast_node_error_create(AST_NODE_TYPEREF_FUN, NULL, token->loc);
    }

    TokenLoc loc = token->loc;
    Vector params = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    token = token_stream_peek_next(ast_parser->ts);
    while (!is_token_stream_end(ast_parser->ts) && token->kind != TOKEN_R_BRACE) {
        ASTNode* param = ast_parser_parse_typeref_fun_param(ast_parser);
        loc.end = param->loc.end;

        if (param->kind == AST_NODE_ERROR) {
            vector_destroy(&params);
            return ast_node_error_create(AST_NODE_TYPEREF_FUN, param, loc);
        }

        vector_push_back(&params, &param);

        token = token_stream_peek_next(ast_parser->ts);
        if (token->kind == TOKEN_COMMA) {
            token_stream_move_forward(ast_parser->ts);
            token = token_stream_peek_next(ast_parser->ts);
            continue;
        }
        break;
    }

    token = advance_if(ast_parser->ts, TOKEN_R_BRACE);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_R_BRACE) {
        vector_destroy(&params);
        return ast_node_error_create(AST_NODE_TYPEREF_FUN, NULL, loc);
    }

    token = advance_if(ast_parser->ts, TOKEN_MINUS_GREATER);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_MINUS_GREATER) {
        vector_destroy(&params);
        return ast_node_error_create(AST_NODE_TYPEREF_FUN, NULL, loc);
    }

    ASTNode* typeref = ast_parser_parse_typeref(ast_parser);
    loc.end = typeref->loc.end;

    if (typeref->kind == AST_NODE_ERROR) {
        vector_destroy(&params);
        return ast_node_error_create(AST_NODE_TYPEREF_FUN, typeref, loc);
    }

    return ast_node_typeref_fun_create(params, typeref, loc);
}

/*...............................PACKAGE............................*/

ASTNode* ast_parser_parse_package_entity(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = token_stream_peek_next(ast_parser->ts);
    switch (token->kind) {
    case TOKEN_KEYWORD_IMPORT: return ast_parser_parse_import_decl(ast_parser);
    case TOKEN_KEYWORD_TYPE:   return ast_parser_parse_stmt_typealias_decl(ast_parser);
    case TOKEN_KEYWORD_FUN:    return ast_parser_parse_fun_decl(ast_parser);
    default:
        break;
    }

    //PARSE_ERROR_LOC_AP(token->loc, "unexpected token '%s' in package entity", token_kind_get_value(token->kind));
    //PARSE_NOTE_LOC_AP(token->loc, "expected 'import', 'fun', identifier, '^', '[', or '(' to start a package entity");
    return ast_node_error_create(AST_NODE_GROUP_PACKAGE_ENTITY, NULL, token->loc);
}

ASTNode* ast_parser_parse_import_decl(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_IMPORT);
    if (token->kind != TOKEN_KEYWORD_IMPORT) {
        return ast_node_error_create(AST_NODE_IMPORT_DECL, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    token = advance_if(ast_parser->ts, TOKEN_LITERAL_STRING);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_LITERAL_STRING) {
        return ast_node_error_create(AST_NODE_IMPORT_DECL, NULL, loc);
    }

    ASTNode* path = ast_node_expr_literal_create(TOKEN_LITERAL_STRING, string_from_sv(token->value), token->loc);
    ASTNode* alias = NULL;

    token = token_stream_peek_next(ast_parser->ts);
    if (token->kind == TOKEN_KEYWORD_AS) {
        token_stream_move_forward(ast_parser->ts);

        alias = ast_parser_parse_identifier(ast_parser);
        loc.end = alias->loc.end;

        if (alias->kind == AST_NODE_ERROR) {
            ast_node_destroy(path);
            return ast_node_error_create(AST_NODE_IMPORT_DECL, alias, loc);
        }
    }

    return ast_node_import_decl_create(path, alias, loc);
}

ASTNode* ast_parser_parse_fun_param(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    ASTNode* id = ast_parser_parse_identifier(ast_parser);
    if (id->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_FUN_PARAM, id, id->loc);
    }

    TokenLoc loc = id->loc;

    ASTNode* typeref = NULL;

    const Token* token = token_stream_peek_next(ast_parser->ts);
    if (token->kind == TOKEN_COLON) {
        token_stream_move_forward(ast_parser->ts);

        typeref = ast_parser_parse_typeref(ast_parser);
        loc.end = typeref->loc.end;

        if (typeref->kind == AST_NODE_ERROR) {
            ast_node_destroy(id);
            return ast_node_error_create(AST_NODE_FUN_PARAM, typeref, loc);
        }
    }

    return ast_node_fun_param_create(id, typeref, loc);
}

ASTNode* ast_parser_parse_fun_sign(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    ASTNode* id = ast_parser_parse_identifier(ast_parser);
    if (id->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_FUN_SIGN, id, id->loc);
    }

    TokenLoc loc = id->loc;

    const Token* token = advance_if(ast_parser->ts, TOKEN_L_BRACE);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_L_BRACE) {
        ast_node_destroy(id);
        return ast_node_error_create(AST_NODE_FUN_SIGN, NULL, loc);
    }

    Vector params = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    token = token_stream_peek_next(ast_parser->ts);
    while (!is_token_stream_end(ast_parser->ts)) {
        if (params.size == 0 && token->kind == TOKEN_R_BRACE) {
            break;
        }

        ASTNode* param = ast_parser_parse_fun_param(ast_parser);
        loc.end = param->loc.end;

        if (param->kind == AST_NODE_ERROR) {
            ast_node_destroy(id);
            vector_destroy(&params);
            return ast_node_error_create(AST_NODE_FUN_SIGN, param, loc);
        }

        vector_push_back(&params, &param);

        token = token_stream_peek_next(ast_parser->ts);
        if (token->kind == TOKEN_COMMA) {
            token_stream_move_forward(ast_parser->ts);
            continue;
        }
        break;
    }

    token = advance_if(ast_parser->ts, TOKEN_R_BRACE);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_R_BRACE) {
        ast_node_destroy(id);
        vector_destroy(&params);
        return ast_node_error_create(AST_NODE_FUN_SIGN, NULL, loc);
    }

    ASTNode* typeref = NULL;

    token = token_stream_peek_next(ast_parser->ts);
    if (token->kind == TOKEN_COLON) {
        token_stream_move_forward(ast_parser->ts);

        typeref = ast_parser_parse_typeref(ast_parser);
        loc.end = typeref->loc.end;

        if (typeref->kind == AST_NODE_ERROR) {
            ast_node_destroy(id);
            vector_destroy(&params);
            return ast_node_error_create(AST_NODE_FUN_SIGN, typeref, loc);
        }
    }

    return ast_node_fun_sign_create(id, params, typeref, loc);
}

ASTNode* ast_parser_parse_fun_decl(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_FUN);
    if (token->kind != TOKEN_KEYWORD_FUN) {
        return ast_node_error_create(AST_NODE_FUN_DECL, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    ASTNode* sign = ast_parser_parse_fun_sign(ast_parser);
    loc.end = sign->loc.end;

    if (sign->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_FUN_DECL, sign, loc);
    }

    Vector block = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    token = token_stream_peek_next(ast_parser->ts);
    if (token->kind == TOKEN_EQUAL) {
        token_stream_move_forward(ast_parser->ts);

        ASTNode* expr = ast_parser_parse_expr(ast_parser);
        loc.end = expr->loc.end;

        if (expr->kind == AST_NODE_ERROR) {
            ast_node_destroy(sign);
            vector_destroy(&block);
            return ast_node_error_create(AST_NODE_FUN_DECL, expr, loc);
        }

        ASTNode* return_stmt = ast_node_stmt_return_create(expr, expr->loc);
        vector_push_back(&block, &return_stmt);
    }
    else {
        while (!is_token_stream_end(ast_parser->ts) && token->kind != TOKEN_KEYWORD_END) {
            ASTNode* stmt = ast_parser_parse_stmt(ast_parser);
            loc.end = stmt->loc.end;

            if (stmt->kind == AST_NODE_ERROR) {
                ast_node_destroy(sign);
                vector_destroy(&block);
                return ast_node_error_create(AST_NODE_FUN_DECL, stmt, loc);
            }

            vector_push_back(&block, &stmt);

            token = token_stream_peek_next(ast_parser->ts);
        }

        token = advance_if(ast_parser->ts, TOKEN_KEYWORD_END);
        loc.end = token->loc.end;

        if (token->kind != TOKEN_KEYWORD_END) {
            ast_node_destroy(sign);
            vector_destroy(&block);
            return ast_node_error_create(AST_NODE_FUN_DECL, NULL, loc);
        }
    }

    return ast_node_fun_decl_create(sign, block, loc);
}

/*...............................STMT...............................*/

ASTNode* ast_parser_parse_stmt(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = token_stream_peek_next(ast_parser->ts);

    if (is_token_kind_beginning_of_stmt(token->kind)) {
        switch (token->kind) {
        case TOKEN_KEYWORD_BEGIN:    return ast_parser_parse_stmt_block(ast_parser);
        case TOKEN_KEYWORD_TYPE:     return ast_parser_parse_stmt_typealias_decl(ast_parser);
        case TOKEN_KEYWORD_VAR:      return ast_parser_parse_stmt_var_decl(ast_parser);
        case TOKEN_KEYWORD_IF:       return ast_parser_parse_stmt_condition(ast_parser);
        case TOKEN_KEYWORD_WHILE:    return ast_parser_parse_stmt_while(ast_parser);
        case TOKEN_KEYWORD_DO:       return ast_parser_parse_stmt_do(ast_parser);
        case TOKEN_KEYWORD_BREAK:    return ast_parser_parse_stmt_break(ast_parser);
        case TOKEN_KEYWORD_CONTINUE: return ast_parser_parse_stmt_continue(ast_parser);
        case TOKEN_KEYWORD_RETURN:   return ast_parser_parse_stmt_return(ast_parser);
        case TOKEN_SEMICOLON:
            TokenLoc loc = token->loc;
            token_stream_move_forward(ast_parser->ts);
            return ast_node_stmt_empty_create(loc);
        default:
            return ast_parser_parse_stmt_expr(ast_parser);
        }
    }

    return ast_node_error_create(AST_NODE_GROUP_STMT, NULL, token->loc);
}

ASTNode* ast_parser_parse_stmt_typealias_decl(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_TYPE);
    if (token->kind != TOKEN_KEYWORD_TYPE) {
        return ast_node_error_create(AST_NODE_STMT_TYPEALIAS_DECL, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    ASTNode* id = ast_parser_parse_identifier(ast_parser);
    loc.end = id->loc.end;

    if (id->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_STMT_TYPEALIAS_DECL, id, loc);
    }

    token = advance_if(ast_parser->ts, TOKEN_EQUAL);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_EQUAL) {
        ast_node_destroy(id);
        return ast_node_error_create(AST_NODE_STMT_TYPEALIAS_DECL, NULL, loc);
    }

    ASTNode* typeref = ast_parser_parse_typeref(ast_parser);
    loc.end = typeref->loc.end;

    if (typeref->kind == AST_NODE_ERROR) {
        ast_node_destroy(id);
        return ast_node_error_create(AST_NODE_STMT_TYPEALIAS_DECL, typeref, loc);
    }

    return ast_node_typerefalias_decl_create(id, typeref, loc);
}

ASTNode* ast_parser_parse_stmt_block(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_BEGIN);
    if (token->kind != TOKEN_KEYWORD_BEGIN) {
        return ast_node_error_create(AST_NODE_STMT_BLOCK, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    Vector block = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    while (!is_token_stream_end(ast_parser->ts) && token_stream_peek_next(ast_parser->ts)->kind != TOKEN_KEYWORD_END) {
        ASTNode* stmt = ast_parser_parse_stmt(ast_parser);
        loc.end = stmt->loc.end;

        if (stmt->kind == AST_NODE_ERROR) {
            vector_destroy(&block);
            return ast_node_error_create(AST_NODE_STMT_BLOCK, stmt, loc);
        }

        vector_push_back(&block, &stmt);
    }

    token = advance_if(ast_parser->ts, TOKEN_KEYWORD_END);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_KEYWORD_END) {
        vector_destroy(&block);
        return ast_node_error_create(AST_NODE_STMT_BLOCK, NULL, loc);
    }

    return ast_node_stmt_block_create(block, loc);
}

ASTNode* ast_parser_parse_stmt_var_item(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    ASTNode* id = ast_parser_parse_identifier(ast_parser);
    if (id->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_STMT_VAR_ITEM, id, id->loc);
    }

    TokenLoc loc = id->loc;

    ASTNode* typeref = NULL;

    const Token* token = token_stream_peek_next(ast_parser->ts);

    if (token->kind == TOKEN_COLON) {
        token_stream_move_forward(ast_parser->ts);

        typeref = ast_parser_parse_typeref(ast_parser);
        loc.end = typeref->loc.end;

        if (typeref->kind == AST_NODE_ERROR) {
            ast_node_destroy(id);
            return ast_node_error_create(AST_NODE_STMT_VAR_ITEM, typeref, loc);
        }
    }

    ASTNode* expr = NULL;

    token = token_stream_peek_next(ast_parser->ts);

    if (token->kind == TOKEN_EQUAL) {
        token_stream_move_forward(ast_parser->ts);

        expr = ast_parser_parse_expr(ast_parser);
        loc.end = expr->loc.end;

        if (expr->kind == AST_NODE_ERROR) {
            ast_node_destroy(id);
            ast_node_destroy(typeref);
            return ast_node_error_create(AST_NODE_STMT_VAR_ITEM, expr, loc);
        }
    }

    return ast_node_stmt_var_item_create(id, typeref, expr, loc);
}

ASTNode* ast_parser_parse_stmt_var_decl(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_VAR);
    if (token->kind != TOKEN_KEYWORD_VAR) {
        return ast_node_error_create(AST_NODE_STMT_VAR_DECL, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    Vector items = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    do {
        ASTNode* item = ast_parser_parse_stmt_var_item(ast_parser);
        loc.end = item->loc.end;

        if (item->kind == AST_NODE_ERROR) {
            vector_destroy(&items);
            return ast_node_error_create(AST_NODE_STMT_VAR_DECL, item, loc);
        }

        vector_push_back(&items, &item);

        token = token_stream_peek_next(ast_parser->ts);
        if (token->kind == TOKEN_COMMA) {
            token_stream_move_forward(ast_parser->ts);
            continue;
        }
        break;
    } while (!is_token_stream_end(ast_parser->ts));

    return ast_node_stmt_var_decl_create(items, loc);
}

ASTNode* ast_parser_parse_stmt_branch(ASTParser* ast_parser, bool start_with_else) {
    assert(ast_parser != NULL);

    const Token* token = NULL;
    if (start_with_else) {
        token = advance_if(ast_parser->ts, TOKEN_KEYWORD_ELSE);
        if (token->kind != TOKEN_KEYWORD_ELSE) {
            return ast_node_error_create(AST_NODE_STMT_BRANCH, NULL, token->loc);
        }
    }
    else {
        token = advance_if(ast_parser->ts, TOKEN_KEYWORD_IF);
        if (token->kind != TOKEN_KEYWORD_IF) {
            return ast_node_error_create(AST_NODE_STMT_BRANCH, NULL, token->loc);
        }
    }

    TokenLoc loc = token->loc;
    bool is_else_br = false;

    if (start_with_else) {
        if (token_stream_peek_next(ast_parser->ts)->kind == TOKEN_KEYWORD_IF) {
            token_stream_move_forward(ast_parser->ts);
        }
        else {
            is_else_br = true;
        }
    }

    ASTNode* expr = NULL;
    if (!is_else_br) {
        expr = ast_parser_parse_expr(ast_parser);
        loc.end = expr->loc.end;

        if (expr->kind == AST_NODE_ERROR) {
            return ast_node_error_create(AST_NODE_STMT_BRANCH, expr, loc);
        }

        token = advance_if(ast_parser->ts, TOKEN_KEYWORD_THEN);
        loc.end = token->loc.end;

        if (token->kind != TOKEN_KEYWORD_THEN) {
            ast_node_destroy(expr);
            return ast_node_error_create(AST_NODE_STMT_BRANCH, NULL, loc);
        }
    }

    Vector block = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    while (!is_token_stream_end(ast_parser->ts)) {
        token = token_stream_peek_next(ast_parser->ts);
        if (is_else_br) {
            if (token->kind == TOKEN_KEYWORD_END) {
                break;
            }
        }
        else if ((token->kind == TOKEN_KEYWORD_ELSE) || (token->kind == TOKEN_KEYWORD_END)) {
            break;
        }

        ASTNode* stmt = ast_parser_parse_stmt(ast_parser);
        loc.end = stmt->loc.end;

        if (stmt->kind == AST_NODE_ERROR) {
            ast_node_destroy(expr);
            vector_destroy(&block);
            return ast_node_error_create(AST_NODE_STMT_BRANCH, stmt, loc);
        }

        vector_push_back(&block, &stmt);
    }

    if (is_else_br) {
        token = token_stream_peek_next(ast_parser->ts);
        loc.end = token->loc.end;

        if (token->kind != TOKEN_KEYWORD_END) {
            ast_node_destroy(expr);
            vector_destroy(&block);
            return ast_node_error_create(AST_NODE_STMT_BRANCH, NULL, loc);
        }
    }
    else {
        token = token_stream_peek_next(ast_parser->ts);
        loc.end = token->loc.end;

        if (token->kind != TOKEN_KEYWORD_ELSE && token->kind != TOKEN_KEYWORD_END) {
            ast_node_destroy(expr);
            vector_destroy(&block);
            return ast_node_error_create(AST_NODE_STMT_BRANCH, NULL, token->loc);
        }
    }

    return ast_node_stmt_branch_create(expr, block, loc);
}

ASTNode* ast_parser_parse_stmt_condition(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = expect(ast_parser->ts, TOKEN_KEYWORD_IF);
    if (token->kind != TOKEN_KEYWORD_IF) {
        return ast_node_error_create(AST_NODE_STMT_CONDITION, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    Vector branches = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    do {
        ASTNode* branch = ast_parser_parse_stmt_branch(ast_parser, branches.size > 0);
        loc.end = branch->loc.end;

        if (branch->kind == AST_NODE_ERROR) {
            vector_destroy(&branches);
            return ast_node_error_create(AST_NODE_STMT_CONDITION, branch, loc);
        }

        vector_push_back(&branches, &branch);

        // in case of else branch
        if (branch->as.stmt_branch.expr == NULL) {
            break;
        }
        token = token_stream_peek_next(ast_parser->ts);
        if (token->kind == TOKEN_KEYWORD_END) {
            break;
        }
    } while (!is_token_stream_end(ast_parser->ts));

    token = advance_if(ast_parser->ts, TOKEN_KEYWORD_END);
    if (token->kind != TOKEN_KEYWORD_END) {
        vector_destroy(&branches);
        return ast_node_error_create(AST_NODE_STMT_CONDITION, NULL, token->loc);
    }

    return ast_node_stmt_condition_create(branches, loc);
}

ASTNode* ast_parser_parse_stmt_while(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_WHILE);
    if (token->kind != TOKEN_KEYWORD_WHILE) {
        return ast_node_error_create(AST_NODE_STMT_WHILE, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    ASTNode* expr = ast_parser_parse_expr(ast_parser);
    loc.end = expr->loc.end;

    if (expr->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_STMT_WHILE, expr, loc);
    }

    token = advance_if(ast_parser->ts, TOKEN_KEYWORD_DO);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_KEYWORD_DO) {
        ast_node_destroy(expr);
        return ast_node_error_create(AST_NODE_STMT_WHILE, NULL, loc);
    }

    Vector block = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    while (!is_token_stream_end(ast_parser->ts)) {
        token = token_stream_peek_next(ast_parser->ts);
        if (token->kind == TOKEN_KEYWORD_END) {
            break;
        }

        ASTNode* stmt = ast_parser_parse_stmt(ast_parser);
        loc.end = stmt->loc.end;

        if (stmt->kind == AST_NODE_ERROR) {
            ast_node_destroy(expr);
            vector_destroy(&block);
            return ast_node_error_create(AST_NODE_STMT_WHILE, stmt, loc);
        }

        vector_push_back(&block, &stmt);
    }

    token = advance_if(ast_parser->ts, TOKEN_KEYWORD_END);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_KEYWORD_END) {
        ast_node_destroy(expr);
        vector_destroy(&block);
        return ast_node_error_create(AST_NODE_STMT_WHILE, NULL, loc);
    }

    return ast_node_stmt_while_create(expr, block, loc);
}

ASTNode* ast_parser_parse_stmt_do(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_DO);
    if (token->kind != TOKEN_KEYWORD_DO) {
        return ast_node_error_create(AST_NODE_STMT_DO, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    Vector block = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    while (!is_token_stream_end(ast_parser->ts)) {
        token = token_stream_peek_next(ast_parser->ts);
        if (token->kind == TOKEN_KEYWORD_LOOP) {
            break;
        }

        ASTNode* stmt = ast_parser_parse_stmt(ast_parser);
        loc.end = stmt->loc.end;

        if (stmt->kind == AST_NODE_ERROR) {
            vector_destroy(&block);
            return ast_node_error_create(AST_NODE_STMT_DO, stmt, loc);
        }

        vector_push_back(&block, &stmt);
    }

    token = advance_if(ast_parser->ts, TOKEN_KEYWORD_LOOP);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_KEYWORD_LOOP) {
        vector_destroy(&block);
        return ast_node_error_create(AST_NODE_STMT_DO, NULL, loc);
    }

    token = advance_if(ast_parser->ts, TOKEN_KEYWORD_WHILE);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_KEYWORD_WHILE) {
        vector_destroy(&block);
        return ast_node_error_create(AST_NODE_STMT_DO, NULL, loc);
    }

    ASTNode* expr = ast_parser_parse_expr(ast_parser);
    loc.end = expr->loc.end;

    if (expr->kind == AST_NODE_ERROR) {
        vector_destroy(&block);
        return ast_node_error_create(AST_NODE_STMT_DO, expr, loc);
    }

    return ast_node_stmt_do_create(expr, block, loc);
}

ASTNode* ast_parser_parse_stmt_break(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_BREAK);
    if (token->kind != TOKEN_KEYWORD_BREAK) {
        return ast_node_error_create(AST_NODE_STMT_BREAK, NULL, token->loc);
    }

    return ast_node_stmt_break_create(token->loc);
}

ASTNode* ast_parser_parse_stmt_continue(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_CONTINUE);
    if (token->kind != TOKEN_KEYWORD_CONTINUE) {
        return ast_node_error_create(AST_NODE_STMT_CONTINUE, NULL, token->loc);
    }

    return ast_node_stmt_continue_create(token->loc);
}

ASTNode* ast_parser_parse_stmt_return(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_KEYWORD_RETURN);
    if (token->kind != TOKEN_KEYWORD_RETURN) {
        return ast_node_error_create(AST_NODE_STMT_RETURN, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    ASTNode* expr = NULL;

    token = token_stream_peek_next(ast_parser->ts);
    if (IS_FLAG_SET(token->flags, TOKEN_FLAG_FIRST_IN_LINE) && token->kind != TOKEN_SEMICOLON) {
        expr = ast_parser_parse_expr(ast_parser);
        loc.end = expr->loc.end;

        if (expr->kind == AST_NODE_ERROR) {
            return ast_node_error_create(AST_NODE_STMT_RETURN, expr, loc);
        }
    }

    return ast_node_stmt_return_create(expr, loc);
}

ASTNode* ast_parser_parse_stmt_expr(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    ASTNode* expr = ast_parser_parse_expr(ast_parser);
    if (expr->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_STMT_EXPR, expr, expr->loc);
    }

    return ast_node_stmt_expr_create(expr, expr->loc);
}

/*...............................EXPR...............................*/

ASTNode* ast_parser_parse_expr(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    return ast_parser_parse_expr_with_prec(ast_parser, OP_PREC_NONE);
}

ASTNode* ast_parser_parse_expr_with_prec(ASTParser* ast_parser, OpPrecedence prec) {
    assert(ast_parser != NULL);

    ASTNode* node = ast_parser_parse_expr_nud(ast_parser);
    if (node->kind == AST_NODE_ERROR) {
        return node;
    }

    const Token* token = token_stream_peek_next(ast_parser->ts);
    while (!is_token_stream_end(ast_parser->ts)) {
        if (IS_FLAG_SET(token->flags, TOKEN_FLAG_FIRST_IN_LINE)) {
            break;
        }

        OpPrecedence new_prec = get_token_kind_precedence(token->kind);
        if (new_prec <= prec) {
            if (is_token_kind_beginning_of_expr(token->kind)) {
                TokenLoc loc = node->loc;
                loc.end = token->loc.end;

                ast_node_destroy(node);

                return ast_node_error_create(AST_NODE_GROUP_EXPR, NULL, loc);
            }

            break;
        }

        node = ast_parser_parse_expr_led(ast_parser, node, new_prec);
        if (node->kind == AST_NODE_ERROR) {
            return node;
        }
        token = token_stream_peek_next(ast_parser->ts);
    }

    return node;
}

ASTNode* ast_parser_parse_expr_nud(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = token_stream_peek_next(ast_parser->ts);
    switch (token->kind) {
    case TOKEN_IDENTIFIER: return ast_parser_parse_expr_place(ast_parser);
    case TOKEN_L_BRACE:    return ast_parser_parse_expr_braces(ast_parser);
    default:
        if (is_token_kind_literal(token->kind)) {
            return ast_parser_parse_expr_literal(ast_parser);
        }
        else if (is_token_kind_prefix_unop(token->kind)) {
            return ast_parser_parse_expr_prefix_unary(ast_parser);
        }
        break;
    }

    return ast_node_error_create(AST_NODE_GROUP_EXPR_NUD, NULL, token->loc);
}

ASTNode* ast_parser_parse_expr_led(ASTParser* ast_parser, ASTNode* lhs, OpPrecedence prec) {
    assert(ast_parser != NULL);

    const Token* token = token_stream_peek_next(ast_parser->ts);
    switch (token->kind) {
    case TOKEN_L_BRACE:   return ast_parser_parse_expr_call(ast_parser, lhs);
    case TOKEN_L_BRACKET: return ast_parser_parse_expr_index(ast_parser, lhs);
    case TOKEN_DOT:       return ast_parser_parse_expr_member(ast_parser, lhs);
    default:
        if (is_token_kind_postfix_unop(token->kind)) {
            return ast_parser_parse_expr_postfix_unary(ast_parser, lhs);
        }
        else if (is_token_kind_binop(token->kind)) {
            return ast_parser_parse_expr_binary(ast_parser, lhs, prec);
        }
        break;
    }
    return ast_node_error_create(AST_NODE_GROUP_EXPR_LED, NULL, token->loc);
}

ASTNode* ast_parser_parse_expr_binary(ASTParser* ast_parser, ASTNode* lhs, OpPrecedence prec) {
    assert(ast_parser != NULL && lhs != NULL);

    TokenLoc loc = lhs->loc;

    const Token* token = token_stream_peek_next(ast_parser->ts);
    loc.end = token->loc.end;

    if (!is_token_kind_binop(token->kind)) {
        ast_node_destroy(lhs);
        return ast_node_error_create(AST_NODE_EXPR_BINARY, NULL, loc);
    }

    TokenKind op = token->kind;

    token_stream_move_forward(ast_parser->ts);

    ASTNode* rhs = (get_op_associativity(prec) == OP_ASSOC_LEFT)
        ? ast_parser_parse_expr_with_prec(ast_parser, prec)
        : ast_parser_parse_expr_with_prec(ast_parser, prec - 1);

    loc.end = rhs->loc.end;

    if (rhs->kind == AST_NODE_ERROR) {
        ast_node_destroy(lhs);
        return ast_node_error_create(AST_NODE_EXPR_BINARY, rhs, loc);
    }

    return ast_node_expr_binary_create(op, lhs, rhs, loc);
}

ASTNode* ast_parser_parse_expr_prefix_unary(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = token_stream_peek_next(ast_parser->ts);
    if (!is_token_kind_prefix_unop(token->kind)) {
        return ast_node_error_create(AST_NODE_EXPR_PREFIX_UNARY, NULL, token->loc);
    }

    TokenLoc loc = token->loc;
    TokenKind op = token->kind;

    token_stream_move_forward(ast_parser->ts);

    ASTNode* rhs = ast_parser_parse_expr_with_prec(ast_parser, OP_PREC_NONE);
    loc.end = rhs->loc.end;

    if (rhs->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_EXPR_PREFIX_UNARY, rhs, token->loc);
    }

    return ast_node_expr_prefix_unary_create(op, rhs, loc);
}

ASTNode* ast_parser_parse_expr_postfix_unary(ASTParser* ast_parser, ASTNode* lhs) {
    assert(ast_parser != NULL && lhs != NULL);

    TokenLoc loc = lhs->loc;

    const Token* token = token_stream_peek_next(ast_parser->ts);
    loc.end = token->loc.end;

    if (!is_token_kind_postfix_unop(token->kind)) {
        ast_node_destroy(lhs);
        return ast_node_error_create(AST_NODE_EXPR_POSTFIX_UNARY, NULL, loc);
    }

    TokenKind op = token->kind;
    token_stream_move_forward(ast_parser->ts);

    return ast_node_expr_postfix_unary_create(op, lhs, loc);
}

ASTNode* ast_parser_parse_expr_braces(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_L_BRACE);
    if (token->kind != TOKEN_L_BRACE) {
        return ast_node_error_create(AST_NODE_EXPR_BRACES, NULL, token->loc);
    }

    TokenLoc loc = token->loc;

    ASTNode* expr = ast_parser_parse_expr_with_prec(ast_parser, OP_PREC_NONE);
    loc.end = expr->loc.end;

    if (expr->kind == AST_NODE_ERROR) {
        return ast_node_error_create(AST_NODE_EXPR_BRACES, expr, loc);
    }

    token = advance_if(ast_parser->ts, TOKEN_R_BRACE);
    loc.end = expr->loc.end;

    if (token->kind != TOKEN_R_BRACE) {
        ast_node_destroy(expr);
        return ast_node_error_create(AST_NODE_EXPR_BRACES, NULL, loc);
    }

    return ast_node_expr_braces_create(expr, loc);
}

ASTNode* ast_parser_parse_expr_place(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = advance_if(ast_parser->ts, TOKEN_IDENTIFIER);
    if (token->kind != TOKEN_IDENTIFIER) {
        return ast_node_error_create(AST_NODE_EXPR_PLACE, NULL, token->loc);
    }

    String msg = string_from_sv(token->value);

    return ast_node_expr_place_create(msg, token->loc);
}

ASTNode* ast_parser_parse_expr_call(ASTParser* ast_parser, ASTNode* callee) {
    assert(ast_parser != NULL && callee != NULL);

    TokenLoc loc = callee->loc;

    const Token* token = advance_if(ast_parser->ts, TOKEN_L_BRACE);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_L_BRACE) {
        ast_node_destroy(callee);
        return ast_node_error_create(AST_NODE_EXPR_CALL, NULL, loc);
    }

    Vector args = vector_create(4, VECTOR_SPECS(ASTNode*, &ast_node_destroy));

    token = token_stream_peek_next(ast_parser->ts);
    while (!is_token_stream_end(ast_parser->ts)) {
        if (args.size == 0 && token->kind == TOKEN_R_BRACE) {
            break;
        }

        ASTNode* arg = ast_parser_parse_expr_with_prec(ast_parser, OP_PREC_NONE);
        loc.end = arg->loc.end;

        if (arg->kind == AST_NODE_ERROR) {
            vector_destroy(&args);
            ast_node_destroy(callee);
            return ast_node_error_create(AST_NODE_EXPR_CALL, arg, loc);
        }

        vector_push_back(&args, &arg);

        token = token_stream_peek_next(ast_parser->ts);
        if (token->kind == TOKEN_COMMA) {
            token_stream_move_forward(ast_parser->ts);
            continue;
        }
        break;
    }

    token = advance_if(ast_parser->ts, TOKEN_R_BRACE);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_R_BRACE) {
        vector_destroy(&args);
        ast_node_destroy(callee);
        return ast_node_error_create(AST_NODE_EXPR_CALL, NULL, loc);
    }

    return ast_node_expr_call_create(callee, args, loc);
}

ASTNode* ast_parser_parse_expr_index(ASTParser* ast_parser, ASTNode* callee) {
    assert(ast_parser != NULL && callee != NULL);

    TokenLoc loc = callee->loc;

    const Token* token = advance_if(ast_parser->ts, TOKEN_L_BRACKET);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_L_BRACKET) {
        ast_node_destroy(callee);
        return ast_node_error_create(AST_NODE_EXPR_INDEX, NULL, loc);
    }

    ASTNode* index = ast_parser_parse_expr_with_prec(ast_parser, OP_PREC_NONE);
    loc.end = index->loc.end;

    if (index->kind == AST_NODE_ERROR) {
        ast_node_destroy(callee);
        return ast_node_error_create(AST_NODE_EXPR_INDEX, index, loc);
    }

    token = advance_if(ast_parser->ts, TOKEN_R_BRACKET);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_R_BRACKET) {
        ast_node_destroy(callee);
        ast_node_destroy(index);
        return ast_node_error_create(AST_NODE_EXPR_INDEX, NULL, loc);
    }

    return ast_node_expr_index_create(callee, index, loc);
}

ASTNode* ast_parser_parse_expr_member(ASTParser* ast_parser, ASTNode* object) {
    assert(ast_parser != NULL && object != NULL);

    TokenLoc loc = object->loc;

    const Token* token = advance_if(ast_parser->ts, TOKEN_DOT);
    loc.end = token->loc.end;

    if (token->kind != TOKEN_DOT) {
        ast_node_destroy(object);
        return ast_node_error_create(AST_NODE_EXPR_MEMBER, NULL, loc);
    }

    ASTNode* member = ast_parser_parse_expr_place(ast_parser);
    loc.end = member->loc.end;

    if (member->kind == AST_NODE_ERROR) {
        ast_node_destroy(object);
        return ast_node_error_create(AST_NODE_EXPR_MEMBER, member, loc);
    }

    return ast_node_expr_member_create(object, member, loc);
}

ASTNode* ast_parser_parse_expr_literal(ASTParser* ast_parser) {
    assert(ast_parser != NULL);

    const Token* token = token_stream_peek_next(ast_parser->ts);
    if (!is_token_kind_literal(token->kind)) {
        REPORT_ERROR_LOC(ast_parser->ts->scanner.rc, "parser", token->loc, "expected literal");
        return ast_node_error_create(AST_NODE_EXPR_LITERAL, NULL, token->loc);
    }

    TokenLoc loc = token->loc;
    TokenKind kind = token->kind;
    String value = string_from_sv(token->value);

    token_stream_move_forward(ast_parser->ts);

    return ast_node_expr_literal_create(kind, value, loc);
}