#include "vane/sema/call_graph.h"

#include <stdlib.h>

#include "vane/compiler/package.h"
#include "vane/compiler/compiler.h"
#include "vane/sema/scope.h"
#include "vane/ast/ast_node.h"
#include "vane/ast/ast_visitor.h"
#include "vane/sema/type.h"

static inline CallNode* call_graph_create_node(CallGraph* graph, struct Symbol* symbol, CallNodeKind kind, struct Package* package) {
    assert(graph != NULL && symbol != NULL && package != NULL);

    CallNode* node = malloc(sizeof(CallNode));
    assert(node != NULL);

    node->symbol  = symbol;
    node->package = package;
    node->kind    = kind;
    node->out     = (Vector) { 0 };

    vector_push_back(&graph->nodes, &node);
    return node;
}

static inline void call_node_destroy(CallNode* node) {
    if (node == NULL) {
        return;
    }

    vector_destroy(&node->out);
    free(node);
}

const char* call_kind_get_name(CallKind kind) {
    switch (kind) {
    case CALL_DIRECT_LOCAL:      return "direct";
    case CALL_DIRECT_EXTERNAL:   return "direct-external";
    case CALL_INDIRECT_LOCAL:    return "indirect";
    case CALL_INDIRECT_EXTERNAL: return "indirect-external";
    default:
        unreachable();
        return NULL;
    }
}

const char* call_node_kind_get_name(CallNodeKind  kind) {
    switch (kind) {
    case CALL_NODE_LOCAL:     return "local";
    case CALL_NODE_EXTERNAL:  return "external";
    case CALL_NODE_SYNTHETIC: return "synthetic";
    default:
        unreachable();
        return NULL;
    }
}

CallGraph* call_graph_create(struct Package* package, ReportCollector* rc) {
    assert(rc != NULL);

    CallGraph* graph = malloc(sizeof(CallGraph));
    assert(graph != NULL);

    graph->package      = package;
    graph->rc           = rc;
    graph->nodes        = vector_create(8, VECTOR_SPECS(CallNode*, &call_node_destroy));
    graph->nodes_by_sym = hashmap_create(8,
        HASHMAP_KEY_SPECS(Symbol*, &item_ptr_hash, &item_ptr_eq, NULL),
        HASHMAP_VALUE_SPECS(CallNode*, NULL)
    );

    return graph;
}

void call_graph_destroy(CallGraph* graph) {
    if (graph == NULL) {
        return;
    }

    vector_destroy(&graph->nodes);
    hashmap_destroy(&graph->nodes_by_sym);
    free(graph);
}

CallNode* call_graph_get_or_add_node(CallGraph* graph, Symbol* fun, CallNodeKind kind, struct Package* package_hint) {
    if (fun == NULL) {
        return NULL;
    }

    CallNode* found = hashmap_get(&graph->nodes_by_sym, &fun);
    if (found != NULL) {
        return found;
    }

    // determine node kind if not provided explicitly: local if matches owner, else external
    CallNodeKind final_kind = kind;
    if (final_kind != CALL_NODE_SYNTHETIC && graph->package != NULL) {
        final_kind = (fun->scope->package = graph->package)
            ? CALL_NODE_LOCAL
            : CALL_NODE_EXTERNAL;
    }

    CallNode* node = call_graph_create_node(graph, fun, final_kind, package_hint != NULL ? package_hint : fun->scope->package);
    hashmap_insert(&graph->nodes_by_sym, &fun, &node);
    return node;
}

void call_graph_add_edge(CallGraph* graph, struct Symbol* caller, struct Symbol* callee, CallKind kind, const ASTNode* ast) {
    assert(graph != NULL);

    if (caller == NULL) {
        return;
    }

    CallNode* from = call_graph_get_or_add_node(graph, caller, CALL_NODE_LOCAL, caller->scope->package);
    if (from == NULL) {
        return;
    }

    CallNode* to = NULL;
    if (callee != NULL) {
        CallNodeKind k = (caller->scope->package == callee->scope->package) ? CALL_NODE_LOCAL : CALL_NODE_EXTERNAL;
        to = call_graph_get_or_add_node(graph, callee, k, callee->scope->package);
    }

    // for unknown indirect, we create a synthetic sink.
    CallEdge e = { .to = to, .kind = kind, .ast = ast, };

    if (from->out.raw == NULL) {
        from->out = vector_create(2, VECTOR_SPECS(CallEdge, NULL));
    }

    vector_push_back(&from->out, &e);
}

void call_graph_merge_into(CallGraph* dest, const CallGraph* src) {
    assert(dest != NULL && src != NULL);

    for (u32 i = 0; i < src->nodes.size; ++i) {
        const CallNode* src_n = vector_at(src->nodes, i);
        CallNode* dest_n = call_graph_get_or_add_node(dest, src_n->symbol, src_n->kind, src_n->package);
        (void)dest_n;

        for (u32 j = 0; j < src_n->out.size; ++j) {
            const CallEdge* src_e = vector_at(src_n->out, j);
            call_graph_add_edge(dest, src_n->symbol, src_e->to != NULL ? src_e->to->symbol : NULL, src_e->kind, src_e->ast);
        }
    }
}