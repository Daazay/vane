#pragma once

#include "vane/utils/defines.h"

typedef enum DiagnosticSeverity DiagnosticSeverity;

enum DiagnosticSeverity {
    DIAG_SEV_DEBUG,
    DIAG_SEV_NOTE,
    DIAG_SEV_INFO,
    DIAG_SEV_WARNING,
    DIAG_SEV_ERROR,

    DIAG_SEV_COUNT,
};

const char* diagnostic_severity_get_name(DiagnosticSeverity sev);