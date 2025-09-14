#include "vane/cfg/cfg_block.h"

#include "vane/ast/ast_node.h"

#include <stdlib.h>

const char* cfg_block_kind_get_name(CFGBlockKind kind) {
    switch (kind) {
    case CFG_BLOCK_ENTRY:  return "entry";
    case CFG_BLOCK_EXIT:   return "exit";
    case CFG_BLOCK_BASIC:  return "basic";
    case CFG_BLOCK_BRANCH: return "branch";
    case CFG_BLOCK_LOOP:   return "loop";
    default:
        unreachable();
        return NULL;
    }
}

CFGBlock* cfg_block_create(u32 id, CFGBlockKind kind, const struct ASTNode* ast) {
    assert(ast != NULL);

    CFGBlock* block = malloc(sizeof(CFGBlock));
    assert(block != NULL);

    block->id    = id;
    block->ast   = ast;
    block->kind  = kind;
    block->stmts = (Vector) { 0 };
    block->preds = (Vector) { 0 };
    block->succs = (Vector) { 0 };

    return block;
}

void cfg_block_destroy(CFGBlock* block) {
    if (block == NULL) {
        return;
    }

    vector_destroy(&block->stmts);
    vector_destroy(&block->preds);
    vector_destroy(&block->succs);

    free(block);
}

void cfg_block_add_stmt(CFGBlock* from, const struct ASTNode* ast) {
    assert(from != NULL);
    assert(ast != NULL && (ast->kind == AST_NODE_STMT_VAR_DECL || ast->kind == AST_NODE_STMT_EXPR || ast->kind == AST_NODE_STMT_RETURN));

    if (from->stmts.raw == NULL) {
        from->stmts = vector_create(
            CFG_BLOCK_DEFAULT_STMT_COUNT,
            VECTOR_SPECS(ASTNode*, NULL)
        );
    }

    vector_push_back(&from->stmts, &ast);
}

void cfg_block_add_edge(CFGBlock* from, CFGBlock* to) {
    assert(from != NULL && to != NULL);

    if (from->succs.raw == NULL) {
        from->succs = vector_create(
            CFG_BLOCK_DEFAULT_SUCCESSORS_COUNT,
            VECTOR_SPECS(CFGBlock*, NULL)
        );
    }

    vector_push_back(&from->succs, &to);

    if (to->preds.raw == NULL) {
        to->preds = vector_create(
            CFG_BLOCK_DEFAULT_SUCCESSORS_COUNT,
            VECTOR_SPECS(CFGBlock*, NULL)
        );
    }

    vector_push_back(&to->preds, &from);
}