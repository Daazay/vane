#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/file_utils.h"
#include "vane/cfg/cfg_function.h"

void dump_cfg_function_dot(FileWriter* w, const CFGFunction* cfg);