#include "vane/scanner/token_stream.h"

#define TOKEN_STREAM_BEGIN_IDX -1

static inline  void token_stream_parse_next(TokenStream* ts) {
    assert(ts != NULL);

    if (!ts->done) {
        Token token = scanner_scan_next(&ts->scanner);
        vector_push_back(&ts->tokens, &token);

        if (token.kind == TOKEN_EOF_) {
            ts->done = true;
        }
    }
}

static inline void token_stream_parse_window(TokenStream* ts, u32 window_size) {
    assert(ts != NULL);

    while (!ts->done && ts->idx + window_size >= ts->tokens.size) {
        token_stream_parse_next(ts);
    }
}


TokenStream token_stream_create(u32 init_cap, StringView path, StringView content) {
    u32 cap = (init_cap > 0)
        ? init_cap
        : TOKEN_STREAM_DEFAULT_CAPACITY;

    return (TokenStream) {
        .scanner = scanner_create(path, content),
        .tokens = vector_create(cap, VECTOR_SPECS(Token, NULL)),
        .idx = TOKEN_STREAM_BEGIN_IDX,
        .done = false,
    };
}

void token_stream_destroy(TokenStream* ts) {
    if (ts == NULL) {
        return;
    }

    vector_destroy(&ts->tokens);
}

bool is_token_stream_end(const TokenStream* ts) {
    assert(ts != NULL);

    if (ts->done && (ts->idx + TOKEN_STREAM_WINDOW_SIZE >= (i32)ts->tokens.size)) {
        return true;
    }
    return false;
}

void token_stream_move_forward(TokenStream* ts) {
    assert(ts != NULL);

    if (ts->idx + TOKEN_STREAM_WINDOW_SIZE + 1 >= (i32)ts->tokens.size) {
        token_stream_parse_window(ts, TOKEN_STREAM_WINDOW_SIZE + 1);
    }

    assert((ts->idx < (i32)ts->tokens.size) && "the end of token stream reached");
    ts->idx++;
}

void token_stream_move_back(TokenStream* ts) {
    assert(ts != NULL);

    assert(ts->idx != TOKEN_STREAM_BEGIN_IDX && "index of token stream must be >= -1");
    ts->idx--;
}

const Token* token_stream_get_curr(TokenStream* ts) {
    assert(ts != NULL);

    if (ts->idx + TOKEN_STREAM_WINDOW_SIZE >= (i32)ts->tokens.size) {
        token_stream_parse_window(ts, TOKEN_STREAM_WINDOW_SIZE);
    }

    if (ts->idx < 0) {
        return NULL;
    }

    return vector_at(ts->tokens, ts->idx);
}

const Token* token_stream_peek_next(TokenStream* ts) {
    assert(ts != NULL);

    if (ts->idx + TOKEN_STREAM_WINDOW_SIZE + 1 >= (i32)ts->tokens.size) {
        token_stream_parse_window(ts, TOKEN_STREAM_WINDOW_SIZE + 1);
    }

    if (ts->idx + 1 == (i32)ts->tokens.size && ts->done) {
        return NULL;
    }

    return vector_at(ts->tokens, ts->idx + 1);
}

const Token* token_stream_advance(TokenStream* ts) {
    assert(ts != NULL);

    const Token* token = token_stream_peek_next(ts);
    token_stream_move_forward(ts);

    return token;
}