#include "vane/dump/ast_dump.h"

#include <stdio.h>

#include "vane/ast/ast_visitor.h"

typedef struct ASTDumpTextCtx ASTDumpTextCtx;

struct ASTDumpTextCtx {
    u32 indent;
};

static inline void ast_simple_visitor_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    ASTDumpTextCtx* ctx = (ASTDumpTextCtx*)data;
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

static inline void ast_simple_visitor_post_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;
    (void)node;

    ASTDumpTextCtx* ctx = (ASTDumpTextCtx*)data;
    if (ctx->indent > 0) {
        ctx->indent--;
    }
}

static inline void ast_dot_visitor_print_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)data;

    printf("  n%llu [label=\"", (u64) * ((const u64*)node));

    switch (node->kind) {
    case AST_NODE_IDENTIFIER:
        printf(SV_FMT, SV_ARG(node->as.id.value));
        break;
    case AST_NODE_TYPEREF_BUILTIN:
        printf("%s", token_kind_get_value(node->as.typeref_builtin.kind));
        break;
    case AST_NODE_TYPEREF_CUSTOM:
        printf(SV_FMT, SV_ARG(node->as.typeref_custom.value));
        break;
    case AST_NODE_EXPR_LITERAL:
        if (node->as.expr_literal.kind == TOKEN_LITERAL_STRING) {
            printf("\\\""SV_FMT"\\\"", SV_ARG(node->as.expr_literal.value));
        }
        else if (node->as.expr_literal.kind == TOKEN_LITERAL_CHAR) {
            printf("\\\'"SV_FMT"\\\'", SV_ARG(node->as.expr_literal.value));
        }
        else {
            printf(SV_FMT, SV_ARG(node->as.expr_literal.value));
        }
        break;
    case AST_NODE_EXPR_BINARY:
        printf("%s", token_kind_get_value(node->as.expr_binary.op));
        break;
    case AST_NODE_EXPR_PREFIX_UNARY:
        printf("%s", token_kind_get_value(node->as.expr_prefix_unary.op));
        break;
    case AST_NODE_EXPR_POSTFIX_UNARY:
        printf("%s", token_kind_get_value(node->as.expr_postfix_unary.op));
        break;
    case AST_NODE_EXPR_PLACE:
        printf(SV_FMT, SV_ARG(node->as.expr_place.value));
        break;
    default:
        printf("%s", ast_node_kind_get_name(node->kind));
        break;
    }
    printf("\"];\n");

    if (parent != NULL) {
        printf("  n%llu -> n%llu;\n", (u64) * ((const u64*)parent), (u64) * ((const u64*)node));
    }
}


void dump_ast_text(const ASTNode* ast) {
    assert(ast != NULL);

    ASTDumpTextCtx ctx = { .indent = 0, };
    ASTVisitor v = {
        .data    = &ctx,
        .pre_fn  = &ast_simple_visitor_pre_fn,
        .post_fn = &ast_simple_visitor_post_fn,
    };

    ast_visit_with(NULL, (ASTNode*)ast, &v);
}

void dump_ast_dot(const ASTNode* ast) {
    assert(ast != NULL);

    ASTVisitor v = {
        .data    = NULL,
        .pre_fn  = &ast_dot_visitor_print_fn,
        .post_fn = NULL,
    };

    printf(
        "digraph {\n"
        "  ranksep = 0.35;\n"
        "  node [\n"
        "    shape = \"record\",\n"
        "    style = \"solid, filled\",\n"
        "    fontcolor = \"dark\",\n"
        "    fontsize = 12,\n"
        "    width = 0.5,\n"
        "    height = 0.25\n"
        "  ];\n\n"
        "  edge [\n"
        "    arrowsize = 0.6,\n"
        "    color = \"black\",\n"
        "    style = \"light\"\n"
        "  ];\n\n"
    );

    ast_visit_with(NULL, (ASTNode*)ast, &v);
    printf("}\n");
}