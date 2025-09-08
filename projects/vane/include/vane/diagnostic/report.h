#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/string_view.h"

#include "vane/diagnostic/diagnostic.h"
#include "vane/scanner/token_loc.h"

typedef struct Report Report;

struct ReportNote {
    String message;
    TokenLoc loc;
};

struct Report {
    DiagnosticSeverity sev;

    String msg;
    TokenLoc loc;

    StringView stage;
};

Report* report_create(DiagnosticSeverity sev, String message, TokenLoc loc, StringView stage);

void report_destroy(Report* report);

void report_print(const Report* report, bool with_color);