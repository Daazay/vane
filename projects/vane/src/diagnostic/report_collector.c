#include "vane/diagnostic/report_collector.h"

#include <stdio.h>

ReportCollector report_collector_create(DiagnosticSeverity verbosity) {
    return (ReportCollector) {
        .reports = vector_create(
            REPORT_COLLECTOR_DEFAULT_CAPACITY,
            VECTOR_SPECS(Report*, &report_destroy)
        ),
        .sev_count = { 0 },
        .verbosity = verbosity,
    };
}

void report_collector_destroy(ReportCollector* collector) {
    if (collector == NULL) {
        return;
    }

    vector_destroy(&collector->reports);
}

void report_collector_clear(ReportCollector* collector) {
    assert(collector != NULL);

    vector_clear(&collector->reports);

    for (u32 i = 0; i < DIAG_SEV_COUNT; i++) {
        collector->sev_count[i] = 0;
    }
}

void report_collector_push_report(ReportCollector* collector, Report* report) {
    assert(collector != NULL && report != NULL);

    collector->sev_count[report->sev]++;

    vector_push_back(&collector->reports, &report);
}

static inline void report_collector_print_summary(const ReportCollector* rc) {
    const u32 e = rc->sev_count[DIAG_SEV_ERROR];
    const u32 w = rc->sev_count[DIAG_SEV_WARNING];
    const u32 i = rc->sev_count[DIAG_SEV_INFO];
    const u32 n = rc->sev_count[DIAG_SEV_NOTE];
    const u32 d = rc->sev_count[DIAG_SEV_DEBUG];
    const u32 total = e + w + i + n + d;

    if (total == 0) {
        return;
    }

    printf("\nSummary: %u error(s), %u warnings(s), %u info(s), %u note(s), %u debug.\n", e, w, i, n, d);
}

void report_collector_print_all(const ReportCollector* collector, bool with_color) {
    assert(collector != NULL);

    for (u32 i = 0; i < collector->reports.size; i++) {
        Report* report = vector_at(collector->reports, i);
        report_print(report, with_color);
    }
    report_collector_print_summary(collector);
}