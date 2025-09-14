#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/vector.h"

struct ASTNode;
typedef enum CFGBlockKind CFGBlockKind;
typedef struct CFGBlock CFGBlock;

#define CFG_BLOCK_DEFAULT_STMT_COUNT         4
#define CFG_BLOCK_DEFAULT_SUCCESSORS_COUNT   4
#define CFG_BLOCK_DEFAULT_PREDECESSORS_COUNT 2

enum CFGBlockKind {
    CFG_BLOCK_ENTRY = 0,
    CFG_BLOCK_EXIT,
    CFG_BLOCK_BASIC,
    CFG_BLOCK_BRANCH,
    CFG_BLOCK_LOOP,
};

struct CFGBlock {
    u32                   id;
    CFGBlockKind          kind;
    const struct ASTNode* ast;

    Vector stmts;
    Vector succs;
    Vector preds;
};

const char* cfg_block_kind_get_name(CFGBlockKind kind);

CFGBlock* cfg_block_create(u32 id, CFGBlockKind kind, const struct ASTNode* ast);

void cfg_block_destroy(CFGBlock* block);

void cfg_block_add_stmt(CFGBlock* from, const struct ASTNode* ast);

void cfg_block_add_edge(CFGBlock* from, CFGBlock* to);