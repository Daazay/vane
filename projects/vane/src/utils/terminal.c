#include "vane/utils/terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_WINDOWS)
#include <io.h>
#include <Windows.h>

#define isatty _isatty
#define fileno _fileno

#else
#include <unistd.h>
#endif

bool is_terminal_support_colors() {
    if (!isatty(fileno(stdout))) {
        return false;
    }

#if defined(PLATFORM_WINDOWS)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    return true;
#else
    const char* term = getenv("TERM");
    if (term == NULL) {
        return false;
    }

    const char* color_terms[] = {
        "xterm", "xterm-color", "xterm-256color", "screen",
        "screen-256color", "tmux", "tmux-256color", "linux", "cygwin"
    };

    for (u32 i = 0; i < sizeof(color_terms) / sizeof(color_terms[0]); ++i) {
        if (strstr(term, color_terms[i])) {
            return true;
        }
    }

    return false;
#endif
}

void terminal_set_style(TerminalStyle style) {
    printf("\033[%dm", style);
}

void terminal_set_color(TerminalColor font_color, TerminalColor bg_color) {
    printf("\033[%d;%dm", font_color + 30, bg_color + 40);
}

void terminal_set_format(TerminalStyle style, TerminalColor font_color, TerminalColor bg_color) {
    printf("\033[%d;%d;%dm", style, font_color + 30, bg_color + 40);
}

void terminal_reset_format() {
    printf("\033[0m");
}