#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/vector.h"
#include "vane/diagnostic/report.h"

#include "vane/diagnostic/diagnostic_tags.h"

typedef struct ReportCollector ReportCollector;

#define REPORT_COLLECTOR_DEFAULT_CAPACITY 16

struct ReportCollector {
    Vector reports;
    u32 sev_count[DIAG_SEV_COUNT];
    DiagnosticSeverity verbosity;
};

ReportCollector report_collector_create(DiagnosticSeverity verbosity);

void report_collector_destroy(ReportCollector* collector);

void report_collector_clear(ReportCollector* collector);

void report_collector_push_report(ReportCollector* collector, Report* report);

void report_collector_print_all(const ReportCollector* collector, bool with_color);

// -- common --

static inline enum DiagnosticSeverity min_severity_for_verbosity(u32 verbosity) {
    switch (verbosity) {
    case 0: return DIAG_SEV_ERROR;   // errors
    case 1: return DIAG_SEV_WARNING; // + warnings
    case 2: return DIAG_SEV_INFO;    // + info
    case 3: return DIAG_SEV_NOTE;    // + notes
    default: return DIAG_SEV_DEBUG;  // + debug (everything)
    }
}

#define SHOULD_REPORT(SEV, VERBOSITY) \
    ((SEV) >= min_severity_for_verbosity(VERBOSITY))

#define REPORT_LOC(RC, SEV, STAGE, LOC, FORMAT, ...) do { \
    if (SHOULD_REPORT(SEV, (RC)->verbosity)) { \
        String __report_message = string_from_fmt(FORMAT, ##__VA_ARGS__); \
        Report* report = report_create(SEV, __report_message, LOC, STR_LIT(STAGE)); \
        report_collector_push_report(RC, report); \
    } \
} while (false);

#define REPORT(RC, SEV, STAGE, FORMAT, ...) REPORT_LOC(RC, SEV, STAGE, ((TokenLoc) {0}), FORMAT, ##__VA_ARGS__)

#define REPORT_DEBUG_LOC(RC, STAGE, LOC, FORMAT, ...)   REPORT_LOC(RC, DIAG_SEV_DEBUG, STAGE, LOC, FORMAT, ##__VA_ARGS__)
#define REPORT_DEBUG(RC, STAGE, FORMAT, ...)            REPORT(RC, DIAG_SEV_DEBUG, STAGE, FORMAT, ##__VA_ARGS__)
#define REPORT_INFO_LOC(RC, STAGE, LOC, FORMAT, ...)    REPORT_LOC(RC, DIAG_SEV_INFO, STAGE, LOC, FORMAT, ##__VA_ARGS__)
#define REPORT_INFO(RC, STAGE, FORMAT, ...)             REPORT(RC, DIAG_SEV_INFO, STAGE, FORMAT, ##__VA_ARGS__)
#define REPORT_NOTE_LOC(RC, STAGE, LOC, FORMAT, ...)    REPORT_LOC(RC, DIAG_SEV_NOTE, STAGE, LOC, FORMAT, ##__VA_ARGS__)
#define REPORT_NOTE(RC, STAGE, FORMAT, ...)             REPORT(RC, DIAG_SEV_NOTE, STAGE, FORMAT, ##__VA_ARGS__)
#define REPORT_WARNING_LOC(RC, STAGE, LOC, FORMAT, ...) REPORT_LOC(RC, DIAG_SEV_WARNING, STAGE, LOC, FORMAT, ##__VA_ARGS__)
#define REPORT_WARNING(RC, STAGE, FORMAT, ...)          REPORT(RC, DIAG_SEV_WARNING, STAGE, FORMAT, ##__VA_ARGS__)
#define REPORT_ERROR_LOC(RC, STAGE, LOC, FORMAT, ...)   REPORT_LOC(RC, DIAG_SEV_ERROR, STAGE, LOC, FORMAT, ##__VA_ARGS__)
#define REPORT_ERROR(RC, STAGE, FORMAT, ...)            REPORT(RC, DIAG_SEV_ERROR, STAGE, FORMAT, ##__VA_ARGS__)