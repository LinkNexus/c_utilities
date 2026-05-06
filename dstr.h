#ifndef C_UTILS_DSTR_IMPLEMENTATION
#define C_UTILS_DSTR_IMPLEMENTATION

#include "darr.h"
#include "strv.h"
#include "utils.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define DSTR_MIN_CAP 4

typedef char *Dstr;
typedef struct {
  size_t size;
  size_t cap;
  char buf[];
} DstrHdr;

#define DSTR_HDR(dstr) ((DstrHdr *)(dstr) - 1)

#define DSTR_LEN(dstr) (DSTR_HDR(dstr)->size)

static inline Dstr dstr_create(void) {
  DstrHdr *hdr = (DstrHdr *)xmalloc(DSTR_MIN_CAP + sizeof *hdr);
  hdr->size = 0;
  hdr->cap = DSTR_MIN_CAP;
  hdr->buf[0] = '\0';
  return hdr->buf;
}

static inline Dstr dstr_from(const char *c_str) {
  size_t input_len = strlen(c_str);
  DstrHdr *hdr = (DstrHdr *)xmalloc(input_len + 1 + sizeof *hdr);

  hdr->size = input_len;
  hdr->cap = input_len + 1;
  memcpy(hdr->buf, c_str, input_len + 1);

  return hdr->buf;
}

static inline Dstr dstr_from_strv(const Strv strv) {
  DstrHdr *hdr = (DstrHdr *)xmalloc(strv.len + 1 + sizeof *hdr);
  hdr->size = strv.len;
  hdr->cap = strv.len + 1;
  memcpy(hdr->buf, strv.buf, strv.len);
  hdr->buf[strv.len] = '\0';

  return hdr->buf;
}

static inline void dstr_destroy(const Dstr dstr) { free(DSTR_HDR(dstr)); }

static inline char *dstr_to_cstr(const Dstr dstr) {
  size_t dstr_size = DSTR_HDR(dstr)->size;
  char *c_str = (char *)xmalloc(dstr_size + 1);
  snprintf(c_str, dstr_size + 1, "%s", dstr);
  return c_str;
}

#define dstr_reserve(dstr, new_cap)                                            \
  do {                                                                         \
    DstrHdr *hdr = DSTR_HDR(dstr);                                             \
    size_t old_size = hdr->size;                                               \
                                                                               \
    DstrHdr *new_hdr = (DstrHdr *)xrealloc(hdr, new_cap + sizeof(*new_hdr));   \
    new_hdr->cap = new_cap;                                                    \
    new_hdr->size = old_size;                                                  \
    (dstr) = new_hdr->buf;                                                     \
  } while (0);

#define dstr_ensure_cap(dstr, count)                                           \
  do {                                                                         \
    DstrHdr *hdr = DSTR_HDR(dstr);                                             \
    if (hdr->size + count + 1 <= hdr->cap)                                     \
      break;                                                                   \
                                                                               \
    size_t new_cap = MAX(MAX(hdr->cap * 2, hdr->cap + count), DSTR_MIN_CAP);   \
    dstr_reserve(dstr, new_cap);                                               \
  } while (0);

#define dstr_append(dstr, c_str)                                               \
  do {                                                                         \
    DstrHdr *hdr = DSTR_HDR(dstr);                                             \
    size_t c_str_len = strlen(c_str);                                          \
                                                                               \
    dstr_ensure_cap(dstr, c_str_len);                                          \
    hdr = DSTR_HDR(dstr);                                                      \
    snprintf(dstr + hdr->size, hdr->cap - hdr->size, "%s", c_str);             \
    DSTR_HDR(dstr)->size += c_str_len;                                         \
  } while (0);

static inline Dstr dstr_concat(const Dstr dstr, const char *c_str) {
  Dstr res = dstr_from(dstr);
  dstr_append(res, c_str);
  return res;
}

static inline size_t dstr_va_req_len(const char *fmt, va_list args) {
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

static inline Dstr dstr_fmt(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  va_list args_copy;
  va_copy(args_copy, args);
  int req_len = vsnprintf(NULL, 0, fmt, args_copy);

  if (req_len < 0) {
    va_end(args_copy);
    print_fn_err_msg("dstr_fmt: Error formatting string: '%s'\n", fmt);
    return NULL;
  }

  va_end(args_copy);

  Dstr res = dstr_create();
  dstr_ensure_cap(res, req_len + 1);

  vsnprintf(res, req_len + 1, fmt, args);
  DSTR_HDR(res)->size = req_len;
  va_end(args);

  return res;
}

static inline void dstr_append_fmt_impl(Dstr *dstr, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t new_len = dstr_va_req_len(fmt, args);
  if (new_len == 0)
    return;

  dstr_ensure_cap(*dstr, new_len + 1);
  DstrHdr *hdr = DSTR_HDR(*dstr);
  vsnprintf(*dstr + hdr->size, new_len + 1, fmt, args);
  hdr->size += new_len;

  va_end(args);
}

#define dstr_append_fmt(dstr, fmt, ...)                                        \
  dstr_append_fmt_impl(&(dstr), fmt, ##__VA_ARGS__)

static inline Dstr dstr_concat_fmt_fn(const Dstr dstr, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t new_len = dstr_va_req_len(fmt, args);
  if (new_len == 0)
    return dstr_from(dstr);

  size_t current_len = DSTR_HDR(dstr)->size;
  Dstr res = dstr_create();
  dstr_ensure_cap(res, current_len + new_len + 1);

  memcpy(res, dstr, current_len);
  vsnprintf(res + current_len, new_len + 1, fmt, args);
  DSTR_HDR(res)->size = current_len + new_len;

  va_end(args);
  return res;
}

#define dstr_concat_fmt(dstr, fmt, ...)                                        \
  dstr_concat_fmt_fn(dstr, fmt, ##__VA_ARGS__)

#endif // !C_UTILS_DSTR_IMPLEMENTATION
