#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/vector.h"
#include "vane/ast/ast_node.h"
#include "vane/diagnostic/report_collector.h"

typedef enum CFGBlockKind CFGBlockKind;
typedef enum CFGEdgeKind  CFGEdgeKind;

typedef struct CFGBlock    CFGBlock;
typedef struct CFGEdge     CFGEdge;

enum CFGBlockKind {
    CFG_BLOCK_UNKNOWN = 0,
    CFG_BLOCK_ENTRY,
    CFG_BLOCK_EXIT,
    CFG_BLOCK_NORMAL,
    CFG_BLOCK_LOOP_HEADER,
    CFG_BLOCK_COND_EXPR,
    CFG_BLOCK_MERGE,
};

enum CFGEdgeKind {
    CFG_EDGE_UNKNOWN = 0,
    CFG_EDGE_FALLTHROUGH,
    CFG_EDGE_TRUE,
    CFG_EDGE_FALSE,
};

struct CFGEdge {
    union {
        CFGBlock* from;
        CFGBlock* to;
    };
    CFGEdgeKind kind;
};

struct CFGBlock {
    u32 id;
    CFGBlockKind kind;

    Vector stmts;    // Vector<ASTNode*>
    Vector succ;     // Vector<CFGEdge>
    Vector pred;     // Vector<CFGEdge>
};

const char* cfg_block_kind_get_name(CFGBlockKind k);

const char* cfg_edge_kind_get_label(CFGEdgeKind k);

CFGBlock* cfg_block_create(u32 id, CFGBlockKind kind);

void cfg_block_destroy(CFGBlock* block);

void cfg_block_add_stmt(CFGBlock* block, const ASTNode* ast);

void cfg_block_add_edge(CFGBlock* from, CFGEdgeKind kind, CFGBlock* to);