#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/file_utils.h"
#include "vane/sema/call_graph.h"

void dump_call_graph_txt(FileWriter* w, const CallGraph* graph);

void dump_call_graph_dot(FileWriter* w, const CallGraph* graph);