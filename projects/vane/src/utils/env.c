#include "vane/utils/env.h"

#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#include "vane/utils/win.h"
#endif

String env_get_var(StringView name) {
    if (is_string_view_empty(name)) {
        return STRING_EMPTY;
    }

#if defined(PLATFORM_WINDOWS)
    u16* wname = NULL;
    u64 wlen = 0;

    if (!win_utf8_to_utf16_alloc(name, &wname, &wlen)) {
        return STRING_EMPTY;
    }

    DWORD need = GetEnvironmentVariableW((LPCWSTR)wname, NULL, 0);
    if (need == 0) {
        free(wname);
        return STRING_EMPTY;
    }

    u16* wval = malloc(sizeof(u16) * need);
    assert(wval != NULL);

    DWORD got = GetEnvironmentVariableW((LPCWSTR)wname, (LPWSTR)wval, need);
    free(wname);

    if (got == 0) {
        free(wval);
        return STRING_EMPTY;
    }

    String out = win_utf16_to_utf8_str(wval, got);
    free(wval);

    return out;
#else
    const char* v = getenv((const char*)name.data);
    if (v == NULL) {
        return STRING_EMPTY;
    }
    return string_from_cstr(v);
#endif
}