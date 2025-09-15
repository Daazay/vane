#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/vector.h"
#include "vane/utils/hashmap.h"
#include "vane/diagnostic/diagnostic.h"

struct Compiler;
struct Package;
struct Symbol;
struct ASTNode;

typedef enum CallKind CallKind;
typedef enum CallNodeKind CallNodeKind;

typedef struct CallEdge  CallEdge;
typedef struct CallNode  CallNode;
typedef struct CallGraph CallGraph;

enum CallKind {
    CALL_UNKNOWN = 0,
    CALL_DIRECT_LOCAL,
    CALL_DIRECT_EXTERNAL,
    CALL_INDIRECT_LOCAL,
    CALL_INDIRECT_EXTERNAL,
};

enum CallNodeKind {
    CALL_NODE_UNKNOWN = 0,
    CALL_NODE_LOCAL,
    CALL_NODE_EXTERNAL,
    CALL_NODE_SYNTHETIC,
};

struct CallNode {
    CallNodeKind kind;
    struct Symbol* symbol;
    struct Package* package;
    Vector out; // Vector<CallEdge>
};

struct CallEdge {
    CallKind kind;
    CallNode* to;
    const struct ASTNode* ast;
};

struct CallGraph {
    // k: [Symbol*, NULL]
    // v: [Package*, NULL]
    Hashmap nodes_by_sym;
    Vector nodes;
    struct Package* package;
    struct ReportCollector* rc;
};

const char* call_kind_get_name(CallKind kind);

const char* call_node_kind_get_name(CallNodeKind kind);

CallGraph* call_graph_create(struct Package* package, struct ReportCollector* rc);

void call_graph_destroy(CallGraph* graph);

CallNode* call_graph_get_or_add_node(CallGraph* graph, struct Symbol* fun, CallNodeKind kind, struct Package* package_hint);

void call_graph_add_edge(CallGraph* graph, struct Symbol* caller, struct Symbol* callee, CallKind kind, const struct ASTNode* ast);

void call_graph_merge_into(CallGraph* dest, const CallGraph* src);