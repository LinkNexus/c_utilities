#ifndef C_UTILS_JSON_LIB_IMPLEMENTATION
#define C_UTILS_JSON_LIB_IMPLEMENTATION

#include "arena.h"
#include "darr.h"
#include "dstr.h"
#include "hashmap.h"
#include "strv.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } JsonType;

typedef enum {
  JSON_TOKEN_STRING, // 0
  JSON_TOKEN_NUMBER,
  JSON_TOKEN_TRUE,
  JSON_TOKEN_FALSE,
  JSON_TOKEN_NULL,
  JSON_TOKEN_LBRACE,
  JSON_TOKEN_RBRACE,
  JSON_TOKEN_LBRACKET,
  JSON_TOKEN_RBRACKET,
  JSON_TOKEN_COLON,
  JSON_TOKEN_COMMA,
  JSON_TOKEN_EOF,
} JsonTokenType;

typedef struct {
  size_t start;
  size_t len;
  size_t start_line;
  size_t start_col;
} TextSpan;

typedef struct {
  char* msg;
  TextSpan span;
} JsonErr;

typedef struct JsonValue JsonValue;
typedef struct HASHMAP_ENTRY_OF(JsonValue*) JsonProp;

struct JsonValue {
  JsonType type;
  char* id;
  TextSpan span;
  union {
    bool bool_value;
    double number_value;
    char* string_value;
    JsonValue* array_value;
    JsonProp* object_value;
  };
};

typedef struct {
  const char* input;
  JsonErr* err;
  size_t input_len;
  size_t pos;
  size_t col;
  size_t line;
} JsonLexerCtx;

typedef struct {
  JsonTokenType type;
  char* value;
  TextSpan span;
} JsonToken;

typedef struct {
  JsonToken** tokens;
  size_t pos;
  JsonErr* err;
} JsonParserCtx;

static inline bool json_lexer_is_end(const JsonLexerCtx* ctx) {
  return ctx->pos >= ctx->input_len;
}

static inline char json_lexer_peek(const JsonLexerCtx* ctx, size_t offset) {
  size_t idx = ctx->pos + offset;
  if (idx >= ctx->input_len)
    return '\0';
  return ctx->input[idx];
}

static inline void json_lexer_advance(JsonLexerCtx* ctx) {
  if (json_lexer_is_end(ctx))
    return;

  ctx->pos++;
  ctx->col++;
}

static inline void json_lexer_advance_line(JsonLexerCtx* ctx) {
  if (json_lexer_peek(ctx, 0) == '\r') {
    json_lexer_advance(ctx);

    if (json_lexer_peek(ctx, 0) == '\n')
      json_lexer_advance(ctx);
  } else if (json_lexer_peek(ctx, 0) == '\n') {
    json_lexer_advance(ctx);
  }

  ctx->line++;
  ctx->col = 1;
}

static inline TextSpan json_lexer_capture_start(const JsonLexerCtx* ctx) {
  return (TextSpan){.start = ctx->pos, .len = 0, .start_line = ctx->line, .start_col = ctx->col};
}

static inline TextSpan json_lexer_capture_end(const JsonLexerCtx* ctx, const TextSpan start) {
  return (TextSpan){.start = start.start,
                    .len = ctx->pos - start.start,
                    .start_line = start.start_line,
                    .start_col = start.start_col};
}

static inline void json_lexer_skip_trivial_chars(JsonLexerCtx* ctx) {
  while (!json_lexer_is_end(ctx)) {
    char c = json_lexer_peek(ctx, 0);

    switch (c) {
      case ' ':
      case '\t':
        json_lexer_advance(ctx);
        break;

      case '\n':
      case '\r':
        json_lexer_advance_line(ctx);
        break;

      default:
        return;
    }
  }
}

static inline bool json_lexer_make_single(JsonLexerCtx* ctx, JsonToken* current_token,
                                          JsonTokenType type) {
  TextSpan start = json_lexer_capture_start(ctx);
  json_lexer_advance(ctx);

  current_token->type = type;
  current_token->value = NULL;
  current_token->span = json_lexer_capture_end(ctx, start);

  return true;
}

static inline int json_lexer_read_hex(JsonLexerCtx* ctx) {
  int value = 0;

  for (int i = 0; i < 4; ++i) {
    if (json_lexer_is_end(ctx)) {
      ctx->err->msg = "Unexpected end of input in unicode escape sequence";
      ctx->err->span = json_lexer_capture_start(ctx);
      return -1;
    }

    char c = json_lexer_peek(ctx, 0);
    value <<= 4;

    if (c >= '0' && c <= '9') {
      value |= (c - '0');
    } else if (c >= 'a' && c <= 'f') {
      value |= (c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
      value |= (c - 'A' + 10);
    } else {
      ctx->err->msg = "Invalid character in Unicode escape sequence";
      ctx->err->span = json_lexer_capture_start(ctx);
      return -1;
    }
    json_lexer_advance(ctx);
  }

  return value;
}

static inline bool json_lexer_read_string(Arena* arena, JsonLexerCtx* ctx,
                                          JsonToken* current_token) {
  TextSpan start = json_lexer_capture_start(ctx);
  json_lexer_advance(ctx);

  Dstr value = arena_dstr_create(arena);

  while (!json_lexer_is_end(ctx)) {
    char c = json_lexer_peek(ctx, 0);

    switch (c) {
      case '\\': {
        json_lexer_advance(ctx);

        if (json_lexer_is_end(ctx)) {
          ctx->err->msg = "Unexpected end of input in string escape sequence";
          ctx->err->span = json_lexer_capture_end(ctx, start);
          return false;
        }

        char escaped = json_lexer_peek(ctx, 0);

        switch (escaped) {
          case '"':
          case '\\':
          case '/':
            arena_dstr_append_char(arena, &value, escaped);
            json_lexer_advance(ctx);
            break;
          case 'b':
            arena_dstr_append_char(arena, &value, '\b');
            json_lexer_advance(ctx);
            break;
          case 'f':
            arena_dstr_append_char(arena, &value, '\f');
            json_lexer_advance(ctx);
            break;
          case 'n':
            arena_dstr_append_char(arena, &value, '\n');
            json_lexer_advance(ctx);
            break;
          case 'r':
            arena_dstr_append_char(arena, &value, '\r');
            json_lexer_advance(ctx);
            break;
          case 't':
            arena_dstr_append_char(arena, &value, '\t');
            json_lexer_advance(ctx);
            break;
          case 'u': {
            json_lexer_advance(ctx);
            int unicode_val = json_lexer_read_hex(ctx);

            if (unicode_val < 0)
              return false;

            arena_dstr_append_utf8(arena, &value, unicode_val);
            break;
          }
          default: {
            ctx->err->msg =
                arena_dstr_fmt(arena, "Invalid escape character %c in string literal", escaped);
            ctx->err->span = json_lexer_capture_end(ctx, start);
            return false;
          }
        }
      }

      case '\n':
      case '\r':
        ctx->err->msg = "Unescaped new line in string literal";
        ctx->err->span = json_lexer_capture_end(ctx, start);
        return false;

      case '"':
        json_lexer_advance(ctx);
        current_token->type = JSON_TOKEN_STRING;
        current_token->value = value;
        current_token->span = json_lexer_capture_end(ctx, start);
        return true;

      default:
        arena_dstr_append_fmt(arena, &value, "%c", c);
        json_lexer_advance(ctx);
        break;
    }
  }

  ctx->err->msg = "Unexpected end of input in string literal";
  ctx->err->span = json_lexer_capture_end(ctx, start);
  return false;
}

static inline bool json_lexer_read_number(Arena* arena, JsonLexerCtx* ctx,
                                          JsonToken* current_token) {
  TextSpan start = json_lexer_capture_start(ctx);
  Dstr num = arena_dstr_create(arena);

  if (json_lexer_peek(ctx, 0) == '-') {
    arena_dstr_append_char(arena, &num, '-');
    json_lexer_advance(ctx);
  }

  if (json_lexer_peek(ctx, 0) == '0') {
    arena_dstr_append_char(arena, &num, '0');
    json_lexer_advance(ctx);
  } else if (isdigit(json_lexer_peek(ctx, 0))) {
    while (isdigit(json_lexer_peek(ctx, 0))) {
      arena_dstr_append_char(arena, &num, json_lexer_peek(ctx, 0));
      json_lexer_advance(ctx);
    }
  } else {
    ctx->err->msg = "Invalid number: '-' must be followed by a digit";
    ctx->err->span = json_lexer_capture_end(ctx, start);
    return false;
  }

  if (json_lexer_peek(ctx, 0) == '.') {
    arena_dstr_append_char(arena, &num, '.');
    json_lexer_advance(ctx);

    if (!isdigit(json_lexer_peek(ctx, 0))) {
      ctx->err->msg = "Invalid number format: expected digit after '.'";
      ctx->err->span = json_lexer_capture_end(ctx, start);

      return false;
    }

    while (isdigit(json_lexer_peek(ctx, 0))) {
      arena_dstr_append_char(arena, &num, json_lexer_peek(ctx, 0));
      json_lexer_advance(ctx);
    }
  }

  if (json_lexer_peek(ctx, 0) == 'e' || json_lexer_peek(ctx, 0) == 'E') {
    arena_dstr_append_char(arena, &num, json_lexer_peek(ctx, 0));
    json_lexer_advance(ctx);

    if (json_lexer_peek(ctx, 0) == '+' || json_lexer_peek(ctx, 0) == '-') {
      arena_dstr_append_char(arena, &num, json_lexer_peek(ctx, 0));
      json_lexer_advance(ctx);
    }

    if (!isdigit(json_lexer_peek(ctx, 0))) {
      ctx->err->msg = "Invalid number format: expected digit in exponent";
      ctx->err->span = json_lexer_capture_end(ctx, start);
      return false;
    }

    while (isdigit(json_lexer_peek(ctx, 0))) {
      arena_dstr_append_char(arena, &num, json_lexer_peek(ctx, 0));
      json_lexer_advance(ctx);
    }
  }

  TextSpan span = json_lexer_capture_end(ctx, start);

  current_token->type = JSON_TOKEN_NUMBER;
  current_token->value = num;
  current_token->span = span;

  return true;
}

static inline bool json_lexer_remaining_starts_with(JsonLexerCtx* ctx, const char* str) {
  Strv strv = strv_from(ctx->input + ctx->pos);
  return strv_starts_with(strv, strv_from(str));
}

static inline bool json_lexer_match_keyword(JsonLexerCtx* ctx, const char* keyword,
                                            JsonTokenType type, JsonToken* current_token) {
  if (json_lexer_remaining_starts_with(ctx, keyword)) {
    TextSpan start = json_lexer_capture_start(ctx);

    for (size_t i = 0; keyword[i] != '\0'; ++i)
      json_lexer_advance(ctx);

    current_token->type = type;
    current_token->value = NULL;
    current_token->span = json_lexer_capture_end(ctx, start);

    return true;
  }
  return false;
}

static inline bool json_lexer_read_keyword(JsonLexerCtx* ctx, JsonToken* current_token) {
  if (json_lexer_match_keyword(ctx, "true", JSON_TOKEN_TRUE, current_token) ||
      json_lexer_match_keyword(ctx, "false", JSON_TOKEN_FALSE, current_token) ||
      json_lexer_match_keyword(ctx, "null", JSON_TOKEN_NULL, current_token))
    return true;

  ctx->err->msg = "Unexpected token: expected 'true', 'false', or 'null'";
  ctx->err->span = json_lexer_capture_start(ctx);
  return false;
}

static inline bool json_lexer_next(Arena* arena, JsonLexerCtx* ctx, JsonToken* current_token) {
  json_lexer_skip_trivial_chars(ctx);

  if (json_lexer_is_end(ctx)) {
    current_token->type = JSON_TOKEN_EOF;
    current_token->value = NULL;
    current_token->span.start = ctx->pos;
    current_token->span.len = 0;
    current_token->span.start_line = ctx->line;
    current_token->span.start_col = ctx->col;
    return true;
  }

  char c = json_lexer_peek(ctx, 0);

  switch (c) {
    case '{':
      return json_lexer_make_single(ctx, current_token, JSON_TOKEN_LBRACE);
    case '}':
      return json_lexer_make_single(ctx, current_token, JSON_TOKEN_RBRACE);
    case '[':
      return json_lexer_make_single(ctx, current_token, JSON_TOKEN_LBRACKET);
    case ']':
      return json_lexer_make_single(ctx, current_token, JSON_TOKEN_RBRACKET);
    case ':':
      return json_lexer_make_single(ctx, current_token, JSON_TOKEN_COLON);
    case ',':
      return json_lexer_make_single(ctx, current_token, JSON_TOKEN_COMMA);
    case '"':
      return json_lexer_read_string(arena, ctx, current_token);
    case '-':
    case '0' ... '9':
      return json_lexer_read_number(arena, ctx, current_token);
    default:
      return json_lexer_read_keyword(ctx, current_token);
  }
}

static inline bool json_tokenize(Arena* arena, const char* input, JsonToken** tokens,
                                 JsonErr* err) {
  JsonLexerCtx ctx = {
      .input = input, .err = err, .input_len = strlen(input), .pos = 0, .col = 1, .line = 1};

  JsonToken token;

  do {
    if (!json_lexer_next(arena, &ctx, &token))
      return false;

    arena_darr_append(arena, *tokens, token);
  } while (token.type != JSON_TOKEN_EOF);

  return true;
}

static inline JsonToken* json_parser_eat(JsonParserCtx* ctx, JsonTokenType expected) {
  JsonToken* token = NULL;

  if (darr_len(*ctx->tokens) <= ctx->pos || (token = &(*ctx->tokens)[ctx->pos])->type != expected) {
    ctx->err->msg = "Unexpected token";
    ctx->err->span = token ? token->span : (TextSpan){.start = 0, .len = 0};
    return NULL;
  }

  ctx->pos++;
  return token;
}

static inline TextSpan json_parser_combine(TextSpan start, TextSpan end) {
  return (TextSpan){.start = start.start,
                    .len = ABS(end.start - start.start),
                    .start_line = start.start_line,
                    .start_col = start.start_col};
}

static inline bool json_parse_object(JsonParserCtx* ctx, JsonValue* out, const Dstr id,
                                     Arena* arena);
static inline bool json_parse_array(JsonParserCtx* ctx, JsonValue* out, const Dstr id,
                                    Arena* arena);

static inline bool json_parse_string(JsonParserCtx* ctx, JsonValue* out, const Dstr id,
                                     Arena* arena) {
  JsonToken* token = json_parser_eat(ctx, JSON_TOKEN_STRING);

  if (!token)
    return false;

  out->type = JSON_STRING;
  out->id = arena_dstr_to_cstr(arena, id);
  out->span = token->span;
  out->string_value = arena_dstr_to_cstr(arena, token->value);

  return true;
}

static inline bool json_parse_number(JsonParserCtx* ctx, JsonValue* out, const Dstr id,
                                     Arena* arena) {
  JsonToken* token = json_parser_eat(ctx, JSON_TOKEN_NUMBER);

  if (!token)
    return false;

  char* endptr;
  double num = strtod(token->value, &endptr);
  if (*endptr != '\0') {
    ctx->err->msg = "Invalid number format: could not parse entire number";
    ctx->err->span = token->span;
    return false;
  }

  out->type = JSON_NUMBER;
  out->id = arena_dstr_to_cstr(arena, id);
  out->span = token->span;
  out->number_value = num;

  return true;
}

static inline bool json_parse_bool(JsonParserCtx* ctx, JsonValue* out, bool val, const Dstr id,
                                   Arena* arena) {
  JsonToken* token = json_parser_eat(ctx, val ? JSON_TOKEN_TRUE : JSON_TOKEN_FALSE);

  if (!token)
    return false;

  out->type = JSON_BOOL;
  out->id = arena_dstr_to_cstr(arena, id);
  out->span = token->span;
  out->bool_value = val;

  return true;
}

static inline bool json_parse_null(JsonParserCtx* ctx, JsonValue* out, const Dstr id,
                                   Arena* arena) {
  JsonToken* token = json_parser_eat(ctx, JSON_TOKEN_NULL);

  if (!token)
    return false;

  out->type = JSON_NULL;
  out->id = arena_dstr_to_cstr(arena, id);
  out->span = token->span;

  return true;
}

static inline bool json_parse_value(JsonParserCtx* ctx, JsonValue* out, const Dstr id,
                                    Arena* arena) {
  if (darr_len(*ctx->tokens) <= ctx->pos) {
    ctx->err->msg = "Unexpected end of input while parsing";
    ctx->err->span.start = 0;
    ctx->err->span.start_line = 0;
    ctx->err->span.start_col = 0;
    return false;
  }

  JsonToken* token = &(*ctx->tokens)[ctx->pos];
  switch (token->type) {
    case JSON_TOKEN_LBRACE:
      return json_parse_object(ctx, out, id, arena);
    case JSON_TOKEN_LBRACKET:
      return json_parse_array(ctx, out, id, arena);
    case JSON_TOKEN_STRING:
      return json_parse_string(ctx, out, id, arena);
    case JSON_TOKEN_NUMBER:
      return json_parse_number(ctx, out, id, arena);
    case JSON_TOKEN_TRUE:
      return json_parse_bool(ctx, out, true, id, arena);
    case JSON_TOKEN_FALSE:
      return json_parse_bool(ctx, out, false, id, arena);
    case JSON_TOKEN_NULL:
      return json_parse_null(ctx, out, id, arena);
    default:
      ctx->err->msg = "Unexpected token while parsing value";
      ctx->err->span = token->span;
      return false;
  }
}

static inline bool json_parse_object(JsonParserCtx* ctx, JsonValue* out, const Dstr id,
                                     Arena* arena) {
  JsonToken* start = json_parser_eat(ctx, JSON_TOKEN_LBRACE);
  if (!start)
    return false;

  JsonProp* props = NULL;

  if (darr_len(*ctx->tokens) <= ctx->pos) {
    ctx->err->msg = "Unexpected end of input while parsing object";
    ctx->err->span = (TextSpan){.start = 0, .len = 0};
    return false;
  }

  if ((*ctx->tokens)[ctx->pos].type == JSON_TOKEN_RBRACE) {
    JsonToken* end = json_parser_eat(ctx, JSON_TOKEN_RBRACE);
    out->type = JSON_OBJECT;
    out->id = arena_dstr_to_cstr(arena, id);
    out->object_value = props;

    return true;
  }

  while (true) {
    JsonToken* key_token = json_parser_eat(ctx, JSON_TOKEN_STRING);
    if (!key_token) {
      return false;
    }

    Dstr key = arena_dstr_concat_fmt(arena, id, ".%s", key_token->value);
    if (!json_parser_eat(ctx, JSON_TOKEN_COLON)) {
      return false;
    }
    JsonValue* value = arena_alloc(arena, sizeof *value);

    if (!json_parse_value(ctx, value, key, arena)) {
      return false;
    }

    arena_hashmap_set(arena, props, key_token->value, value);

    switch ((*ctx->tokens)[ctx->pos].type) {
      case JSON_TOKEN_COMMA:
        if (!json_parser_eat(ctx, JSON_TOKEN_COMMA)) {
          return false;
        }
        break;
      case JSON_TOKEN_RBRACE:
        JsonToken* end = json_parser_eat(ctx, JSON_TOKEN_RBRACE);
        out->type = JSON_OBJECT;
        out->span = json_parser_combine(start->span, end->span);
        out->id = arena_dstr_to_cstr(arena, id);
        out->object_value = props;
        return true;
      default:
        ctx->err->msg = "Expected ',' or '}' in object literal";
        ctx->err->span = (*ctx->tokens)[ctx->pos].span;
        return false;
    }
  }
}

static inline bool json_parse_array(JsonParserCtx* ctx, JsonValue* out, const Dstr id,
                                    Arena* arena) {
  JsonToken* start = json_parser_eat(ctx, JSON_TOKEN_LBRACKET);
  if (!start)
    return false;

  JsonValue* elements = NULL;

  if (darr_len(*ctx->tokens) <= ctx->pos) {
    ctx->err->msg = "Unexpected end of input while parsing array";
    ctx->err->span = (TextSpan){.start = 0, .len = 0};
    return false;
  }

  if ((*ctx->tokens)[ctx->pos].type == JSON_TOKEN_RBRACKET) {
    JsonToken* end = json_parser_eat(ctx, JSON_TOKEN_RBRACKET);

    out->type = JSON_ARRAY;
    out->id = arena_dstr_to_cstr(arena, id);
    out->span = json_parser_combine(start->span, end->span);
    out->array_value = elements;

    return true;
  }

  size_t idx = 0;

  while (true) {
    Dstr item_id = arena_dstr_concat_fmt(arena, id, "[%zu]", idx);
    JsonValue value;

    if (!json_parse_value(ctx, &value, item_id, arena)) {
      return false;
    }

    arena_darr_append(arena, elements, value);

    switch ((*ctx->tokens)[ctx->pos].type) {
      case JSON_TOKEN_COMMA:
        if (!json_parser_eat(ctx, JSON_TOKEN_COMMA)) {
          return false;
        }
        idx++;
        break;

      case JSON_TOKEN_RBRACKET:
        JsonToken* end = json_parser_eat(ctx, JSON_TOKEN_RBRACKET);
        out->type = JSON_ARRAY;
        out->id = arena_dstr_to_cstr(arena, id);
        out->span = json_parser_combine(start->span, end->span);
        out->array_value = elements;
        return true;

      default:
        ctx->err->msg = "Expected ',' or ']' in array literal";
        ctx->err->span = (*ctx->tokens)[ctx->pos].span;
        return false;
    }
  }
}

static inline char* json_build_error_msg(Arena* arena, Arena* target_arena, JsonErr* err) {
  Dstr msg = arena_dstr_fmt(arena, "Error at line %zu, column %zu: '%s'\n", err->span.start_line,
                            err->span.start_col, err->msg);
  return arena_dstr_to_cstr(target_arena, msg);
}

static inline bool json_parse(const char* input, Arena* target_arena, JsonValue* out,
                              char** err_msg) {
  JsonErr err = {0};
  JsonToken* tokens = NULL;
  JsonParserCtx parser_ctx = {.tokens = &tokens, .pos = 0, .err = &err};
  Arena arena = arena_create(1024);

  if (!json_tokenize(&arena, input, &tokens, &err)) {
    *err_msg = json_build_error_msg(&arena, target_arena, &err);
    arena_destroy(&arena);
    return false;
  }

  if (!json_parse_value(&parser_ctx, out, arena_dstr_from(&arena, "$"), target_arena)) {
    *err_msg = json_build_error_msg(&arena, target_arena, &err);
    arena_destroy(&arena);
    return false;
  }

  if (darr_len(tokens) > parser_ctx.pos && tokens[parser_ctx.pos].type != JSON_TOKEN_EOF) {
    err.msg = "Extra data after top-level JSON value";
    err.span = tokens[parser_ctx.pos].span;
    *err_msg = json_build_error_msg(&arena, target_arena, &err);
    arena_destroy(&arena);
    return false;
  }

  arena_destroy(&arena);
  return true;
}

static inline bool json_get_ref(JsonValue* root, const char* path, JsonValue** out) {
  Strv* fragments = strv_split(strv_from(path), '.');
  JsonValue* current = root;

  for (size_t i = 0; i < darr_len(fragments); ++i) {
    Strv* segment = &fragments[i];
    Dstr key = dstr_from_strv(*segment);

    if (current->type == JSON_OBJECT) {
      JsonValue* next;

      if (!hashmap_get(current->object_value, key, &next)) {
        darr_destroy(fragments);
        dstr_destroy(key);
        return false;
      }

      current = next;
      dstr_destroy(key);
    } else if (current->type == JSON_ARRAY) {
      char* endptr;
      long idx = strtol(key, &endptr, 10);

      if (*endptr != '\0' || idx < 0 || (size_t)idx >= darr_len(current->array_value)) {
        darr_destroy(fragments);
        dstr_destroy(key);
        return false;
      }

      JsonValue* next = &current->array_value[idx];
      if (!next) {
        darr_destroy(fragments);
        dstr_destroy(key);
        return false;
      }

      current = next;
      dstr_destroy(key);
    } else {
      darr_destroy(fragments);
      dstr_destroy(key);
      return false;
    }
  }

  *out = current;
  darr_destroy(fragments);
  return true;
}

static inline bool json_get(JsonValue* root, const char* path, JsonValue* out) {
  JsonValue* ref;
  if (!json_get_ref(root, path, &ref)) {
    return false;
  }

  *out = *ref;
  return true;
}

#endif // !C_UTILS_JSON_LIB_IMPLEMENTATION
