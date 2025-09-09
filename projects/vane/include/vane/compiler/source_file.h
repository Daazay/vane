#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/file_utils.h"
#include "vane/utils/hashmap.h"

#include "vane/ast/ast_node.h"

#include "vane/diagnostic/report_collector.h"

#include "vane/sema/scope.h"

struct Compiler;
typedef struct SourceFile SourceFile;
typedef struct ImportEntry ImportEntry;

struct ImportEntry {
    StringView name;

    const ASTNode* node;
    struct Package* target;

    StringView collection_name;
    StringView package_path;
};

struct SourceFile {
    String content;
    StringView path;

    ASTNode* ast;

    //// k: [StringView, NULL]
    //// v: [ImportEntry, NULL]
    //Hashmap imports;

    // ImportEntry
    Vector imports;

    Scope* scope;
    struct Package* package;

    ReportCollector* rc;
};

SourceFile* source_file_create(StringView path, ReportCollector* rc);

void source_file_destroy(SourceFile* source_file);

bool source_file_parse_ast(SourceFile* source_file);

bool source_file_resolve_imports(SourceFile* source_file, struct Compiler* compiler);

bool source_file_resolve_symbol_decls(SourceFile* source_file);

bool source_file_bind_symbols(SourceFile* source_file);