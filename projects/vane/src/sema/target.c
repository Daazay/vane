#include "vane/sema/target.h"

TargetInfo target_infos[TARGET_ARCH_COUNT] = {
    { TARGET_ARCH_X86_64, 8, 8, 16 },
};

StringView target_arch_to_sv(TargetArchitecture arch) {
    switch (arch) {
    case TARGET_ARCH_X86_64: return STR_LIT("x86_64");
    default: return STR_LIT("unknown");
    }
}

bool target_arch_from_sv(StringView sv, TargetArchitecture* out) {
    assert(out != NULL);

    if (is_string_view_empty(sv)) {
        return false;
    }

    if (string_view_eq_sv(sv, STR_LIT("x86_64")) || string_view_eq_sv(sv, STR_LIT("x64"))) {
        *out = TARGET_ARCH_X86_64;
        return true;
    }

    return false;
}