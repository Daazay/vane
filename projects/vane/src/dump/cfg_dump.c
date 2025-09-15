#include "vane/dump/cfg_dump.h"
#include "vane/ast/ast_node.h"

static inline void dot_write_escaped(FileWriter* w, StringView sv) {
    assert(w != NULL);

    for (u64 i = 0; i < sv.len; ++i) {
        char c = (char)sv.data[i];
        switch (c) {
        case '\\': file_writer_write_cstr(w, "\\\\"); break;
        case '\"': file_writer_write_cstr(w, "\\\""); break;
        case '\n': file_writer_write_cstr(w, "\\l");  break; // newline -> line break
        case '\r': /* skip */                          break;
        case '\t': file_writer_write_cstr(w, "\\t");  break;
        default:
            // Graphviz is fine with most ASCII, but be conservative.
            file_writer_write_format(w, "%c", c);
            break;
        }
    }
}

static inline void dump_cfg_dot_edges(FileWriter* w, const CFGBlock* block) {
    assert(w != NULL && block != NULL);

    for (u32 i = 0; i < block->succ.size; ++i) {
        const CFGEdge* succ = vector_at(block->succ, i);

        file_writer_write_format(w, "  bb%u -> bb%u [label=\"%s\"];\n", (u64)(uptr_t)block->id, (u64)(uptr_t)succ->from->id, cfg_edge_kind_get_name(succ->kind));
    }
}

static inline void dump_cfg_dot_block(FileWriter* w, const CFGBlock* block) {
    assert(w != NULL && block != NULL);

    // node attributes: shape=box, rounded
    file_writer_write_format(w, "  bb%u [shape=box, style=\"rounded\", color=\"%s\", label=\"",
        (u64)(uptr_t)block->id,
        ((block->kind != CFG_BLOCK_ENTRY && block->pred.size == 0) ? "red" : "")
    );

    // header line "B{id}:"
    file_writer_write_cstr(w, "B");
    file_writer_write_format(w, "%u", (u64)(uptr_t)block->id);
    file_writer_write_format(w, ":%s\\l", cfg_block_kind_get_name(block->kind));

    // stmts (if any)
    for (u32 i = 0; i < block->stmts.size; ++i) {
        const ASTNode* stmt = vector_at(block->stmts, i);

        file_writer_write_cstr(w, "  ");
        file_writer_write_cstr(w, ast_node_kind_get_name(stmt->kind));
        file_writer_write_cstr(w, "\\l");
    }

    file_writer_write_cstr(w, "\"];\n");
}

void dump_cfg_function_dot(FileWriter* w, const CFGFunction* cfg) {
    assert(w != NULL && cfg != NULL);

    // graph header
    file_writer_write_sv(w, STR_LIT("digraph CFG {\n"));
    file_writer_write_sv(w, STR_LIT("  graph [fontname=\"Consolas\", rankdir=TB];\n"));
    file_writer_write_sv(w, STR_LIT("  node  [fontname=\"Consolas\", fontsize=10];\n"));
    file_writer_write_sv(w, STR_LIT("  edge  [fontname=\"Consolas\", fontsize=9];\n"));

    // title node (invisible) to show function name at top

    file_writer_write_sv(w, STR_LIT("  label=\""));
    //StringView fun_name = string_get_view(cfg->ast->as.fun_decl.sign->as.fun_sign.id->as.id.value);
    //file_writer_write_sv(w, fun_name);
    file_writer_write_sv(w, STR_LIT("\";\n"));

    file_writer_write_sv(w, STR_LIT("  labelloc=top;\n"));
    file_writer_write_sv(w, STR_LIT("  fontsize=12;\n\n"));
    file_writer_write_sv(w, STR_LIT("  fontsize=12;\n\n"));

    // nodes
    for (u32 i = 0; i < cfg->blocks.size; ++i) {
        const CFGBlock* block = vector_at(cfg->blocks, i);
        dump_cfg_dot_block(w, block);
    }

    file_writer_write_cstr(w, "\n");

    // edges
    for (u32 i = 0; i < cfg->blocks.size; ++i) {
        const CFGBlock* bb = vector_at(cfg->blocks, i);
        dump_cfg_dot_edges(w, bb);
    }

    file_writer_write_cstr(w, "}\n");
}