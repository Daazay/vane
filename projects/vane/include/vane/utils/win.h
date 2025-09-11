#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string_view.h"
#include "vane/utils/string.h"

#if defined(PLATFORM_WINDOWS)

bool win_utf8_to_utf16_alloc(StringView u8_sv, u16** out_w, u64* out_len);

String win_utf16_to_utf8_str(const u16* wbuf, u64 wlen);

#endif