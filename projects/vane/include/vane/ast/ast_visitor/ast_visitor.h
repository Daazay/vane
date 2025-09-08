#pragma once

#include "vane/utils/defines.h"

#include "vane/ast/ast_node.h"

typedef struct ASTVisitor ASTVisitor;

typedef void(*ASTVisitorPreFn)(ASTNode* parent, ASTNode* node, void* data);
typedef void(*ASTVisitorPostFn)(ASTNode* parent, ASTNode* node, void* data);

struct ASTVisitor {
    ASTVisitorPreFn pre_fn;
    ASTVisitorPreFn post_fn;
    void* data;
};

void ast_visit_with(ASTNode* parent, ASTNode* node, const ASTVisitor* visitor);