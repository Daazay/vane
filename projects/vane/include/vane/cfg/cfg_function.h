#pragma once

#include "vane/utils/defines.h"

#include "vane/cfg/cfg_block.h"

struct ASTNode;
struct ReportCollector;
typedef struct CFGFunction CFGFunction;

struct CFGFunction {
    Vector blocks;
    const struct ASTNode* ast;
    CFGBlock* entry;
    CFGBlock* exit;
};

CFGFunction* cfg_build_function(const struct ASTNode* ast, struct ReportCollector* rc);

void cfg_function_destroy(CFGFunction* cfg);