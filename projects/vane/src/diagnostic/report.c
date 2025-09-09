#include "vane/diagnostic/report.h"

#include <stdio.h>
#include <stdlib.h>

#include "vane/utils/terminal.h"

Report* report_create(DiagnosticSeverity sev, String message, TokenLoc loc, StringView stage) {
    Report* report = malloc(sizeof(Report));
    assert(report != NULL);

    report->sev = sev;
    report->msg = message;
    report->loc = loc;
    report->stage = stage;

    return report;
}

void report_destroy(Report* report) {
    if (report == NULL) {
        return;
    }

    string_destroy(&report->msg);
    free(report);
}

static inline void print_sev(DiagnosticSeverity sev, bool with_color) {
    if (with_color) {
        switch (sev) {
        case DIAG_SEV_DEBUG:   terminal_set_color(TERMINAL_COLOR_WHITE,  TERMINAL_COLOR_BLACK); break;
        case DIAG_SEV_INFO:    terminal_set_color(TERMINAL_COLOR_GREEN,  TERMINAL_COLOR_BLACK); break;
        case DIAG_SEV_NOTE:    terminal_set_color(TERMINAL_COLOR_CYAN,   TERMINAL_COLOR_BLACK); break;
        case DIAG_SEV_WARNING: terminal_set_color(TERMINAL_COLOR_YELLOW, TERMINAL_COLOR_BLACK); break;
        case DIAG_SEV_ERROR:   terminal_set_color(TERMINAL_COLOR_RED,    TERMINAL_COLOR_BLACK); break;
        default:
            unreachable();
            break;
        }
    }

    printf("%s", diagnostic_severity_get_name(sev));
    if (with_color) {
        terminal_reset_format();
    }
}

void report_print(const Report* report, bool with_color) {
    assert(report != NULL);

    if (!is_string_view_empty(report->loc.path)) {
        printf(SV_FMT":%d:%d: ", SV_ARG(report->loc.path),
            report->loc.begin.line, report->loc.begin.column
        );
    }

    printf("["SV_FMT"] ", SV_ARG(report->stage));

    print_sev(report->sev, with_color);

    printf(": "SV_FMT"\n", SV_ARG(report->msg));
}