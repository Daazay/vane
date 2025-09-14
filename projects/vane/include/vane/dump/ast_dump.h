#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/file_utils.h"
#include "vane/ast/ast_node.h"

void dump_ast_text(FileWriter* w, const ASTNode* ast);

void dump_ast_dot(FileWriter* w, const ASTNode* ast);