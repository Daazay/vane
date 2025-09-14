#include "vane/cfg/cfg_function.h"

#include <stdlib.h>

#include "vane/ast/ast_node.h"
#include "vane/diagnostic/report_collector.h"
#include "vane/sema/scope.h"

typedef struct CFGLoopAnchor CFGLoopAnchor;
typedef struct CFGBuilder CFGBuilder;

#define CFG_BUILDER_DEFAULT_LOOP_ANCHOR_COUNT 8
#define CFG_FUNCTION_DEFAULT_BLOCKS_COUNT     4

struct CFGLoopAnchor {
    struct Scope* scope;
    CFGBlock* break_target;
    CFGBlock* continue_target;
};

struct CFGBuilder {
    Vector loop_anchors;
    CFGFunction* cfg;
    CFGBlock* curr;
    ReportCollector* rc;
    bool reachable;
};

static inline CFGBuilder cfg_builder_create(CFGFunction* cfg, ReportCollector* rc) {
    CFGBuilder builder = { 0 };

    builder.cfg = cfg;
    builder.curr = NULL;
    builder.loop_anchors = (Vector){ 0 };
    builder.reachable = true;
    builder.rc = rc;

    return builder;
}

static inline void cfg_builder_destroy(CFGBuilder* builder) {
    if (builder == NULL) {
        return;
    }

    vector_destroy(&builder->loop_anchors);
}

static inline CFGBlock* cfg_builder_create_basic(CFGBuilder* builder, const ASTNode* ast) {
    assert(builder != NULL && ast != NULL);

    CFGBlock* block = cfg_block_create(builder->cfg->blocks.size, CFG_BLOCK_BASIC, ast);
    vector_push_back(&builder->cfg->blocks, &block);
    return block;
}

static inline CFGBlock* cfg_builder_create_branch(CFGBuilder* builder, const ASTNode* ast) {
    assert(builder != NULL && ast != NULL);

    CFGBlock* block = cfg_block_create(builder->cfg->blocks.size, CFG_BLOCK_BRANCH, ast);
    vector_push_back(&builder->cfg->blocks, &block);
    return block;
}

static inline CFGBlock* cfg_builder_create_loop(CFGBuilder* builder, const ASTNode* ast) {
    assert(builder != NULL && ast != NULL);

    CFGBlock* block = cfg_block_create(builder->cfg->blocks.size, CFG_BLOCK_LOOP, ast);
    vector_push_back(&builder->cfg->blocks, &block);
    return block;
}

static inline CFGBlock* cfg_builder_ensure_basic(CFGBuilder* builder, const ASTNode* ast) {
    assert(builder != NULL);

    if (builder->curr != NULL && builder->curr->kind == CFG_BLOCK_BASIC) {
        return builder->curr;
    }

    CFGBlock* block = cfg_builder_create_basic(builder, ast);
    if (builder->reachable && builder->curr != NULL) {
        cfg_block_add_edge(builder->curr, block);
    }

    builder->curr = block;
    builder->reachable = true;

    return block;
}

static inline void cfg_builder_connect_to(CFGBuilder* builder, CFGBlock* target) {
    assert(builder != NULL && target != NULL);

    if (builder->reachable && builder->curr != NULL) {
        cfg_block_add_edge(builder->curr, target);
    }
}

static inline void cfg_builder_warn_unreachable_if_needed(CFGBuilder* builder, const ASTNode* anchor) {
    assert(builder != NULL && anchor != NULL);

    if (!builder->reachable) {
        REPORT_WARNING_LOC(builder->rc, DIAG_SEMA_CFG, anchor->loc, "unreachable code: control does not flow here.");
    }
}

static inline void cfg_builder_push_loop(CFGBuilder* builder, Scope* scope, CFGBlock* break_target, CFGBlock* continue_target) {
    assert(builder != NULL && break_target != NULL && continue_target != NULL);
    assert(scope != NULL && scope->kind == SCOPE_LOOP);

    if (builder->loop_anchors.raw == NULL) {
        builder->loop_anchors = vector_create(
            CFG_BUILDER_DEFAULT_LOOP_ANCHOR_COUNT,
            VECTOR_SPECS(CFGLoopAnchor, NULL)
        );
    }

    CFGLoopAnchor loop_anchor = { .scope = scope, .break_target = break_target, .continue_target = continue_target, };
    vector_push_back(&builder->loop_anchors, &loop_anchor);
}

static inline void cfg_builder_pop_loop(CFGBuilder* builder, Scope* scope) {
    assert(builder != NULL);
    assert(scope != NULL && scope->kind == SCOPE_LOOP);

    for (u32 i = 0; i < builder->loop_anchors.size; ++i) {
        CFGLoopAnchor* loop_anchor = vector_at(builder->loop_anchors, i);

        if (loop_anchor->scope == scope) {
            vector_remove(&builder->loop_anchors, i);
        }
    }
}

static inline CFGLoopAnchor* cfg_builder_find_loop(CFGBuilder* builder, Scope* scope) {
    assert(builder != NULL);
    assert(scope != NULL && scope->kind == SCOPE_LOOP);

    for (u32 i = 0; i < builder->loop_anchors.size; ++i) {
        CFGLoopAnchor* loop_anchor = vector_at(builder->loop_anchors, i);

        if (loop_anchor->scope == scope) {
            return loop_anchor;
        }
    }

    return NULL;
}

static inline void cfg_builder_build_stmt_list(CFGBuilder* builder, const Vector* list);

static inline bool cfg_builder_build_one_branch(CFGBuilder* builder, ASTNode* ast, bool is_first, CFGBlock* merge, CFGBlock** incoming_false) {
    assert(builder != NULL && merge != NULL);
    assert(ast != NULL && ast->kind == AST_NODE_STMT_BRANCH);

    ASTNode* expr = ast->as.stmt_branch.expr;
    Vector* body  = &ast->as.stmt_branch.block;

    if (expr != NULL) {
        // if / else-if
        CFGBlock* branch = cfg_builder_create_branch(builder, ast);

        if (is_first) {
            cfg_builder_connect_to(builder, branch);
        }
        else if (incoming_false != NULL && *incoming_false != NULL) {
            cfg_builder_connect_to(builder, *incoming_false);
        }

        // true -> then
        const ASTNode* then_anchor = (body->size > 0) ? vector_at_front(*body) : ast;
        CFGBlock* then_block = cfg_builder_create_basic(builder, then_anchor);
        cfg_block_add_edge(branch, then_block);

        // build then-body
        CFGBlock* saved_curr = builder->curr;

        builder->curr = then_block;
        builder->reachable = true;
        cfg_builder_build_stmt_list(builder, body);

        if (builder->reachable && builder->curr != NULL) {
            cfg_block_add_edge(builder->curr, merge);
        }

        // false flows to next test
        if (incoming_false != NULL) {
            *incoming_false = branch;
        }

        // no linear fall-through from original context across a test
        builder->curr = saved_curr;
        builder->reachable = false;

        return true;
    }

    // else - branch
    const ASTNode* else_anchor = (body->size > 0) ? vector_at_front(*body) : ast;
    CFGBlock* else_block = cfg_builder_create_basic(builder, else_anchor);

    if (incoming_false != NULL && *incoming_false != NULL) {
        cfg_block_add_edge(*incoming_false, else_block);
        *incoming_false = NULL;
    }
    else {
        cfg_builder_connect_to(builder, else_block);
    }

    CFGBlock* saved_curr = builder->curr;

    builder->curr = else_block;
    builder->reachable = true;
    cfg_builder_build_stmt_list(builder, body);

    if (builder->reachable && builder->curr != NULL) {
        cfg_block_add_edge(builder->curr, merge);
    }

    builder->curr = saved_curr;
    builder->reachable = false;

    return true;
}

static inline void cfg_builder_build_condition(CFGBuilder* builder, const ASTNode* ast) {
    assert(builder != NULL);
    assert(ast != NULL && ast->kind == AST_NODE_STMT_CONDITION);

    const Vector* branches = &ast->as.stmt_condition.branches;

    if (branches->size == 0) {
        REPORT_NOTE_LOC(builder->rc, "cfg", ast->loc, "condition has no branches, nothing to emit.");
        return;
    }

    cfg_builder_warn_unreachable_if_needed(builder, ast);

    CFGBlock* merge = cfg_builder_create_basic(builder, ast);
    CFGBlock* incoming_false = NULL;

    for (u32 i = 0; i < branches->size; ++i) {
        ASTNode* br = vector_at(*branches, i);
        cfg_builder_build_one_branch(builder, br, (i == 0), merge, &incoming_false);
    }

    if (incoming_false != NULL) {
        cfg_block_add_edge(incoming_false, merge);
    }

    builder->curr = merge;
    builder->reachable = true;
}

static inline void cfg_builder_build_while(CFGBuilder* builder, const ASTNode* ast) {
    assert(builder != NULL);
    assert(ast != NULL && ast->kind == AST_NODE_STMT_WHILE);

    cfg_builder_warn_unreachable_if_needed(builder, ast);

    CFGBlock* header = cfg_builder_create_loop(builder, ast);

    const ASTNode* body_anchor = (ast->as.stmt_while.block.size > 0)
        ? vector_at_back(ast->as.stmt_while.block)
        : ast;

    CFGBlock* body = cfg_builder_create_basic(builder, body_anchor);
    CFGBlock* exit = cfg_builder_create_basic(builder, ast);

    // entry to header
    cfg_builder_connect_to(builder, header);

    // header branches
    cfg_block_add_edge(header, body); // true
    cfg_block_add_edge(header, exit); // false

    // loop anchors
    cfg_builder_push_loop(builder, ast->scope, exit, header);

    builder->curr = body;
    builder->reachable = true;

    cfg_builder_build_stmt_list(builder, &ast->as.stmt_while.block);

    // backedge if body falls through
    if (builder->reachable && builder->curr != NULL) {
        cfg_block_add_edge(builder->curr, header);
    }

    cfg_builder_pop_loop(builder, ast->scope);

    // continue after loop
    builder->curr = exit;
    builder->reachable = true;
}

static inline void cfg_builder_build_do(CFGBuilder* builder, const ASTNode* ast) {
    assert(builder != NULL);
    assert(ast != NULL && ast->kind == AST_NODE_STMT_DO);

    cfg_builder_warn_unreachable_if_needed(builder, ast);

    const ASTNode* body_anchor = (ast->as.stmt_do.block.size > 0)
        ? vector_at_back(ast->as.stmt_do.block)
        : ast;

    CFGBlock* body   = cfg_builder_create_basic(builder, body_anchor);
    CFGBlock* header = cfg_builder_create_loop(builder, ast);
    CFGBlock* exit   = cfg_builder_create_basic(builder, ast);

    // entry to body
    cfg_builder_connect_to(builder, body);

    // loop anchors
    cfg_builder_push_loop(builder, ast->scope, exit, header);

    builder->curr = body;
    builder->reachable = true;

    cfg_builder_build_stmt_list(builder, &ast->as.stmt_do.block);

    if (builder->reachable && builder->curr != NULL) {
        cfg_block_add_edge(builder->curr, header);
    }

    // header branches
    cfg_block_add_edge(header, body); // true
    cfg_block_add_edge(header, exit); // false

    cfg_builder_pop_loop(builder, ast->scope);

    builder->curr = exit;
    builder->reachable = true;
}

static inline void cfg_builder_prepare_stmt(CFGBuilder* builder, const ASTNode* ast) {
    assert(builder != NULL);

    switch (ast->kind) {
    case AST_NODE_STMT_VAR_DECL:
    case AST_NODE_STMT_EXPR: {
        CFGBlock* block = cfg_builder_ensure_basic(builder, ast);
        cfg_block_add_stmt(block, ast);
    } break;

    case AST_NODE_STMT_RETURN: {
        CFGBlock* block = cfg_builder_ensure_basic(builder, ast);
        cfg_block_add_stmt(block, ast);
        cfg_block_add_edge(block, builder->cfg->exit);
        builder->reachable = false;
    } break;

    case AST_NODE_STMT_BREAK: {
        CFGLoopAnchor* anchor = cfg_builder_find_loop(builder, ast->scope);
        if (anchor == NULL) {
            REPORT_ERROR_LOC(builder->rc, DIAG_SEMA_CFG, ast->loc, "'break' used outside of a loop.");
            builder->reachable = false;
            break;
        }

        CFGBlock* block = cfg_builder_ensure_basic(builder, ast);
        cfg_block_add_stmt(block, ast);
        cfg_block_add_edge(block, anchor->break_target);
        builder->reachable = false;
    } break;

    case AST_NODE_STMT_CONTINUE: {
        CFGLoopAnchor* anchor = cfg_builder_find_loop(builder, ast->scope);
        if (anchor == NULL) {
            REPORT_ERROR_LOC(builder->rc, DIAG_SEMA_CFG, ast->loc, "'continue' used outside of a loop.");
            builder->reachable = false;
            break;
        }

        CFGBlock* block = cfg_builder_ensure_basic(builder, ast);
        cfg_block_add_stmt(block, ast);
        cfg_block_add_edge(block, anchor->continue_target);
        builder->reachable = false;
    } break;

    case AST_NODE_STMT_BLOCK: {
        cfg_builder_build_stmt_list(builder, &ast->as.stmt_block.block);
    } break;

    case AST_NODE_STMT_CONDITION: {
        cfg_builder_build_condition(builder, ast);
    } break;

    case AST_NODE_STMT_WHILE: {
        cfg_builder_build_while(builder, ast);
    } break;

    case AST_NODE_STMT_DO: {
        cfg_builder_build_do(builder, ast);
    } break;

    default: {
        unreachable();
        break;
    }
    }
}

static inline void cfg_builder_build_stmt_list(CFGBuilder* builder, const Vector* list) {
    assert(builder != NULL && list != NULL);

    for (u32 i = 0; i < list->size; ++i) {
        ASTNode* ast = vector_at(*list, i);

        if (!builder->reachable) {
            cfg_builder_warn_unreachable_if_needed(builder, ast);
        }

        cfg_builder_prepare_stmt(builder, ast);
    }
}

CFGFunction* cfg_build_function(const struct ASTNode* ast, struct ReportCollector* rc) {
    assert(rc != NULL && ast != NULL && ast->kind == AST_NODE_FUN_DECL);

    CFGFunction* cfg = malloc(sizeof(CFGFunction));
    assert(cfg != NULL);

    cfg->ast = ast;

    cfg->ast    = ast;
    cfg->blocks = vector_create(
        CFG_FUNCTION_DEFAULT_BLOCKS_COUNT,
        VECTOR_SPECS(CFGBlock*, &cfg_block_destroy)
    );
    cfg->entry  = NULL;
    cfg->exit   = NULL;

    CFGBuilder builder = cfg_builder_create(cfg, rc);

    cfg->entry = cfg_builder_create_basic(&builder, ast);
    cfg->exit  = cfg_builder_create_basic(&builder, ast);

    builder.curr = cfg->entry;
    builder.reachable = true;

    const Vector* body = &ast->as.fun_decl.block;
    cfg_builder_build_stmt_list(&builder, body);

    if (builder.reachable && builder.curr != NULL) {
        cfg_block_add_edge(builder.curr, cfg->exit);
    }

    cfg_builder_destroy(&builder);

    return cfg;
}

void cfg_function_destroy(CFGFunction* cfg) {
    if (cfg == NULL) {
        return;
    }

    vector_destroy(&cfg->blocks);
    free(cfg);
}