#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/string_view.h"

#include "vane/scanner/token.h"

#include "vane/diagnostic/report_collector.h"

typedef struct Scanner Scanner;

#define SCANNER_DEFAULT_LINE_POS                1
#define SCANNER_DEFAULT_COLUMN_POS              1
#define SCANNER_MAX_COMMENT_BLOCK_NESTING_LEVEL 10

struct Scanner {
    TokenLoc loc;

    StringView content;
    u64 pos;

    Rune curr_rune;
    u32 curr_rune_len;

    TokenFlags flags;

    ReportCollector* rc;
};

Scanner scanner_create(StringView path, StringView content, ReportCollector* rc);

Token scanner_scan_next(Scanner* scanner);