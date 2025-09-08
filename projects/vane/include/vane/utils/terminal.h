#pragma once

#include "vane/utils/defines.h"

typedef enum TerminalColor TerminalColor;
typedef enum TerminalStyle TerminalStyle;

enum TerminalColor {
    TERMINAL_COLOR_BLACK = 0,
    TERMINAL_COLOR_RED = 1,
    TERMINAL_COLOR_GREEN = 2,
    TERMINAL_COLOR_YELLOW = 3,
    TERMINAL_COLOR_BLUE = 4,
    TERMINAL_COLOR_MAGENTA = 5,
    TERMINAL_COLOR_CYAN = 6,
    TERMINAL_COLOR_WHITE = 7,
};

enum TerminalStyle {
    TERMINAL_STYLE_RESET = 0,
    TERMINAL_STYLE_BOLD = 1,
    TERMINAL_STYLE_DIM = 2,
    TERMINAL_STYLE_ITALIC = 3,
    TERMINAL_STYLE_UNDERLINE = 4,
    TERMINAL_STYLE_BLINK = 5,
};

bool is_terminal_support_colors();

void terminal_set_style(TerminalStyle style);

void terminal_set_color(TerminalColor font_color, TerminalColor bg_color);

void terminal_set_format(TerminalStyle style, TerminalColor font_color, TerminalColor bg_color);

void terminal_reset_format();