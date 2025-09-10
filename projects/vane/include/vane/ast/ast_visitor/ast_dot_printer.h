#pragma once

#include "vane/ast/ast_visitor/ast_visitor.h"

void ast_dot_visitor_print_fn(ASTNode* parent, ASTNode* node, void* data);

void ast_print_dot(const ASTNode* node, void* stream);