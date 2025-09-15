#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/vector.h"
#include "vane/cfg/cfg_block.h"

typedef struct CFGFunction CFGFunction;

struct CFGFunction {
    const ASTNode* fun_decl;
    Vector blocks;
    CFGBlock* entry;
    CFGBlock* exit;
};

CFGFunction* cfg_function_build(const ASTNode* ast, ReportCollector* rc);

void cfg_function_destroy(CFGFunction* cfg);