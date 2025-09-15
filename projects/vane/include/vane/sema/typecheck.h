#pragma once

#include "vane/utils/defines.h"
#include "vane/sema/type.h"
#include "vane/sema/type_system.h"
#include "vane/sema/scope.h"
#include "vane/diagnostic/diagnostic.h"

Type* typecheck_resolve_expr_type(Scope* scope, ASTNode* expr, TypeSystem* ts, ReportCollector* rc);