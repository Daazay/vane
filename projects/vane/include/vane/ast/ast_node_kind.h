#pragma once

#include "vane/utils/defines.h"

typedef enum ASTNodeKind ASTNodeKind;

enum ASTNodeKind {
#define AST_NODE(KIND, ...) AST_NODE_##KIND,
#include "vane/ast/ast_node_kind.def"
};

const char* ast_node_kind_get_name(ASTNodeKind kind);

bool is_ast_node_kind_a_misc(ASTNodeKind kind);

bool is_ast_node_kind_a_type(ASTNodeKind kind);

bool is_ast_node_kind_a_stmt(ASTNodeKind kind);

bool is_ast_node_kind_an_expr(ASTNodeKind kind);