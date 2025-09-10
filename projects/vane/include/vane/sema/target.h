#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string_view.h"

typedef enum TargetArchitecture TargetArchitecture;
typedef struct TargetInfo TargetInfo;

enum TargetArchitecture {
    TARGET_ARCH_X86_64,

    TARGET_ARCH_COUNT,
};

struct TargetInfo {
    TargetArchitecture arch;
    u8 pointer_size;
    u8 int_size;
    u8 max_alignment;
};

extern TargetInfo target_infos[];

StringView target_arch_to_sv(TargetArchitecture arch);

bool target_arch_from_sv(StringView sv, TargetArchitecture* out);