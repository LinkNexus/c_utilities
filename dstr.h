#ifndef C_UTILS_DSTR_IMPLEMENTATION
#define C_UTILS_DSTR_IMPLEMENTATION

#include "arena.h"
#include "strv.h"
#include "utils.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define DSTR_MIN_CAP 4

typedef char* Dstr;
typedef struct {
  size_t size;
  size_t cap;
  char buf[];
} DstrHdr;

#define DSTR_HDR(dstr) ((DstrHdr*)(dstr) - 1)

#define DSTR_LEN(dstr) (DSTR_HDR(dstr)->size)

static inline Dstr dstr_create(void) {
  DstrHdr* hdr = (DstrHdr*)xmalloc(DSTR_MIN_CAP + sizeof *hdr);
  hdr->size = 0;
  hdr->cap = DSTR_MIN_CAP;
  hdr->buf[0] = '\0';
  return hdr->buf;
}

static inline Dstr arena_dstr_create(Arena* arena) {
  DstrHdr* hdr = arena_alloc(arena, DSTR_MIN_CAP + sizeof *hdr);
  hdr->size = 0;
  hdr->cap = DSTR_MIN_CAP;
  hdr->buf[0] = '\0';
  return hdr->buf;
}

static inline Dstr dstr_from(const char* c_str) {
  size_t input_len = strlen(c_str);
  DstrHdr* hdr = (DstrHdr*)xmalloc(input_len + 1 + sizeof *hdr);

  hdr->size = input_len;
  hdr->cap = input_len + 1;
  memcpy(hdr->buf, c_str, input_len + 1);

  return hdr->buf;
}

static inline Dstr arena_dstr_from(Arena* arena, const char* c_str) {
  size_t input_len = strlen(c_str);
  DstrHdr* hdr = arena_alloc(arena, input_len + 1 + sizeof *hdr);

  hdr->size = input_len;
  hdr->cap = input_len + 1;
  memcpy(hdr->buf, c_str, input_len + 1);

  return hdr->buf;
}

static inline Dstr dstr_from_strv(const Strv strv) {
  DstrHdr* hdr = (DstrHdr*)xmalloc(strv.len + 1 + sizeof *hdr);
  hdr->size = strv.len;
  hdr->cap = strv.len + 1;
  memcpy(hdr->buf, strv.buf, strv.len);
  hdr->buf[strv.len] = '\0';

  return hdr->buf;
}

static inline Dstr arena_dstr_from_strv(Arena* arena, const Strv strv) {
  DstrHdr* hdr = arena_alloc(arena, strv.len + 1 + sizeof *hdr);
  hdr->size = strv.len;
  hdr->cap = strv.len + 1;
  memcpy(hdr->buf, strv.buf, strv.len);
  hdr->buf[strv.len] = '\0';

  return hdr->buf;
}

static inline void dstr_destroy(const Dstr dstr) {
  free(DSTR_HDR(dstr));
}

static inline char* dstr_to_cstr(const Dstr dstr) {
  size_t dstr_size = DSTR_HDR(dstr)->size;
  char* c_str = (char*)xmalloc(dstr_size + 1);
  snprintf(c_str, dstr_size + 1, "%s", dstr);
  return c_str;
}

static inline char* arena_dstr_to_cstr(Arena* arena, const Dstr dstr) {
  size_t dstr_size = DSTR_HDR(dstr)->size;
  char* c_str = arena_alloc(arena, dstr_size + 1);
  snprintf(c_str, dstr_size + 1, "%s", dstr);
  return c_str;
}

static inline void dstr_reserve(Dstr* dstr, size_t new_cap) {
  DstrHdr* hdr = DSTR_HDR(*dstr);
  size_t old_size = hdr->size;

  DstrHdr* new_hdr = (DstrHdr*)xrealloc(hdr, new_cap + sizeof *new_hdr);
  new_hdr->cap = new_cap;
  new_hdr->size = old_size;
  *dstr = new_hdr->buf;
}

static inline void arena_dstr_reserve(Arena* arena, Dstr* dstr, size_t new_cap) {
  DstrHdr* hdr = DSTR_HDR(*dstr);
  size_t old_size = hdr->size;

  DstrHdr* new_hdr = (DstrHdr*)arena_alloc(arena, new_cap + sizeof *new_hdr);
  memcpy(new_hdr, hdr, sizeof(*hdr) + old_size + 1);
  new_hdr->cap = new_cap;
  new_hdr->size = old_size;
  *dstr = new_hdr->buf;
}

static inline void dstr_ensure_cap(Dstr* dstr, size_t count) {
  DstrHdr* hdr = DSTR_HDR(*dstr);
  if (hdr->size + count + 1 <= hdr->cap)
    return;

  size_t new_cap = MAX(MAX(hdr->cap * 2, hdr->cap + count), DSTR_MIN_CAP);
  dstr_reserve(dstr, new_cap);
}

static inline void arena_dstr_ensure_cap(Arena* arena, Dstr* dstr, size_t count) {
  DstrHdr* hdr = DSTR_HDR(*dstr);
  if (hdr->size + count + 1 <= hdr->cap)
    return;

  size_t new_cap = MAX(MAX(hdr->cap * 2, hdr->cap + count), DSTR_MIN_CAP);
  arena_dstr_reserve(arena, dstr, new_cap);
}

static inline void dstr_append(Dstr* dstr, const char* c_str) {
  DstrHdr* hdr = DSTR_HDR(*dstr);
  size_t c_str_len = strlen(c_str);

  dstr_ensure_cap(dstr, c_str_len);
  hdr = DSTR_HDR(*dstr);
  snprintf(*dstr + hdr->size, hdr->cap - hdr->size, "%s", c_str);
  hdr->size += c_str_len;
}

static inline void arena_dstr_append(Arena* arena, Dstr* dstr, const char* c_str) {
  DstrHdr* hdr = DSTR_HDR(*dstr);
  size_t c_str_len = strlen(c_str);

  arena_dstr_ensure_cap(arena, dstr, c_str_len);
  hdr = DSTR_HDR(*dstr);
  snprintf(*dstr + hdr->size, hdr->cap - hdr->size, "%s", c_str);
  hdr->size += c_str_len;
}

static inline Dstr dstr_concat(const char* c_str1, const char* c_str2) {
  Dstr res = dstr_from(c_str1);
  dstr_append(&res, c_str2);
  return res;
}

static inline Dstr arena_dstr_concat(Arena* arena, const char* c_str1, const char* c_str2) {
  Dstr res = arena_dstr_from(arena, c_str1);
  arena_dstr_append(arena, &res, c_str2);
  return res;
}

static inline size_t dstr_va_req_len(const char* fmt, va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  int req_len = vsnprintf(NULL, 0, fmt, args_copy);

  if (req_len < 0) {
    va_end(args_copy);
    return 0;
  }

  va_end(args_copy);
  return (size_t)req_len;
}

static inline Dstr dstr_fmt(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  va_list args_copy;
  va_copy(args_copy, args);
  int req_len = vsnprintf(NULL, 0, fmt, args_copy);

  if (req_len < 0) {
    va_end(args_copy);
    va_end(args);
    print_fn_err_msg("dstr_fmt: Error formatting string: '%s'\n", fmt);
    return NULL;
  }

  va_end(args_copy);

  Dstr res = dstr_create();
  dstr_ensure_cap(&res, req_len + 1);

  vsnprintf(res, req_len + 1, fmt, args);
  DSTR_HDR(res)->size = req_len;
  va_end(args);

  return res;
}

static inline Dstr arena_dstr_fmt(Arena* arena, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  va_list args_copy;
  va_copy(args_copy, args);
  int req_len = vsnprintf(NULL, 0, fmt, args_copy);

  if (req_len < 0) {
    va_end(args_copy);
    va_end(args);
    print_fn_err_msg("dstr_fmt: Error formatting string: '%s'\n", fmt);
    return NULL;
  }

  va_end(args_copy);

  Dstr res = arena_dstr_create(arena);
  arena_dstr_ensure_cap(arena, &res, req_len + 1);

  vsnprintf(res, req_len + 1, fmt, args);
  DSTR_HDR(res)->size = req_len;
  va_end(args);

  return res;
}

static inline void dstr_append_fmt(Dstr* dstr, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t new_len = dstr_va_req_len(fmt, args);
  if (new_len == 0) {
    va_end(args);
    return;
  }

  dstr_ensure_cap(dstr, new_len + 1);
  DstrHdr* hdr = DSTR_HDR(*dstr);
  vsnprintf((*dstr) + hdr->size, new_len + 1, fmt, args);
  hdr->size += new_len;

  va_end(args);
}

static inline void arena_dstr_append_fmt(Arena* arena, Dstr* dstr, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t new_len = dstr_va_req_len(fmt, args);
  if (new_len == 0) {
    va_end(args);
    return;
  }

  arena_dstr_ensure_cap(arena, dstr, new_len + 1);
  DstrHdr* hdr = DSTR_HDR(*dstr);
  vsnprintf((*dstr) + hdr->size, new_len + 1, fmt, args);
  hdr->size += new_len;

  va_end(args);
}

static inline void dstr_append_char(Dstr* dstr, const char c) {
  return dstr_append_fmt(dstr, "%c", c);
}

static inline void arena_dstr_append_char(Arena* arena, Dstr* dstr, const char c) {
  return arena_dstr_append_fmt(arena, dstr, "%c", c);
}

static inline int dstr_get_utf8_buf(int codepoint, char utf8_buf[4]) {
  if (codepoint < 0 || codepoint > 0x10FFFF)
    return 0;

  if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
    return 0;

  int utf8_len = 0;

  if (codepoint < 0x80) {
    utf8_buf[0] = codepoint;
    utf8_len = 1;
  } else if (codepoint < 0x800) {
    utf8_buf[0] = 0xC0 | (codepoint >> 6);
    utf8_buf[1] = 0x80 | (codepoint & 0x3F);
    utf8_len = 2;
  } else if (codepoint < 0x10000) {
    utf8_buf[0] = 0xE0 | (codepoint >> 12);
    utf8_buf[1] = 0x80 | ((codepoint >> 6) & 0x3F);
    utf8_buf[2] = 0x80 | (codepoint & 0x3F);
    utf8_len = 3;
  } else {
    utf8_buf[0] = 0xF0 | (codepoint >> 18);
    utf8_buf[1] = 0x80 | ((codepoint >> 12) & 0x3F);
    utf8_buf[2] = 0x80 | ((codepoint >> 6) & 0x3F);
    utf8_buf[3] = 0x80 | (codepoint & 0x3F);
    utf8_len = 4;
  }

  return utf8_len;
}

static inline void dstr_append_utf8(Dstr* dstr, int codepoint) {
  char utf8_buf[4];
  int utf8_len = dstr_get_utf8_buf(codepoint, utf8_buf);
  dstr_append_fmt(dstr, "%.*s", utf8_len, utf8_buf);
}

static inline void arena_dstr_append_utf8(Arena* arena, Dstr* dstr, int codepoint) {
  char utf8_buf[4];
  int utf8_len = dstr_get_utf8_buf(codepoint, utf8_buf);
  arena_dstr_append_fmt(arena, dstr, "%.*s", utf8_len, utf8_buf);
}

static inline Dstr dstr_concat_fmt(const char* c_str, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t new_len = dstr_va_req_len(fmt, args);
  if (new_len == 0) {
    va_end(args);
    return dstr_from(c_str);
  }

  size_t current_len = DSTR_HDR(c_str)->size;
  Dstr res = dstr_create();
  dstr_ensure_cap(&res, current_len + new_len + 1);

  memcpy(res, c_str, current_len);
  vsnprintf(res + current_len, new_len + 1, fmt, args);
  DSTR_HDR(res)->size = current_len + new_len;

  va_end(args);
  return res;
}

static inline Dstr arena_dstr_concat_fmt(Arena* arena, const char* c_str, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t new_len = dstr_va_req_len(fmt, args);
  if (new_len == 0) {
    va_end(args);
    return arena_dstr_from(arena, c_str);
  }

  size_t current_len = DSTR_HDR(c_str)->size;
  Dstr res = arena_dstr_create(arena);
  arena_dstr_ensure_cap(arena, &res, current_len + new_len + 1);

  memcpy(res, c_str, current_len);
  vsnprintf(res + current_len, new_len + 1, fmt, args);
  DSTR_HDR(res)->size = current_len + new_len;

  va_end(args);
  return res;
}

static inline Dstr dstr_concat_utf8(const char* c_str, int codepoint) {
  Dstr res = dstr_from(c_str);
  dstr_append_utf8(&res, codepoint);
  return res;
}

static inline Dstr arena_dstr_concat_utf8(Arena* arena, const char* c_str, int codepoint) {
  Dstr res = dstr_from(c_str);
  arena_dstr_append_utf8(arena, &res, codepoint);
  return res;
}

#endif // !C_UTILS_DSTR_IMPLEMENTATION
