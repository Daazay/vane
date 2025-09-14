#include "vane/dump/ast_dump.h"

#include <stdio.h>

#include "vane/ast/ast_visitor.h"
#include "vane/scanner/token_kind.h"

typedef struct ASTDumpTextCtx ASTDumpTextCtx;
typedef struct ASTDumpDotCtx ASTDumpDotCtx;

struct ASTDumpTextCtx {
    u32 indent;
    FileWriter* w;
};

struct ASTDumpDotCtx {
    FileWriter* w;
};

static inline void ast_simple_visitor_pre_fn(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    ASTDumpTextCtx* ctx = (ASTDumpTextCtx*)data;
    assert(ctx != NULL && ctx->w != NULL);

    for (u32 k = 0; k < ctx->indent; ++k) {
        file_writer_write_cstr(ctx->w, "  ");
    }

    file_writer_write_format(ctx->w, "%s [%d:%d-%d:%d]\n",
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
    ASTDumpDotCtx* ctx = (ASTDumpDotCtx*)data;
    assert(ctx != NULL && ctx->w != NULL);

    file_writer_write_format(ctx->w, "  n%llu [label=\"", (u64)(uptr_t)node);

    switch (node->kind) {
    case AST_NODE_IDENTIFIER:
        file_writer_write_format(ctx->w, SV_FMT, SV_ARG(node->as.id.value));
        break;
    case AST_NODE_TYPEREF_BUILTIN:
        file_writer_write_format(ctx->w, "%s", token_kind_get_value(node->as.typeref_builtin.kind));
        break;
    case AST_NODE_TYPEREF_CUSTOM:
        file_writer_write_format(ctx->w, SV_FMT, SV_ARG(node->as.typeref_custom.value));
        break;
    case AST_NODE_EXPR_LITERAL:
        if (node->as.expr_literal.kind == TOKEN_LITERAL_STRING) {
            file_writer_write_format(ctx->w, "\\\""SV_FMT"\\\"", SV_ARG(node->as.expr_literal.value));
        }
        else if (node->as.expr_literal.kind == TOKEN_LITERAL_CHAR) {
            file_writer_write_format(ctx->w, "\\\'"SV_FMT"\\\'", SV_ARG(node->as.expr_literal.value));
        }
        else {
            file_writer_write_format(ctx->w, SV_FMT, SV_ARG(node->as.expr_literal.value));
        }
        break;
    case AST_NODE_EXPR_BINARY:
        file_writer_write_format(ctx->w, "%s", token_kind_get_value(node->as.expr_binary.op));
        break;
    case AST_NODE_EXPR_PREFIX_UNARY:
        file_writer_write_format(ctx->w, "%s", token_kind_get_value(node->as.expr_prefix_unary.op));
        break;
    case AST_NODE_EXPR_POSTFIX_UNARY:
        file_writer_write_format(ctx->w, "%s", token_kind_get_value(node->as.expr_postfix_unary.op));
        break;
    case AST_NODE_EXPR_PLACE:
        file_writer_write_format(ctx->w, SV_FMT, SV_ARG(node->as.expr_place.value));
        break;
    default:
        file_writer_write_format(ctx->w, "%s", ast_node_kind_get_name(node->kind));
        break;
    }
    file_writer_write_format(ctx->w, "\"];\n");

    if (parent != NULL) {
        file_writer_write_format(ctx->w, "  n%llu -> n%llu;\n", (u64)(uptr_t)parent, (u64)(uptr_t)node);
    }
}


void dump_ast_text(FileWriter* w, const ASTNode* ast) {
    assert(ast != NULL && w != NULL && w->is_open);

    ASTDumpTextCtx ctx = { .indent = 0, .w = w, };
    ASTVisitor v = {
        .data    = &ctx,
        .pre_fn  = &ast_simple_visitor_pre_fn,
        .post_fn = &ast_simple_visitor_post_fn,
    };

    ast_visit_with(NULL, (ASTNode*)ast, &v);
}

void dump_ast_dot(FileWriter* w, const ASTNode* ast) {
    assert(ast != NULL && w->is_open);

    ASTDumpDotCtx ctx = { .w = w, };

    ASTVisitor v = {
        .data    = &ctx,
        .pre_fn  = &ast_dot_visitor_print_fn,
        .post_fn = NULL,
    };

    file_writer_write_format(w,
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
    file_writer_write_cstr(w, "}\n");
}