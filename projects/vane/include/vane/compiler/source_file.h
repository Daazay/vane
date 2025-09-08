#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/file_utils.h"

#include "vane/ast/ast_node.h"

#include "vane/diagnostic/report_collector.h"

struct Compiler;
typedef struct SourceFile SourceFile;

struct SourceFile {
    String content;
    StringView path;

    ASTNode* ast;

    // ImportEntry
    Vector imports;

    struct Package* package;

    ReportCollector* rc;
};

SourceFile* source_file_create(StringView path, ReportCollector* rc);

void source_file_destroy(SourceFile* source_file);

bool source_file_parse_ast(SourceFile* source_file);

bool source_file_resolve_imports(SourceFile* source_file, struct Compiler* compiler);