#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/diagnostic/diagnostic.h"
#include "vane/ast/ast_node.h"
#include "vane/sema/type.h"
#include "vane/sema/type_system.h"
#include "vane/sema/scope.h"

typedef struct CEvalResult CEvalResult;
typedef struct CEvalCtx    CEvalCtx;

typedef bool (*CEvalResolveSymbolFn)(const ASTNode* indent_node, u64* out_u, bool* out_is_signed);

struct CEvalResult {
    bool ok;
    bool is_const;
    bool is_signed;
    union {
        u64 u;
        i64 s;
    };
};

struct CEvalCtx {
    ReportCollector*     rc;
    TypeSystem*          ts;
    Scope*               scope;
    CEvalResolveSymbolFn resolve_fn;
};

CEvalResult ceval_eval_int(const ASTNode* expr, const CEvalCtx* ctx);

bool ceval_eval_array_size(const ASTNode* size_expr, const CEvalCtx* ctx, u64* out);

CEvalResult ceval_eval_strlen(const ASTNode* expr, const CEvalCtx* ctx);