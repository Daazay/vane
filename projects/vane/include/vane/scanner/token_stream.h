#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/vector.h"

#include "vane/scanner/token_kind.h"
#include "vane/scanner/scanner.h"

#include "vane/diagnostic/report_collector.h"

typedef struct TokenStream TokenStream;

#define TOKEN_STREAM_DEFAULT_CAPACITY 5120
#define TOKEN_STREAM_WINDOW_SIZE 2

struct TokenStream {
    Scanner scanner;
    Vector tokens;
    i32 idx;
    bool done;
};

TokenStream token_stream_create(u32 init_cap, StringView path, StringView content, ReportCollector* rc);

void token_stream_destroy(TokenStream* ts);

bool is_token_stream_end(const TokenStream* ts);

void token_stream_move_forward(TokenStream* ts);

void token_stream_move_back(TokenStream* ts);

const Token* token_stream_get_curr(TokenStream* ts);

const Token* token_stream_peek_next(TokenStream* ts);

const Token* token_stream_advance(TokenStream* ts);