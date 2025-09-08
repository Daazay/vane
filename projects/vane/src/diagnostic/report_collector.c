#include "vane/diagnostic/report_collector.h"

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

void report_collector_print_all(const ReportCollector* collector, bool with_color) {
    assert(collector != NULL);

    for (u32 i = 0; i < collector->reports.size; i++) {
        Report* report = vector_at(collector->reports, i);
        report_print(report, with_color);
    }
}