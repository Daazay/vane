#include "vane/compiler/package.h"

#include <stdlib.h>

#include "vane/compiler/compiler.h"
#include "vane/ast/ast_visitor.h"
#include "vane/sema/typecheck.h"

Package* package_create(StringView path, struct Compiler* compiler) {
    Package* package = malloc(sizeof(Package));
    assert(package != NULL);

    package->path = path;

    package->scope = NULL;
    package->parent_package = NULL;

    package->source_files = (Vector){ 0 };
    package->subpackages = (Vector) { 0 };

    package->compiler = compiler;
    package->is_core = false;

    package->call_graph = NULL;

    return package;
}

void package_destroy(Package* package) {
    if (package == NULL) {
        return;
    }

    vector_destroy(&package->source_files);
    vector_destroy(&package->subpackages);

    call_graph_destroy(package->call_graph);

    free(package);
}

void package_add_source_file(Package* package, SourceFile* source_file) {
    assert(package != NULL && source_file != NULL);
    assert(source_file->package == NULL && source_file->package != package && "source file already belongs to another package");

    if (package->source_files.raw == 0) {
        package->source_files = vector_create(
            PACKAGE_DEFAULT_SOURCE_FILE_COUNT,
            VECTOR_SPECS(const SourceFile*, NULL)
        );
    }

    source_file->package = package;
    vector_push_back(&package->source_files, &source_file);
}

void package_add_subpackage(Package* package, Package* subpackage) {
    assert(package != NULL && subpackage != NULL);
    assert(subpackage->parent_package == NULL && subpackage->parent_package != package && "subpackage already attached elsewhere");

    if (package->subpackages.raw == 0) {
        package->subpackages = vector_create(
            PACKAGE_DEFAULT_SUBPACKAGE_COUNT,
            VECTOR_SPECS(const Package*, NULL)
        );
    }

    subpackage->parent_package = package;
    vector_push_back(&package->subpackages, &subpackage);
}

bool package_resolve_symbol_decls(Package* package, Scope* prelude_scope) {
    assert(package != NULL);
    assert(prelude_scope != NULL || (package->parent_package && package->parent_package->scope != NULL));

    if (package->scope != NULL) {
        return true;
    }

    // Chain to parent scope if available, otherwise to prelude/global.
    Scope* parent_chain = (package->parent_package && package->parent_package->scope)
        ? package->parent_package->scope
        : prelude_scope;

    assert(parent_chain != NULL && "A package scope must chain to a valid parent/prelude.");

    package->scope = scope_create(SCOPE_PACKAGE, parent_chain, NULL, package);

    REPORT_DEBUG(&package->compiler->rc, DIAG_SEMA_SYMBOLS, "created package scope for '" SV_FMT "'.", SV_ARG(package->path));

    bool is_good = true;
    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* source_file = vector_at(package->source_files, i);

        if (!source_file_resolve_symbol_decls(source_file)) {
            is_good = false;
        }
    }

    return is_good;
}

bool package_bind_symbols(Package* package) {
    assert(package != NULL);
    assert(package->scope != NULL && "bind requires package scope (resolve decls first)");

    bool is_good = true;
    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* source_file = vector_at(package->source_files, i);

        assert(source_file->scope != NULL && "bind requires file scope; call resolve decls first");

        if (!source_file_bind_symbols(source_file)) {
            is_good = false;
        }
    }

    return is_good;
}

bool package_validate_semantics(Package* package) {
    assert(package != NULL);
    bool is_good = true;

    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* sf = vector_at(package->source_files, i);
        if (!source_file_validate_semantics(sf)) {
            is_good = false;
        }
    }
    return is_good;
}

typedef struct CallGraphBuildCtx CallGraphBuildCtx;

struct CallGraphBuildCtx {
    CallGraph* graph;
    Package* package;
    Symbol* current_fun;
    Scope* scope;
    TypeSystem* ts;
    ReportCollector* rc;
};

static inline void call_graph_build_pre(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    CallGraphBuildCtx* ctx = (CallGraphBuildCtx*)data;
    assert(ctx != NULL);

    switch (node->kind) {
    case AST_NODE_FUN_DECL: {
        ctx->scope = node->scope;
        ctx->current_fun = node->symbol;

        if (ctx->current_fun != NULL) {
            call_graph_get_or_add_node(ctx->graph, ctx->current_fun, CALL_NODE_LOCAL, ctx->package);
        }
    } break;

    case AST_NODE_STMT_BLOCK:
    case AST_NODE_STMT_BRANCH:
    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO:  {
        ctx->scope = node->scope;
    } break;

    default: break;
    }
}

static inline void call_graph_build_post(ASTNode* parent, ASTNode* node, void* data) {
    (void)parent;

    CallGraphBuildCtx* ctx = (CallGraphBuildCtx*)data;
    assert(ctx != NULL);

    if (node->kind == AST_NODE_EXPR_CALL) {
        if (ctx->current_fun == NULL) {
            return;
        }

        ASTNode* callee = node->as.expr_call.callee;

        // 1) bound to a function symbol -> direct (local/external)
        if (callee->symbol != NULL && callee->symbol->kind == SYMBOL_FUNCTION) {
            CallKind kind = (callee->symbol->scope->package == ctx->package)
                ? CALL_DIRECT_LOCAL
                : CALL_DIRECT_EXTERNAL;

            call_graph_add_edge(ctx->graph, ctx->current_fun, callee->symbol, kind, node);
        }
        // 2) not a function symbol, check its type to see if its a function typed expression
        else {
            Type* t = typecheck_resolve_expr_type(ctx->scope, callee, ctx->ts, ctx->rc);
            const Type* u = t != NULL ? type_unwrap(t) : NULL;

            // indirect call: we don't know the precise callee symbol
            if (u != NULL && u->kind == TYPE_FUNCTION) {
                CallKind kind = CALL_INDIRECT_LOCAL;
                call_graph_add_edge(ctx->graph, ctx->current_fun, NULL, kind, node);
            }
        }
    }

    switch (node->kind) {
    case AST_NODE_FUN_DECL:
    case AST_NODE_STMT_BLOCK:
    case AST_NODE_STMT_BRANCH:
    case AST_NODE_STMT_WHILE:
    case AST_NODE_STMT_DO:
        if (ctx->scope != NULL) {
            ctx->scope = ctx->scope->parent;
        }
        if (node->kind == AST_NODE_FUN_DECL) {
            ctx->current_fun = NULL;
        }
        break;
    default: break;
    }
}

bool package_build_call_graph(struct Package* package) {
    assert(package != NULL && package->scope != NULL);

    if (package->call_graph != NULL) {
        return true;
    }

    CallGraph* graph = call_graph_create(package, &package->compiler->rc);
    CallGraphBuildCtx ctx = {
        .graph = graph,
        .package = package,
        .current_fun = NULL,
        .scope = package->scope,
        .ts = &package->compiler->ts,
        .rc = &package->compiler->rc,
    };

    ASTVisitor v = {
        .data = &ctx,
        .pre_fn = &call_graph_build_pre,
        .post_fn = &call_graph_build_post,
    };

    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* source_file = vector_at(package->source_files, i);

        const Vector* ents = &source_file->ast->as.source_file.entities;
        for (u32 k = 0; k < ents->size; ++k) {
            ASTNode* ast = vector_at(*ents, k);
            ast_visit_with(NULL, ast, &v);
        }
    }

    package->call_graph = graph;
    return true;
}

bool package_build_cfg(Package* package) {
    assert(package != NULL);

    bool is_good = true;

    for (u32 i = 0; i < package->source_files.size; ++i) {
        SourceFile* source_file = vector_at(package->source_files, i);

        if (!source_file_build_cfgs(source_file)) {
            is_good = false;
        }
    }

    return is_good;
}