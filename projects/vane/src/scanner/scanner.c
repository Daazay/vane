#include "vane/scanner/scanner.h"

#include "vane/utils/string_utils.h"

#define EOF_CHAR '\0'

static inline char scanner_curr_char(const Scanner* scanner) {
    if (scanner->pos >= scanner->content.len) {
        return EOF_CHAR;
    }
    return scanner->content.data[scanner->pos];
}

static inline void scanner_advance_char(Scanner* scanner) {
    char prev_ch = scanner_curr_char(scanner);
    if (prev_ch == '\r' || prev_ch == '\n') {
        scanner->loc.end.column = SCANNER_DEFAULT_COLUMN_POS;
        scanner->loc.end.line++;

        // Set flags for next token
        SET_FLAG(scanner->flags, TOKEN_FLAG_FIRST_IN_LINE);
        SET_FLAG(scanner->flags, TOKEN_FLAG_HAS_LEADING_LBR);
    }
    else {
        scanner->loc.end.column++;
    }

    scanner->pos++;

    // Handle CRLF sequence
    char ch = scanner_curr_char(scanner);
    if (prev_ch == '\r' && ch == '\n') {
        scanner->pos++;
    }
}

#define RETURN_TOKEN(KIND, VALUE) do { \
    TokenFlags token_flags = scanner->flags; \
    char next_ch = scanner_curr_char(scanner); \
    if (is_hspace(next_ch)) { \
        SET_FLAG(token_flags, TOKEN_FLAG_HAS_TRAILING_WS); \
    } \
    else if (is_vspace(next_ch)) { \
        SET_FLAG(token_flags, TOKEN_FLAG_HAS_TRAILING_LBR); \
    } \
    Token token = token_create(KIND, token_flags, VALUE, scanner->loc); \
    CLEAR_FLAG(scanner->flags, TOKEN_FLAG_FIRST_IN_LINE); \
    return token; \
} while (false)

static inline void scanner_skip_whitespaces(Scanner* scanner) {
    bool saw_hspace = false;
    bool saw_vspace = false;

    char ch = EOF_CHAR;
    while ((ch = scanner_curr_char(scanner)) != EOF_CHAR) {
        if (is_hspace(ch)) {
            saw_hspace = true;
            scanner_advance_char(scanner);
            continue;
        }
        else if (is_vspace(ch)) {
            saw_vspace = true;
            scanner_advance_char(scanner);
            continue;
        }
        else {
            break;
        }
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
        //CLEAR_FLAG(scanner->flags, TOKEN_FLAG_FIRST_IN_LINE);
    }
}

static inline void scanner_skip_comment_line(Scanner* scanner) {
    char ch = EOF_CHAR;
    while ((ch = scanner_curr_char(scanner)) != EOF_CHAR) {
        if (is_vspace(ch)) {
            break;
        }
        scanner_advance_char(scanner);
    }
}

static inline bool scanner_skip_comment_block(Scanner* scanner) {
    char ch = EOF_CHAR;
    char prev_ch = EOF_CHAR;

    i32 nesting_level = 1;

    while (nesting_level > 0 && (ch = scanner_curr_char(scanner)) != EOF_CHAR) {
        // nested comment block
        if (prev_ch == '/' && ch == '*') {
            nesting_level++;
            prev_ch = EOF_CHAR;
            scanner_advance_char(scanner);
        }
        else if (prev_ch == '*' && ch == '/') {
            nesting_level--;
            prev_ch = EOF_CHAR;
            scanner_advance_char(scanner);
        }
        else {
            prev_ch = ch;
            scanner_advance_char(scanner);
        }
    }

    return nesting_level == 0;
}

static inline Token scanner_parse_string_literal(Scanner* scanner) {
    char ch = EOF_CHAR;
    char prev_ch = EOF_CHAR;

    u64 prev_pos = scanner->pos;
    bool good = false;

    while ((ch = scanner_curr_char(scanner)) != EOF_CHAR) {
        if (prev_ch != '\\' && ch == '\"') {
            good = true;
            scanner_advance_char(scanner);
            break;
        }
        else if (is_vspace(ch)) {
            good = false;
            break;
        }
        scanner_advance_char(scanner);
        prev_ch = ch;
    }

    if (!good) {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    const u64 len = scanner->pos - prev_pos - 1;
    StringView value = string_view_subview(scanner->content, prev_pos, len);
    RETURN_TOKEN(TOKEN_LITERAL_STRING, value);
}

static inline Token scanner_parse_char_literal(Scanner* scanner) {
    char ch = EOF_CHAR;
    char prev_ch = EOF_CHAR;

    u64 prev_pos = scanner->pos;
    bool good = false;

    while ((ch = scanner_curr_char(scanner)) != EOF_CHAR) {
        if (prev_ch != '\\' && ch == '\'') {
            good = true;
            scanner_advance_char(scanner);
            break;
        }
        else if (is_vspace(ch)) {
            good = false;
            break;
        }
        scanner_advance_char(scanner);
        prev_ch = ch;
    }

    if (!good) {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    const u64 len = scanner->pos - prev_pos - 1;
    if (len < 1) {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }
    else if ((scanner->content.data[prev_pos] == '\\' && len > 2) ||
             (scanner->content.data[prev_pos] != '\\' && len > 1)) {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    StringView value = string_view_subview(scanner->content, prev_pos, len);
    RETURN_TOKEN(TOKEN_LITERAL_CHAR, value);
}

static inline Token scanner_parse_prefixed_number_literal(Scanner* scanner, TokenKind kind, bool (*is_valid_char_fn)(char c)) {
    char ch = EOF_CHAR;
    char prev_ch = EOF_CHAR;

    u64 prev_pos = scanner->pos - 2;
    bool good = false;

    while ((ch = scanner_curr_char(scanner)) != EOF_CHAR) {
        if (is_space(ch) || (is_punc(ch) && ch != '_')) {
            break;
        }
        else if (ch == '_') {
            if (prev_ch == '_') {
                good = false;
            }
        }
        else if (!is_valid_char_fn(ch)) {
            good = false;
        }
        scanner_advance_char(scanner);
        prev_ch = ch;
    }

    const u64 len = scanner->pos - prev_pos;
    if (len == 2) {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }
    else if (!good || prev_ch == '_') {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    StringView value = string_view_subview(scanner->content, prev_pos, len);
    RETURN_TOKEN(kind, value);
}

static inline Token scanner_parse_dec_literal(Scanner* scanner) {
    char ch = EOF_CHAR;
    char prev_ch = EOF_CHAR;

    bool good = true;
    const u64 prev_pos = scanner->pos - 1;

    while ((ch = scanner_curr_char(scanner)) != EOF_CHAR) {
        if (is_space(ch) || (is_punc(ch) && ch != '_')) {
            break;
        }
        else if (ch == '_') {
            if (prev_ch == '_') {
                good = false;
            }
        }
        else if (!is_digit(ch)) {
            good = false;
        }
        scanner_advance_char(scanner);
        prev_ch = ch;
    }

    const u64 len = scanner->pos - prev_pos;

    if (!good || prev_ch == '_') {
        RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
    }

    StringView value = string_view_subview(scanner->content, prev_pos, len);
    RETURN_TOKEN(TOKEN_LITERAL_DEC, value);
}

static inline Token scanner_parse_number_literal(Scanner* scanner) {
    char ch = scanner_curr_char(scanner);
    scanner_advance_char(scanner);

    if (ch == '0') {
        ch = scanner_curr_char(scanner);

        // HEX LITERAL
        if (ch == 'x' || ch == 'X') {
            scanner_advance_char(scanner);
            return scanner_parse_prefixed_number_literal(scanner, TOKEN_LITERAL_HEX, &is_xdigit);
        }
        // OCT LITERAL
        else if (ch == 'o' || ch == 'O') {
            scanner_advance_char(scanner);
            return scanner_parse_prefixed_number_literal(scanner, TOKEN_LITERAL_OCT, &is_odigit);
        }
        // BIN LITERAL
        else if (ch == 'b' || ch == 'B') {
            scanner_advance_char(scanner);
            return scanner_parse_prefixed_number_literal(scanner, TOKEN_LITERAL_BIN, &is_bdigit);
        }
    }

    return scanner_parse_dec_literal(scanner);
}

static inline u32 keyword_hash(StringView value) {
    if (value.len < 2) {
        if (value.len > 0) {
            return value.data[0];
        }
        return 0;
    }
    return ((u32)value.data[0] << 24) | ((u32)value.data[1] << 16) | ((u32)value.data[value.len - 1] << 8) | ((u32)value.len & 0xFF);
}

static inline TokenKind keyword_or_identifier(StringView value) {
    u32 hash = keyword_hash(value);

    #define KWD_OR_ID_CASE(VALUE, C1, C2, C3, KIND) \
    case ((u32)C1 << 24) | ((u32)C2 << 16) | ((u32)C3 << 8): \
    if (string_view_eq_sv(value, STR_LIT(VALUE))) { \
        return KIND; \
    } \
    break

    switch (hash) {
    KWD_OR_ID_CASE("true",     't', 'r', 'e', TOKEN_LITERAL_BOOL);
    KWD_OR_ID_CASE("false",    'f', 'a', 'e', TOKEN_LITERAL_BOOL);

    KWD_OR_ID_CASE("import",   'i', 'm', 't', TOKEN_KEYWORD_IMPORT);
    KWD_OR_ID_CASE("private",  'p', 'r', 'e', TOKEN_KEYWORD_PRIVATE);
    KWD_OR_ID_CASE("public",   'p', 'u', 'c', TOKEN_KEYWORD_PUBLIC);
    KWD_OR_ID_CASE("class",    'c', 'l', 's', TOKEN_KEYWORD_CLASS);
    KWD_OR_ID_CASE("as",       'a', 's', 's', TOKEN_KEYWORD_AS);
    KWD_OR_ID_CASE("type",     't', 'y', 'e', TOKEN_KEYWORD_TYPE);
    KWD_OR_ID_CASE("fun",      'f', 'u', 'n', TOKEN_KEYWORD_FUN);
    KWD_OR_ID_CASE("begin",    'b', 'e', 'n', TOKEN_KEYWORD_BEGIN);
    KWD_OR_ID_CASE("end",      'e', 'n', 'd', TOKEN_KEYWORD_END);
    KWD_OR_ID_CASE("const",    'c', 'o', 't', TOKEN_KEYWORD_CONST);
    KWD_OR_ID_CASE("var",      'v', 'a', 'r', TOKEN_KEYWORD_VAR);
    KWD_OR_ID_CASE("if",       'i', 'f', 'f', TOKEN_KEYWORD_IF);
    KWD_OR_ID_CASE("then",     't', 'h', 'n', TOKEN_KEYWORD_THEN);
    KWD_OR_ID_CASE("else",     'e', 'l', 'e', TOKEN_KEYWORD_ELSE);
    KWD_OR_ID_CASE("while",    'w', 'h', 'e', TOKEN_KEYWORD_WHILE);
    KWD_OR_ID_CASE("do",       'd', 'o', 'o', TOKEN_KEYWORD_DO);
    KWD_OR_ID_CASE("loop",     'l', 'o', 'p', TOKEN_KEYWORD_LOOP);
    KWD_OR_ID_CASE("until",    'u', 'n', 'l', TOKEN_KEYWORD_UNTIL);
    KWD_OR_ID_CASE("break",    'b', 'r', 'k', TOKEN_KEYWORD_BREAK);
    KWD_OR_ID_CASE("continue", 'c', 'o', 'e', TOKEN_KEYWORD_CONTINUE);
    KWD_OR_ID_CASE("return",   'r', 'e', 'n', TOKEN_KEYWORD_RETURN);
    KWD_OR_ID_CASE("defer",    'd', 'e', 'r', TOKEN_KEYWORD_DEFER);
    default:
        break;
    }

    return TOKEN_IDENTIFIER;
}

static inline Token scanner_parse_identifier_or_keyword(Scanner* scanner) {
    char ch = EOF_CHAR;
    const u64 prev_pos = scanner->pos;

    while ((ch = scanner_curr_char(scanner)) != EOF_CHAR) {
        if (!is_alnum_(ch)) {
            break;
        }
        scanner_advance_char(scanner);
    }

    StringView value = string_view_subview(scanner->content, prev_pos, scanner->pos - prev_pos);
    TokenKind kind = keyword_or_identifier(value);

    RETURN_TOKEN(kind, value);
}

Scanner scanner_create(StringView path, StringView content) {
    return (Scanner) {
        .loc = {
            .path = path,
            .begin = { .line = SCANNER_DEFAULT_LINE_POS, .column = SCANNER_DEFAULT_COLUMN_POS },
            .end   = { .line = SCANNER_DEFAULT_LINE_POS, .column = SCANNER_DEFAULT_COLUMN_POS },
        },
        .content = content,
        .pos = 0,
        .flags = TOKEN_FLAG_FIRST_IN_LINE,
    };
}

#define CASE_C1(CHAR, KIND) case CHAR: { \
    scanner_advance_char(scanner); \
    RETURN_TOKEN(KIND, STRING_VIEW_EMPTY); \
} break

#define CASE_C1C1(CHAR1, CHAR2, KIND1, KIND2) case CHAR1: { \
    scanner_advance_char(scanner); \
    if (scanner_curr_char(scanner) == CHAR2) { \
        scanner_advance_char(scanner); \
        RETURN_TOKEN(KIND2, STRING_VIEW_EMPTY); \
    } \
    RETURN_TOKEN(KIND1, STRING_VIEW_EMPTY); \
} break

#define CASE_C1C2(CHAR1, CHAR21, CHAR22, KIND1, KIND21, KIND22) case CHAR1: { \
    scanner_advance_char(scanner); \
    if (scanner_curr_char(scanner) == CHAR21) { \
        scanner_advance_char(scanner); \
        RETURN_TOKEN(KIND21, STRING_VIEW_EMPTY); \
    } else if (scanner_curr_char(scanner) == CHAR22) { \
        scanner_advance_char(scanner); \
        RETURN_TOKEN(KIND22, STRING_VIEW_EMPTY); \
    } \
    RETURN_TOKEN(KIND1, STRING_VIEW_EMPTY); \
} break

#define CASE_C1C2C1(CHAR1, CHAR21, CHAR22, CHAR3, KIND1, KIND21, KIND22, KIND3) case CHAR1: { \
    scanner_advance_char(scanner); \
    if (scanner_curr_char(scanner) == CHAR21) { \
        scanner_advance_char(scanner); \
        RETURN_TOKEN(KIND21, STRING_VIEW_EMPTY); \
    } else if (scanner_curr_char(scanner) == CHAR22) { \
        scanner_advance_char(scanner); \
        if (scanner_curr_char(scanner) == CHAR3) { \
            scanner_advance_char(scanner); \
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

    char ch = scanner_curr_char(scanner);
    switch (ch) {
    CASE_C1('\0', TOKEN_EOF_);
    CASE_C1('(', TOKEN_L_BRACE);
    CASE_C1(')', TOKEN_R_BRACE);
    CASE_C1('{', TOKEN_L_CURLY);
    CASE_C1('}', TOKEN_R_CURLY);
    CASE_C1('[', TOKEN_L_BRACKET);
    CASE_C1(']', TOKEN_R_BRACKET);
    CASE_C1('.', TOKEN_DOT);
    CASE_C1(',', TOKEN_COMMA);
    CASE_C1(':', TOKEN_COLON);
    CASE_C1(';', TOKEN_SEMICOLON);
    CASE_C1C2('+', '=', '+', TOKEN_PLUS, TOKEN_PLUS_EQUAL, TOKEN_PLUS_PLUS);
    case '-': {
        scanner_advance_char(scanner);
        if (scanner_curr_char(scanner) == '>') {
            scanner_advance_char(scanner);
            RETURN_TOKEN(TOKEN_MINUS_GREATER, STRING_VIEW_EMPTY);
        } else if (scanner_curr_char(scanner) == '=') {
            scanner_advance_char(scanner);
            RETURN_TOKEN(TOKEN_MINUS_EQUAL, STRING_VIEW_EMPTY);
        }
        else if (scanner_curr_char(scanner) == '-') {
            scanner_advance_char(scanner);
            RETURN_TOKEN(TOKEN_MINUS_MINUS, STRING_VIEW_EMPTY);
        }
        RETURN_TOKEN(TOKEN_MINUS, STRING_VIEW_EMPTY);
    } break;
    CASE_C1C1('*', '=', TOKEN_STAR, TOKEN_STAR_EQUAL);
    case '/': {
        scanner_advance_char(scanner);
        ch = scanner_curr_char(scanner);

        switch (ch) {
        case '/':
            scanner_skip_comment_line(scanner);
            return scanner_scan_next(scanner);
        case '*':
            if (!scanner_skip_comment_block(scanner)) {
                RETURN_TOKEN(TOKEN_INVALID, STRING_VIEW_EMPTY);
            }
            return scanner_scan_next(scanner);
        case '=':
            scanner_advance_char(scanner);
            RETURN_TOKEN(TOKEN_SLASH_EQUAL, STRING_VIEW_EMPTY);
        default:
            RETURN_TOKEN(TOKEN_SLASH, STRING_VIEW_EMPTY);
        }
    } break;
    CASE_C1C1('%', '=', TOKEN_PERCENT, TOKEN_PERCENT_EQUAL);
    CASE_C1('^', TOKEN_CARET);
    CASE_C1C2('&', '=', '&', TOKEN_AMP, TOKEN_AMP_EQUAL, TOKEN_AMP_AMP);
    CASE_C1C2('|', '=', '|', TOKEN_PIPE, TOKEN_PIPE_EQUAL, TOKEN_PIPE_PIPE);
    CASE_C1('~', TOKEN_TILDE);
    CASE_C1C1('=', '=', TOKEN_EQUAL, TOKEN_EQUAL_EQUAL);
    CASE_C1C1('!', '=', TOKEN_EXCLAIM, TOKEN_EXCLAIM_EQUAL);
    CASE_C1C2C1('>', '=', '>', '=', TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_GREATER_GREATER, TOKEN_GREATER_GREATER_EQUAL);
    CASE_C1C2C1('<', '=', '<', '=', TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_LESS_LESS, TOKEN_LESS_LESS_EQUAL);
    case '"':
        scanner_advance_char(scanner);
        return scanner_parse_string_literal(scanner);
    case '\'':
        scanner_advance_char(scanner);
        return scanner_parse_char_literal(scanner);
    default:
        // NUMBER LITERALS
        if (is_digit(ch)) {
            return scanner_parse_number_literal(scanner);
        }
        // IDENTIFIER OR KEYWORD
        else if (is_alnum_(ch)) {
            return scanner_parse_identifier_or_keyword(scanner);
        }

        // UNKNOWN
        const u64 prev_pos = scanner->pos;
        while ((ch = scanner_curr_char(scanner)) != EOF_CHAR) {
            if (is_space(ch) || is_punc(ch)) {
                break;
            }
            scanner_advance_char(scanner);
        }

        StringView value = string_view_subview(scanner->content, prev_pos, scanner->pos - prev_pos);
        RETURN_TOKEN(TOKEN_UNKNOWN, value);
    }
}