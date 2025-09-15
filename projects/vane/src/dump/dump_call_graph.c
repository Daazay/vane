#include "vane/dump/call_graph_dump.h"

#include "vane/ast/ast_node.h"
#include "vane/sema/symbol.h"

void dump_call_graph_txt(FileWriter* w, const CallGraph* graph) {
    assert(graph != NULL && w != NULL);

    file_writer_write_format(w, "CallGraph (%s)\n", graph->package != NULL
        ? "package"
        : "global"
    );

    for (u32 i = 0; i < graph->nodes.size; ++i) {
        const CallNode* node = vector_at(graph->nodes, i);
        const StringView name = node->symbol != NULL
            ? node->symbol->name
            : STR_LIT("<synthetic>");

        file_writer_write_format(w, "  "SV_FMT":\n", SV_ARG(name));

        for (u32 k = 0; k < node->out.size; ++k) {
            const CallEdge* edge = vector_at(node->out, k);

            StringView to_name = edge->to != NULL && edge->to->symbol != NULL
                ? edge->to->symbol->name
                : STR_LIT("<unknown>");

            file_writer_write_format(w, "    -> "SV_FMT"  [%s]  @"SV_FMT"%u:%u\n",
                SV_ARG(to_name), call_kind_get_name(edge->kind),
                SV_ARG(edge->ast->loc.path), edge->ast->loc.begin.line, edge->ast->loc.begin.column
            );
        }
    }
}

void dump_call_graph_dot(FileWriter* w, const CallGraph* graph) {
    assert(graph != NULL && w != NULL);

    file_writer_write_cstr(w, "digraph callGraph {\n");
    file_writer_write_cstr(w, "  rankdir=LR;\n");

    for (u32 i = 0; i < graph->nodes.size; ++i) {
        const CallNode* node = vector_at(graph->nodes, i);

        const StringView name = node->symbol != NULL
            ? node->symbol->name
            : STR_LIT("<synthetic>");


        const bool ext = (node->kind == CALL_NODE_EXTERNAL);

        file_writer_write_format(w, "  \"%llu\" [label=\""SV_FMT"\"%s];\n",
            (u64)(uptr_t)node, SV_ARG(name), ext ? ", style=dashed" : ""
        );
    }

    for (u32 i = 0; i < graph->nodes.size; ++i) {
        const CallNode* node = vector_at(graph->nodes, i);

        for (u32 k = 0; k < node->out.size; ++k) {
            const CallEdge* edge = vector_at(node->out, k);

            const char* style = "solid";

            if (edge->kind == CALL_INDIRECT_LOCAL || edge->kind == CALL_INDIRECT_EXTERNAL) {
                style = "dotted";
            }

            u64 from = (u64)(uptr_t)node;
            u64 to = edge->to != NULL ? (u64)(uptr_t)edge->to : (u64)(uptr_t)node;

            file_writer_write_format(w, "  \"%lld\" -> \"%lld\" [label=\"%s\", style=%s];\n", from, to, call_kind_get_name(edge->kind), style);
        }
    }

    file_writer_write_cstr(w, "}\n");
}