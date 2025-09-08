#include "vane/scanner/scanner.h"

#include "vane/utils/string_utils.h"

static inline void scanner_advance_rune(Scanner* scanner) {
    if (scanner->curr_rune == (Rune)RUNE_EOF) {
        return;
    }

    // Handle newlines
    // Handle LF
    if (scanner->curr_rune == (Rune)'\n') {
        scanner->loc.end.line++;
        scanner->loc.end.column = SCANNER_DEFAULT_COLUMN_POS;

        // Set flags for next token
        SET_FLAG(scanner->flags, TOKEN_FLAG_FIRST_IN_LINE);
        SET_FLAG(scanner->flags, TOKEN_FLAG_HAS_LEADING_LBR);
    }
    // Handle CRLF
    else if (scanner->curr_rune == (Rune)'\r') {
        u64 next_pos = scanner->pos + scanner->curr_rune_len;
        if (next_pos < scanner->content.len) {
            u32 next_len = 0;
            Rune next_rune = string_view_rune_at_byte(scanner->content, next_pos, &next_len);

            // Skip LF after CR
            if (next_rune == (Rune)'\n') {
                scanner->pos = next_pos + next_len;
                if (scanner->pos < scanner->content.len) {
                    scanner->curr_rune = string_view_rune_at_byte(scanner->content, scanner->pos, &scanner->curr_rune_len);
                }
                else {
                    scanner->curr_rune = (Rune)RUNE_EOF;
                    scanner->curr_rune_len = 0;
                }

                scanner->loc.end.line++;
                scanner->loc.end.column = SCANNER_DEFAULT_COLUMN_POS;

                // Set flags for next token
                SET_FLAG(scanner->flags, TOKEN_FLAG_FIRST_IN_LINE);
                SET_FLAG(scanner->flags, TOKEN_FLAG_HAS_LEADING_LBR);
                return;
            }
        }

        // Treat lone CR as normal char
        scanner->loc.end.column++;
    }
    else {
        // Normal char
        scanner->loc.end.column++;
    }

    // Advance position
    scanner->pos += scanner->curr_rune_len;

    if (scanner->pos >= scanner->content.len) {
        scanner->curr_rune = (Rune)RUNE_EOF;
        scanner->curr_rune_len = 0;
    }
    else {
        scanner->curr_rune = string_view_rune_at_byte(scanner->content, scanner->pos, &scanner->curr_rune_len);
    }
}

#define RETURN_TOKEN(KIND, VALUE) do { \
    TokenFlags token_flags = scanner->flags; \
    if (is_hspace_rune(scanner->curr_rune)) { \
        SET_FLAG(token_flags, TOKEN_FLAG_HAS_TRAILING_WS); \
    } \
    else if (is_vspace_rune(scanner->curr_rune)) { \
        SET_FLAG(token_flags, TOKEN_FLAG_HAS_TRAILING_LBR); \
    } \
    Token token = token_create(KIND, token_flags, VALUE, scanner->loc); \
    CLEAR_FLAG(scanner->flags, TOKEN_FLAG_FIRST_IN_LINE); \
    return token; \
} while (false)

static inline void scanner_skip_whitespaces(Scanner* scanner) {
    bool saw_hspace = false;
    bool saw_vspace = false;

    while (scanner->curr_rune != RUNE_EOF) {
        if (is_hspace_rune(scanner->curr_rune)) {
            saw_hspace = true;
            scanner_advance_rune(scanner);
            continue;
        }
        else if (is_vspace_rune(scanner->curr_rune)) {
            saw_vspace = true;
            scanner_advance_rune(scanner);
            continue;
        }
        break;
    }

    if (saw_hspace) {
        SET_FLAG(scanner->flags, TOKEN_FLAG_HAS_LEADING_WS);
    }
    else {
        CLEAR_FLAG(scanner->flags, TOKEN_FLAG_HAS_LEADING_WS);
    }

    if (saw_vspace) {
        SET_FLAG(scanner->flags, TOKEN_FLAG_HAS_LEADING_LBR);
        SET_FLAG(scanner->flags, TOKEN_FLAG_FIRST_IN_LINE);
    }
    else {
        CLEAR_FLAG(scanner->flags, TOKEN_FLAG_HAS_LEADING_LBR);
    }
}

static inline void scanner_skip_comment_line(Scanner* scanner) {
    while (scanner->curr_rune != RUNE_EOF) {
        if (is_vspace_rune(scanner->curr_rune)) {
            break;
        }
        scanner_advance_rune(scanner);
    }
}

static inline bool scanner_skip_comment_block(Scanner* scanner) {
    Rune prev_r = RUNE_EOF;

    i32 nesting_level = 1;

    while (nesting_level > 0 && scanner->curr_rune != RUNE_EOF) {
        // nested comment block
        if (prev_r == (Rune)'/' && scanner->curr_rune == (Rune)'*') {
            nesting_level++;
            prev_r = RUNE_EOF;
            scanner_advance_rune(scanner);
        }
        else if (prev_r == (Rune)'*' && scanner->curr_rune == (Rune)'/') {
            nesting_level--;
            prev_r = RUNE_EOF;
            scanner_advance_rune(scanner);
        }
        else {
            prev_r = scanner->curr_rune;
            scanner_advance_rune(scanner);
        }
    }

    return nesting_level == 0;
}

static inline Token scanner_parse_string_literal(Scanner* scanner) {
    Rune prev_r = RUNE_EOF;

    u64 prev_pos = scanner->pos;
    bool good = false;

    while (scanner->curr_rune != RUNE_EOF) {
        if (prev_r != (Rune)'\\' && scanner->curr_rune == (Rune)'\"') {
            good = true;
            scanner_advance_rune(scanner);
            break;
        }
        else if (is_vspace_rune(scanner->curr_rune)) {
            good = false;
            break;
        }
        scanner_advance_rune(scanner);
        prev_r = scanner->curr_rune;
    }

    if (!good) {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    StringView value = string_view_subview(scanner->content, prev_pos, scanner->pos - prev_pos - 1);
    RETURN_TOKEN(TOKEN_LITERAL_STRING, value);
}

static inline Token scanner_parse_char_literal(Scanner* scanner) {
    Rune prev_r = RUNE_EOF;

    u64 prev_pos = scanner->pos;
    u64 rune_count = 0;
    bool good = false;

    while (scanner->curr_rune != RUNE_EOF) {
        rune_count++;

        if (prev_r != (Rune)'\\' && scanner->curr_rune == (Rune)'\'') {
            good = true;
            scanner_advance_rune(scanner);
            break;
        }
        else if (is_vspace_rune(scanner->curr_rune)) {
            good = false;
            break;
        }
        scanner_advance_rune(scanner);
        prev_r = scanner->curr_rune;
    }

    if (!good) {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    if (prev_r == (Rune)'\\') {
        // backslash + escaped char
        if (rune_count != 2) {
            RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
        }
    }
    else {
        if (rune_count != 1) {
            RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
        }
    }

    StringView value = string_view_subview(scanner->content, prev_pos, scanner->pos - prev_pos - 1);
    RETURN_TOKEN(TOKEN_LITERAL_CHAR, value);
}

static inline Token scanner_parse_prefixed_number_literal(Scanner* scanner, TokenKind kind, bool (*is_valid_rune_fn)(Rune r)) {
    Rune prev_r = RUNE_EOF;

    u64 prev_pos = scanner->pos;
    bool good = false;

    while (scanner->curr_rune != RUNE_EOF) {
        if (is_space_rune(scanner->curr_rune) ||
            (is_punc_rune(scanner->curr_rune) && (scanner->curr_rune != (Rune)'_'))) {
            break;
        }
        else if (scanner->curr_rune == (Rune)'_') {
            if (prev_r == (Rune)'_') {
                good = false;
            }
        }
        else if (!is_valid_rune_fn(scanner->curr_rune)) {
            good = false;
        }

        scanner_advance_rune(scanner);
        prev_r = scanner->curr_rune;
    }

    if (scanner->pos == prev_pos) {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }
    else if (!good || prev_r == (Rune)'_') {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    StringView value = string_view_subview(scanner->content, prev_pos, scanner->pos - prev_pos);
    RETURN_TOKEN(kind, value);
}

static inline Token scanner_parse_dec_literal(Scanner* scanner) {
    Rune prev_r = RUNE_EOF;

    bool good = true;
    const u64 prev_pos = scanner->pos - 1;

    while (scanner->curr_rune != RUNE_EOF) {
        if (is_space_rune(scanner->curr_rune) ||
            (is_punc_rune(scanner->curr_rune) && (scanner->curr_rune != (Rune)'_'))) {
            break;
        }
        else if (scanner->curr_rune == (Rune)'_') {
            if (prev_r == (Rune)'_') {
                good = false;
            }
        }
        else if (!is_digit_rune(scanner->curr_rune)) {
            good = false;
        }
        scanner_advance_rune(scanner);
        prev_r = scanner->curr_rune;
    }

    if (!good || prev_r == (Rune)'_') {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    StringView value = string_view_subview(scanner->content, prev_pos, scanner->pos - prev_pos);
    RETURN_TOKEN(TOKEN_LITERAL_DEC, value);
}

static inline Token scanner_parse_number_literal(Scanner* scanner) {
    Rune curr_r = scanner->curr_rune;
    scanner_advance_rune(scanner);

    if (curr_r == '0') {
        curr_r = scanner->curr_rune;

        // HEX LITERAL
        if (curr_r == (Rune)'x' || curr_r == (Rune)'X') {
            scanner_advance_rune(scanner);
            return scanner_parse_prefixed_number_literal(scanner, TOKEN_LITERAL_HEX, &is_xdigit_rune);
        }
        // OCT LITERAL
        if (curr_r == (Rune)'o' || curr_r == (Rune)'O') {
            scanner_advance_rune(scanner);
            return scanner_parse_prefixed_number_literal(scanner, TOKEN_LITERAL_OCT, &is_odigit_rune);
        }
        // BIN LITERAL
        if (curr_r == (Rune)'b' || curr_r == (Rune)'B') {
            scanner_advance_rune(scanner);
            return scanner_parse_prefixed_number_literal(scanner, TOKEN_LITERAL_BIN, &is_bdigit_rune);
        }
    }

    return scanner_parse_dec_literal(scanner);
}

static inline u32 keyword_hash_runes(StringView value) {
    if (value.len == 0) {
        return 0;
    }

    u64 pos = 0;
    u32 rune_len = 0;
    u32 hash = 0;

    for (u64 i = 0; i < 3 && pos < value.len; ++i) {
        Rune r = string_view_rune_at_byte(value, pos, &rune_len);
        pos += rune_len;
        hash |= (r << 8 * (3 - i));
    }

    hash |= value.len & 0xFF;
    return hash;
}

static inline TokenKind keyword_or_identifier(StringView value) {
    u32 hash = keyword_hash_runes(value);

    #define KWD_OR_ID_CASE(VALUE, C1, C2, C3, KIND) \
    case ((u32)C1 << 24) | ((u32)C2 << 16) | ((u32)C3 << 8) | ((u32)(ARR_SIZE(VALUE) - 1) & 0xFF): \
    if (string_view_eq_sv(value, STR_LIT(VALUE))) { \
        return KIND; \
    } \
    break

    switch (hash) {
    KWD_OR_ID_CASE("true",     't', 'r', 'u', TOKEN_LITERAL_BOOL);
    KWD_OR_ID_CASE("false",    'f', 'a', 'l', TOKEN_LITERAL_BOOL);

    KWD_OR_ID_CASE("import",   'i', 'm', 'p', TOKEN_KEYWORD_IMPORT);
    KWD_OR_ID_CASE("private",  'p', 'r', 'i', TOKEN_KEYWORD_PRIVATE);
    KWD_OR_ID_CASE("public",   'p', 'u', 'b', TOKEN_KEYWORD_PUBLIC);
    KWD_OR_ID_CASE("class",    'c', 'l', 'a', TOKEN_KEYWORD_CLASS);
    KWD_OR_ID_CASE("as",       'a', 's', 's', TOKEN_KEYWORD_AS);
    KWD_OR_ID_CASE("type",     't', 'y', 'p', TOKEN_KEYWORD_TYPE);
    KWD_OR_ID_CASE("fun",      'f', 'u', 'n', TOKEN_KEYWORD_FUN);
    KWD_OR_ID_CASE("begin",    'b', 'e', 'g', TOKEN_KEYWORD_BEGIN);
    KWD_OR_ID_CASE("end",      'e', 'n', 'd', TOKEN_KEYWORD_END);
    KWD_OR_ID_CASE("const",    'c', 'o', 'n', TOKEN_KEYWORD_CONST);
    KWD_OR_ID_CASE("var",      'v', 'a', 'r', TOKEN_KEYWORD_VAR);
    KWD_OR_ID_CASE("if",       'i', 'f', 'f', TOKEN_KEYWORD_IF);
    KWD_OR_ID_CASE("then",     't', 'h', 'e', TOKEN_KEYWORD_THEN);
    KWD_OR_ID_CASE("else",     'e', 'l', 's', TOKEN_KEYWORD_ELSE);
    KWD_OR_ID_CASE("while",    'w', 'h', 'i', TOKEN_KEYWORD_WHILE);
    KWD_OR_ID_CASE("do",       'd', 'o', 'o', TOKEN_KEYWORD_DO);
    KWD_OR_ID_CASE("loop",     'l', 'o', 'o', TOKEN_KEYWORD_LOOP);
    KWD_OR_ID_CASE("until",    'u', 'n', 't', TOKEN_KEYWORD_UNTIL);
    KWD_OR_ID_CASE("break",    'b', 'r', 'e', TOKEN_KEYWORD_BREAK);
    KWD_OR_ID_CASE("continue", 'c', 'o', 'n', TOKEN_KEYWORD_CONTINUE);
    KWD_OR_ID_CASE("return",   'r', 'e', 't', TOKEN_KEYWORD_RETURN);
    KWD_OR_ID_CASE("defer",    'd', 'e', 'd', TOKEN_KEYWORD_DEFER);
    default:
        break;
    }

    return TOKEN_IDENTIFIER;
}

static inline Token scanner_parse_identifier_or_keyword(Scanner* scanner) {
    const u64 prev_pos = scanner->pos;

    while (scanner->curr_rune != RUNE_EOF) {
        if (!is_alnum_rune_(scanner->curr_rune)) {
            break;
        }
        scanner_advance_rune(scanner);
    }

    StringView value = string_view_subview(scanner->content, prev_pos, scanner->pos - prev_pos);
    TokenKind kind = keyword_or_identifier(value);

    RETURN_TOKEN(kind, value);
}

Scanner scanner_create(StringView path, StringView content) {
    Scanner scanner = {0};

    scanner.content = content;
    scanner.pos = 0;

    scanner.loc.path = path;
    scanner.loc.begin.line = SCANNER_DEFAULT_LINE_POS;
    scanner.loc.begin.column = SCANNER_DEFAULT_COLUMN_POS;

    scanner.flags = TOKEN_FLAG_FIRST_IN_LINE;

    if (!is_string_view_empty(content)) {
        scanner.curr_rune = string_view_rune_at_byte(content, scanner.pos, &scanner.curr_rune_len);
    }
    else {
        scanner.curr_rune = (Rune)RUNE_EOF;
        scanner.curr_rune_len = 0;
    }

    return scanner;
}

#define CASE_R1(RUNE, KIND) case RUNE: { \
    scanner_advance_rune(scanner); \
    RETURN_TOKEN(KIND, STRING_VIEW_EMPTY); \
} break

#define CASE_R1R1(RUNE1, RUNE2, KIND1, KIND2) case RUNE1: { \
    scanner_advance_rune(scanner); \
    if (scanner->curr_rune == (Rune)RUNE2) { \
        scanner_advance_rune(scanner); \
        RETURN_TOKEN(KIND2, STRING_VIEW_EMPTY); \
    } \
    RETURN_TOKEN(KIND1, STRING_VIEW_EMPTY); \
} break

#define CASE_R1R2(RUNE1, RUNE21, RUNE22, KIND1, KIND21, KIND22) case RUNE1: { \
    scanner_advance_rune(scanner); \
    if (scanner->curr_rune == (Rune)RUNE21) { \
        scanner_advance_rune(scanner); \
        RETURN_TOKEN(KIND21, STRING_VIEW_EMPTY); \
    } else if (scanner->curr_rune == (Rune)RUNE22) { \
        scanner_advance_rune(scanner); \
        RETURN_TOKEN(KIND22, STRING_VIEW_EMPTY); \
    } \
    RETURN_TOKEN(KIND1, STRING_VIEW_EMPTY); \
} break

#define CASE_R1R2R1(RUNE1, RUNE21, RUNE22, RUNE3, KIND1, KIND21, KIND22, KIND3) case RUNE1: { \
    scanner_advance_rune(scanner); \
    if (scanner->curr_rune == (Rune)RUNE21) { \
        scanner_advance_rune(scanner); \
        RETURN_TOKEN(KIND21, STRING_VIEW_EMPTY); \
    } else if (scanner->curr_rune == (Rune)RUNE22) { \
        scanner_advance_rune(scanner); \
        if (scanner->curr_rune == (Rune)RUNE3) { \
            scanner_advance_rune(scanner); \
            RETURN_TOKEN(KIND3, STRING_VIEW_EMPTY); \
        } \
        RETURN_TOKEN(KIND22, STRING_VIEW_EMPTY); \
    } \
    RETURN_TOKEN(KIND1, STRING_VIEW_EMPTY); \
} break

Token scanner_scan_next(Scanner* scanner) {
    assert(scanner != NULL);

    scanner_skip_whitespaces(scanner);
    scanner->loc.begin = scanner->loc.end;

    switch (scanner->curr_rune) {
    CASE_R1(-1, TOKEN_EOF_);
    CASE_R1('(', TOKEN_L_BRACE);
    CASE_R1(')', TOKEN_R_BRACE);
    CASE_R1('{', TOKEN_L_CURLY);
    CASE_R1('}', TOKEN_R_CURLY);
    CASE_R1('[', TOKEN_L_BRACKET);
    CASE_R1(']', TOKEN_R_BRACKET);
    CASE_R1('.', TOKEN_DOT);
    CASE_R1(',', TOKEN_COMMA);
    CASE_R1(':', TOKEN_COLON);
    CASE_R1(';', TOKEN_SEMICOLON);
    CASE_R1R2('+', '=', '+', TOKEN_PLUS, TOKEN_PLUS_EQUAL, TOKEN_PLUS_PLUS);
    case '-': {
        scanner_advance_rune(scanner);
        if (scanner->curr_rune == (Rune)'>') {
            scanner_advance_rune(scanner);
            RETURN_TOKEN(TOKEN_MINUS_GREATER, STRING_VIEW_EMPTY);
        } else if (scanner->curr_rune == (Rune)'=') {
            scanner_advance_rune(scanner);
            RETURN_TOKEN(TOKEN_MINUS_EQUAL, STRING_VIEW_EMPTY);
        }
        else if (scanner->curr_rune == (Rune)'-') {
            scanner_advance_rune(scanner);
            RETURN_TOKEN(TOKEN_MINUS_MINUS, STRING_VIEW_EMPTY);
        }
        RETURN_TOKEN(TOKEN_MINUS, STRING_VIEW_EMPTY);
    } break;
    CASE_R1R1('*', '=', TOKEN_STAR, TOKEN_STAR_EQUAL);
    case '/': {
        scanner_advance_rune(scanner);
        switch (scanner->curr_rune) {
        case '/':
            scanner_skip_comment_line(scanner);
            return scanner_scan_next(scanner);
        case '*':
            if (!scanner_skip_comment_block(scanner)) {
                RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
            }
            return scanner_scan_next(scanner);
        case '=':
            scanner_advance_rune(scanner);
            RETURN_TOKEN(TOKEN_SLASH_EQUAL, STRING_VIEW_EMPTY);
        default:
            RETURN_TOKEN(TOKEN_SLASH, STRING_VIEW_EMPTY);
        }
    } break;
    CASE_R1R1('%', '=', TOKEN_PERCENT, TOKEN_PERCENT_EQUAL);
    CASE_R1('^', TOKEN_CARET);
    CASE_R1R2('&', '=', '&', TOKEN_AMP, TOKEN_AMP_EQUAL, TOKEN_AMP_AMP);
    CASE_R1R2('|', '=', '|', TOKEN_PIPE, TOKEN_PIPE_EQUAL, TOKEN_PIPE_PIPE);
    CASE_R1('~', TOKEN_TILDE);
    CASE_R1R1('=', '=', TOKEN_EQUAL, TOKEN_EQUAL_EQUAL);
    CASE_R1R1('!', '=', TOKEN_EXCLAIM, TOKEN_EXCLAIM_EQUAL);
    CASE_R1R2R1('>', '=', '>', '=', TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_GREATER_GREATER, TOKEN_GREATER_GREATER_EQUAL);
    CASE_R1R2R1('<', '=', '<', '=', TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_LESS_LESS, TOKEN_LESS_LESS_EQUAL);
    case '"':
        scanner_advance_rune(scanner);
        return scanner_parse_string_literal(scanner);
    case '\'':
        scanner_advance_rune(scanner);
        return scanner_parse_char_literal(scanner);
    default:
        // NUMBER LITERALS
        if (is_digit_rune(scanner->curr_rune)) {
            return scanner_parse_number_literal(scanner);
        }
        // IDENTIFIER OR KEYWORD
        else if (is_alpha_rune(scanner->curr_rune) || (scanner->curr_rune == (Rune)'_')) {
            return scanner_parse_identifier_or_keyword(scanner);
        }

        // UNKNOWN
        const u64 prev_pos = scanner->pos;
        while (scanner->curr_rune != RUNE_EOF) {
            if (is_space_rune(scanner->curr_rune) || is_punc_rune(scanner->curr_rune)) {
                break;
            }
            scanner_advance_rune(scanner);
        }

        StringView value = string_view_subview(scanner->content, prev_pos, scanner->pos - prev_pos);
        RETURN_TOKEN(TOKEN_UNKNOWN, value);
    }
}