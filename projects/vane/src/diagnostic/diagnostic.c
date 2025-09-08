#include "vane/diagnostic/diagnostic.h"

const char* diagnostic_severity_get_name(DiagnosticSeverity sev) {
    switch (sev) {
    case DIAG_SEV_DEBUG:   return "debug";
    case DIAG_SEV_NOTE:    return "note";
    case DIAG_SEV_INFO:    return "info";
    case DIAG_SEV_WARNING: return "warning";
    case DIAG_SEV_ERROR:   return "error";
    default:
        unreachable();
        return NULL;
    }
}