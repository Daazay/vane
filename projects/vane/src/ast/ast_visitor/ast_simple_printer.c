#include "vane/ast/ast_visitor/ast_simple_printer.h"

#include <stdio.h>

typedef struct ASTSimpleVisitor ASTSimpleVisitor;

struct ASTSimpleVisitor {
    u32 indent;
    void* out;
};

void ast_simple_visitor_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    ASTSimpleVisitor* ctx = (ASTSimpleVisitor*)data;
    for (u32 k = 0; k < ctx->indent; ++k) {
        printf("  ");
    }

    printf("%s [%d:%d-%d:%d]\n",
        ast_node_kind_get_name(node->kind),
        node->loc.begin.line, node->loc.begin.column,
        node->loc.end.line, node->loc.end.column
    );

    ctx->indent++;
}

void ast_simple_visitor_post_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;
    (void)node;

    ASTSimpleVisitor* ctx = (ASTSimpleVisitor*)data;
    if (ctx->indent > 0) {
        ctx->indent--;
    }
}

void ast_print_simple(const ASTNode* node, void* stream) {
    assert(node != NULL && stream != NULL);

    ASTSimpleVisitor ctx = { .out = stream, };
    ASTVisitor v = {
        .data = &ctx,
        .pre_fn = &ast_simple_visitor_pre_fn,
        .post_fn = &ast_simple_visitor_post_fn,
    };

    ast_visit_with(NULL,  (ASTNode*)node, &v);
}