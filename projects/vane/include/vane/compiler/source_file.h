#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/file_utils.h"
#include "vane/utils/hashmap.h"

#include "vane/ast/ast_node.h"

#include "vane/diagnostic/report_collector.h"

#include "vane/sema/scope.h"

struct Compiler;
typedef enum ImportBaseKind ImportBaseKind;
typedef struct SourceFile SourceFile;
typedef struct ImportEntry ImportEntry;

#define SOURCE_FILE_DEFAULT_ENTITIES_COUNT 8
#define SOURCE_FILE_DEFAULT_IMPORT_COUNT   2

enum ImportBaseKind {
    IMPORT_BASE_RELATIVE = 9, // "package_path"                 -> from current package (default)
    IMPORT_BASE_PROJECT_ROOT, // ":package_path"                -> from project root
    IMPORT_BASE_COLLECTION,   // "collection_name:package_path" -> from collection
};

struct ImportEntry {
    StringView name;
    ASTNode* node;
    struct Package* target;

    ImportBaseKind base;
    StringView collection_name; // if base == IMPORT_BASE_COLLECTION
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

    bool imports_resolved;
};

SourceFile* source_file_create(StringView path, ReportCollector* rc);

void source_file_destroy(SourceFile* source_file);

bool source_file_parse_ast(SourceFile* source_file);

bool source_file_resolve_imports(SourceFile* source_file, struct Compiler* compiler);

bool source_file_resolve_symbol_decls(SourceFile* source_file);

bool source_file_bind_symbols(SourceFile* source_file);