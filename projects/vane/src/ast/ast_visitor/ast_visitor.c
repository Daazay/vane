#include "vane/ast/ast_visitor/ast_visitor.h"

void ast_visit_with(ASTNode* parent, ASTNode* node, const ASTVisitor* visitor) {
    if (node == NULL) {
        return;
    }

    if (visitor->pre_fn != NULL) {
        visitor->pre_fn(parent, node, visitor->data);
    }

    switch (node->kind) {
#define AST_NODE_VISIT_CHILD_FN(CHILD, ...) ast_visit_with(node, CHILD, visitor);
#define AST_NODE(KIND, NAME, STRUCT, DESTROY, VISIT) \
case AST_NODE_##KIND: { \
    VISIT; \
    break; \
}
#include "vane/ast/ast_node_kind.def"
    default:
        unreachable();
        break;
    }

    if (visitor->post_fn != NULL) {
        visitor->post_fn(parent, node, visitor->data);
    }
}