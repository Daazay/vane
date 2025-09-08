#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/vector.h"

#include "vane/scanner/token_loc.h"
#include "vane/scanner/token_kind.h"

#include "vane/ast/ast_node_kind.h"

typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTNodeKind kind;

    union {
#define AST_NODE(KIND, NAME, STRUCT, ...) STRUCT
#include "vane/ast/ast_node_kind.def"
    } as;

    TokenLoc loc;
};

ASTNode* ast_node_create(ASTNodeKind kind, TokenLoc loc);

void ast_node_destroy(ASTNode* node);