#include "vane/cfg/cfg_block.h"

#include <stdlib.h>

const char* cfg_block_kind_get_name(CFGBlockKind kind) {
    switch (kind) {
    case CFG_BLOCK_ENTRY:       return "entry";
    case CFG_BLOCK_EXIT:        return "exit";
    case CFG_BLOCK_NORMAL:      return "normal";
    case CFG_BLOCK_LOOP_HEADER: return "loop";
    case CFG_BLOCK_COND_EXPR:   return "cond";
    case CFG_BLOCK_MERGE:       return "merge";
    default:
        unreachable();
        return NULL;
    }
}

const char* cfg_edge_kind_get_label(CFGEdgeKind kind) {
    switch (kind) {
    case CFG_EDGE_FALLTHROUGH: return "";
    case CFG_EDGE_TRUE:        return "T";
    case CFG_EDGE_FALSE:       return "F";

    default:
        unreachable();
        return NULL;
    }
}

CFGBlock* cfg_block_create(u32 id, CFGBlockKind kind) {
    CFGBlock* block = malloc(sizeof(CFGBlock));
    assert(block != NULL);

    block->id    = id;
    block->kind  = kind;
    block->stmts = (Vector) { 0 };
    block->succ  = (Vector) { 0 };
    block->pred  = (Vector) { 0 };

    return block;
}

void cfg_block_destroy(CFGBlock* block) {
    if (block == NULL) {
        return;
    }

    vector_destroy(&block->stmts);
    vector_destroy(&block->pred);
    vector_destroy(&block->succ);

    free(block);
}

void cfg_block_add_stmt(CFGBlock* block, const ASTNode* ast) {
    assert(block != NULL && ast != NULL);

    if (block->stmts.raw == NULL) {
        block->stmts = vector_create(8, ITEM_SPECS(ASTNode*, NULL));
    }
    vector_push_back(&block->stmts, &ast);
}

void cfg_block_add_edge(CFGBlock* from, CFGEdgeKind kind, CFGBlock* to) {
    assert(from != NULL && to != NULL);

    if (from->succ.raw == NULL) {
        from->succ = vector_create(2, ITEM_SPECS(CFGEdge, NULL));
    }
    CFGEdge edge_to = { .kind = kind, .to = to, };
    vector_push_back(&from->succ, &edge_to);

    if (to->pred.raw == NULL) {
        to->pred = vector_create(2, ITEM_SPECS(CFGEdge, NULL));
    }

    CFGEdge edge_from = { .kind = kind, .from = from, };
    vector_push_back(&to->pred, &edge_from);
}