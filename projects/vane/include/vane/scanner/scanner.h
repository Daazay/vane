#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/string_view.h"

#include "vane/scanner/token.h"

typedef struct Scanner Scanner;

#define SCANNER_DEFAULT_LINE_POS   1
#define SCANNER_DEFAULT_COLUMN_POS 1

struct Scanner {
    TokenLoc loc;

    StringView content;
    u64 pos;

    TokenFlags flags;
};

Scanner scanner_create(StringView path, StringView content);

Token scanner_scan_next(Scanner* scanner);