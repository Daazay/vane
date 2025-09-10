#pragma once

#include "vane/ast/ast_visitor/ast_visitor.h"

void ast_simple_visitor_pre_fn(ASTNode* parent, ASTNode* node, void* data);

void ast_simple_visitor_post_fn(ASTNode* parent, ASTNode* node, void* data);

void ast_print_simple(const ASTNode* node, void* stream);