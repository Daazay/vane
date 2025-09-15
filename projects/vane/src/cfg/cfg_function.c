#include "vane/cfg/cfg_function.h"

#include <stdlib.h>

typedef struct CFGBuildCtx CFGBuildCtx;
typedef struct LoopTargets LoopTargets;

struct LoopTargets {
    CFGBlock* break_target;
    CFGBlock* continue_target;
};

struct CFGBuildCtx {
    ReportCollector* rc;
    CFGFunction*     cfg;
    CFGBlock*        curr;
    Vector           loop_targets;
};

static inline CFGBlock* cfg_builder_create_block(CFGBuildCtx* ctx, CFGBlockKind kind) {
    assert(ctx != NULL && ctx->cfg != NULL);

    CFGBlock* block = cfg_block_create(ctx->cfg->blocks.size, kind);

    if (ctx->cfg->blocks.raw == NULL) {
        ctx->cfg->blocks = vector_create(4, VECTOR_SPECS(CFGBlock*, &cfg_block_destroy));
    }
    vector_push_back(&ctx->cfg->blocks, &block);
    return block;
}

static inline bool cfg_builder_is_terminator_stmt(const ASTNode* ast) {
    assert(ast != NULL);
    return ast->kind == AST_NODE_STMT_RETURN ||
           ast->kind == AST_NODE_STMT_BREAK  ||
           ast->kind == AST_NODE_STMT_CONTINUE;
}

static inline CFGBlock* cfg_builder_ensure_curr(CFGBuildCtx* ctx) {
    assert(ctx != NULL);

    // dead/unreachable new block; not connected unless caller connects
    if (ctx->curr == NULL) {
        ctx->curr = cfg_builder_create_block(ctx, CFG_BLOCK_NORMAL);
    }
    return ctx->curr;
}

static inline void cfg_builder_push_loop_targets(CFGBuildCtx* ctx, CFGBlock* break_target, CFGBlock* continue_target) {
    assert(ctx != NULL);

    if (ctx->loop_targets.raw == NULL) {
        ctx->loop_targets = vector_create(4, VECTOR_SPECS(LoopTargets, NULL));
    }
    LoopTargets lt = { .break_target = break_target, .continue_target = continue_target, };
    vector_push_back(&ctx->loop_targets, &lt);
}

static inline void cfg_builder_pop_loop_targets(CFGBuildCtx* ctx) {
    assert(ctx != NULL);
    assert(ctx->loop_targets.size > 0);
    vector_pop_back(&ctx->loop_targets);
}

static inline LoopTargets* cfg_builder_get_last_loop_targets(CFGBuildCtx* ctx) {
    assert(ctx != NULL);
    if (ctx->loop_targets.size == 0) {
        return NULL;
    }
    return vector_at_back(ctx->loop_targets);
}

static inline void cfg_builder_build_stmt_list(CFGBuildCtx* ctx, const Vector* list);

static inline void cfg_builder_build_condition(CFGBuildCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL && ast->kind == AST_NODE_STMT_CONDITION);

    const Vector* branches = &ast->as.stmt_condition.branches;
    assert(branches->size > 0 && "empty condition: no branches provided");

    CFGBlock* merge = cfg_builder_create_block(ctx, CFG_BLOCK_MERGE);
    // where jump if branch condition expr is false
    CFGBlock* chain_entry = cfg_builder_ensure_curr(ctx);

    // we connect test(false) later, directly to the next test / else / merges
    CFGBlock* last_test = NULL;
    bool saw_else = false;

    for (u32 i = 0; i < branches->size; ++i) {
        const ASTNode* br = vector_at(*branches, i);

        // else-branch: expr == NULL
        if (br->as.stmt_branch.expr == NULL) {
            assert(i + 1 == branches->size && "invalid condition: 'else' branch must be last");

            CFGBlock* else_body = cfg_builder_create_block(ctx, CFG_BLOCK_NORMAL);

            // false from the last test drops into else directly
            if (last_test != NULL) {
               cfg_block_add_edge(last_test, CFG_EDGE_FALSE, else_body);
            }
            // defensive: no tests created
            else {
                cfg_block_add_edge(chain_entry, CFG_EDGE_FALLTHROUGH, else_body);
            }

            CFGBlock* saved_curr = ctx->curr;
            ctx->curr = else_body;
            cfg_builder_build_stmt_list(ctx, &br->as.stmt_branch.block);

            if (ctx->curr != NULL && ctx->curr->succ.size == 0) {
                cfg_block_add_edge(ctx->curr, CFG_EDGE_FALLTHROUGH, merge);
            }

            ctx->curr = saved_curr;

            saw_else = true;
            last_test = NULL;
            break;
        }

        // if / else-if condition expr
        CFGBlock* test = cfg_builder_create_block(ctx, CFG_BLOCK_COND_EXPR);
        cfg_block_add_stmt(test, br->as.stmt_branch.expr);

        // previous test's FALSS flows directly into this test
        if (last_test != NULL) {
            cfg_block_add_edge(last_test, CFG_EDGE_FALSE, test);
        }
        // first test: chain_entry -> test
        else {
            cfg_block_add_edge(chain_entry, CFG_EDGE_FALLTHROUGH, test);
        }

        // TRUE -> body
        CFGBlock* body = cfg_builder_create_block(ctx, CFG_BLOCK_NORMAL);
        cfg_block_add_edge(test, CFG_EDGE_TRUE, body);

        // build body
        CFGBlock* saved_curr = ctx->curr;
        ctx->curr = body;
        cfg_builder_build_stmt_list(ctx, &br->as.stmt_branch.block);
        if (ctx->curr != NULL && ctx->curr->succ.size == 0) {
            cfg_block_add_edge(ctx->curr, CFG_EDGE_FALLTHROUGH, merge);
        }
        ctx->curr = saved_curr;

        // keep this test so its FALSE can be wired to the *next* test/else/merge
        last_test = test;
    }

    // no else branch -> final test's FALSE goes to merge (no connector)
    if (!saw_else) {
        if (last_test != NULL) {
            cfg_block_add_edge(last_test, CFG_EDGE_FALSE, merge);
        }
        else {
            // degenerate case: no tests produced (malformed AST) � still wire something
            cfg_block_add_edge(chain_entry, CFG_EDGE_FALLTHROUGH, merge);
        }
    }

    // continue after the whole chain at the merge block
    ctx->curr = merge;

}

static void cfg_builder_maybe_warn_unreachable(CFGBuildCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL);
    if (ctx->curr == NULL) {
        REPORT_WARNING_LOC(ctx->rc, DIAG_SEMA_CFG, ast->loc, "unreachable statement detected");
    }
}

static inline void cfg_builder_build_stmt(CFGBuildCtx* ctx, const ASTNode* ast) {
    assert(ctx != NULL && ast != NULL);

    switch (ast->kind) {
    // just skip
    case AST_NODE_STMT_EMPTY:
        break;

    case AST_NODE_STMT_VAR_DECL:
    case AST_NODE_STMT_EXPR: {
        cfg_builder_maybe_warn_unreachable(ctx, ast);
        CFGBlock* block = cfg_builder_ensure_curr(ctx);
        cfg_block_add_stmt(block, ast);
    } break;

    case AST_NODE_STMT_RETURN: {
        cfg_builder_maybe_warn_unreachable(ctx, ast);
        CFGBlock* block = cfg_builder_ensure_curr(ctx);
        cfg_block_add_stmt(block, ast);
        cfg_block_add_edge(block, CFG_EDGE_FALLTHROUGH, ctx->cfg->exit);
        ctx->curr = NULL;
    } break;

    case AST_NODE_STMT_BREAK: {
        cfg_builder_maybe_warn_unreachable(ctx, ast);
        CFGBlock* block = cfg_builder_ensure_curr(ctx);
        cfg_block_add_stmt(block, ast);
        LoopTargets* target = cfg_builder_get_last_loop_targets(ctx);
        if (target == NULL) {
            REPORT_DEBUG(ctx->rc, DIAG_SEMA_CFG, "usage of invalid 'break'.");
        }
        else {
            cfg_block_add_edge(block, CFG_EDGE_FALLTHROUGH, target->break_target);
        }
        ctx->curr = NULL;
    } break;

    case AST_NODE_STMT_CONTINUE: {
        cfg_builder_maybe_warn_unreachable(ctx, ast);
        CFGBlock* block = cfg_builder_ensure_curr(ctx);
        cfg_block_add_stmt(block, ast);
        LoopTargets* target = cfg_builder_get_last_loop_targets(ctx);
        if (target == NULL) {
            REPORT_DEBUG(ctx->rc, DIAG_SEMA_CFG, "usage of invalid 'continue'.");
        }
        else {
            cfg_block_add_edge(block, CFG_EDGE_FALLTHROUGH, target->continue_target);
        }
        ctx->curr = NULL;
    } break;

    case AST_NODE_STMT_BLOCK: {
        cfg_builder_build_stmt_list(ctx, &ast->as.stmt_block.block);
    } break;

    case AST_NODE_STMT_CONDITION: {
        cfg_builder_build_condition(ctx, ast);
    } break;

    case AST_NODE_STMT_WHILE: {
        // layout:  curr -> cond; cond(T) -> body -> cond ; cond(F) -> after
        CFGBlock* cond = cfg_builder_create_block(ctx, CFG_BLOCK_LOOP_HEADER);
        cfg_block_add_edge(cfg_builder_ensure_curr(ctx), CFG_EDGE_FALLTHROUGH, cond);
        cfg_block_add_stmt(cond, ast->as.stmt_while.expr);

        CFGBlock* body = cfg_builder_create_block(ctx, CFG_BLOCK_NORMAL);
        CFGBlock* after = cfg_builder_create_block(ctx, CFG_BLOCK_NORMAL);

        cfg_block_add_edge(cond, CFG_EDGE_TRUE, body);
        cfg_block_add_edge(cond, CFG_EDGE_FALSE, after);

        // push loop targets
        cfg_builder_push_loop_targets(ctx, after, cond);

        ctx->curr = body;
        cfg_builder_build_stmt_list(ctx, &ast->as.stmt_while.block);
        if (ctx->curr != NULL) {
            cfg_block_add_edge(ctx->curr, CFG_EDGE_FALLTHROUGH, cond);
        }

        // pop loop targets
        cfg_builder_pop_loop_targets(ctx);

        ctx->curr = after;
        break;
    }

    case AST_NODE_STMT_DO: {
        // layout: curr -> body -> cond; cond(T) -> body ; cond(F) -> after
        CFGBlock* body = cfg_builder_create_block(ctx, CFG_BLOCK_NORMAL);
        CFGBlock* cond = cfg_builder_create_block(ctx, CFG_BLOCK_LOOP_HEADER);
        CFGBlock* after = cfg_builder_create_block(ctx, CFG_BLOCK_NORMAL);

        cfg_block_add_edge(cfg_builder_ensure_curr(ctx), CFG_EDGE_FALLTHROUGH, body);

        // continue should go to cond (post-test)
        cfg_builder_push_loop_targets(ctx, after, cond);

        CFGBlock* saved_curr = ctx->curr;
        ctx->curr = body;
        cfg_builder_build_stmt_list(ctx, &ast->as.stmt_do.block);
        if (ctx->curr != NULL) {
            cfg_block_add_edge(ctx->curr, CFG_EDGE_FALLTHROUGH, cond);
        }
        ctx->curr = saved_curr;

        cfg_block_add_stmt(cond, ast->as.stmt_do.expr);
        cfg_block_add_edge(cond, CFG_EDGE_TRUE, body);
        cfg_block_add_edge(cond, CFG_EDGE_FALSE, after);

        // pop loop targets
        cfg_builder_pop_loop_targets(ctx);

        ctx->curr = after;
        break;
    }

    default:
        unreachable();
        break;
    }
}

static void cfg_builder_build_stmt_list(CFGBuildCtx* ctx, const Vector* list) {
    assert(ctx != NULL && list != NULL);

    for (u32 i = 0; i < list->size; ++i) {
        const ASTNode* s = vector_at(*list, i);
        cfg_builder_build_stmt(ctx, s);
    }
}

CFGFunction* cfg_function_build(const ASTNode* ast, ReportCollector* rc) {
    assert(ast != NULL && rc != NULL);

    CFGFunction* cfg = malloc(sizeof(CFGFunction));
    assert(cfg != NULL);

    cfg->fun_decl = ast;
    cfg->blocks   = (Vector){ 0 };
    cfg->entry    = NULL;
    cfg->exit     = NULL;

    CFGBuildCtx ctx = { 0 };
    ctx.cfg          = cfg;
    ctx.rc           = rc;
    ctx.curr         = NULL;
    ctx.loop_targets = (Vector) { 0 };

    //
    cfg->entry = cfg_builder_create_block(&ctx, CFG_BLOCK_ENTRY);
    cfg->exit  = cfg_builder_create_block(&ctx, CFG_BLOCK_EXIT);

    CFGBlock* start = cfg_builder_create_block(&ctx, CFG_BLOCK_NORMAL);
    cfg_block_add_edge(cfg->entry, CFG_EDGE_FALLTHROUGH, start);
    ctx.curr = start;

    // function body
    cfg_builder_build_stmt_list(&ctx, &ast->as.fun_decl.block);

    // if function end is reachable, fallthrough to exit
    if (ctx.curr != NULL && ctx.curr->succ.size == 0) {
        cfg_block_add_edge(ctx.curr, CFG_EDGE_FALLTHROUGH, cfg->exit);
    }

    vector_destroy(&ctx.loop_targets);

    return cfg;
}

void cfg_function_destroy(CFGFunction* cfg) {
    if (cfg == NULL) {
        return;
    }

    vector_destroy(&cfg->blocks);
    free(cfg);
}