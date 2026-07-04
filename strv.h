#ifndef C_UTILS_STRING_VIEWS
#define C_UTILS_STRING_VIEWS

#include "darr.h"
#include "arena.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct {
  const char *buf;
  size_t len;
} Strv;

static inline Strv arena_strv_from(Arena *arena, const char *c_str) {
  size_t len = strlen(c_str);
  char *buf = arena_strndup(arena, c_str, len);
  return (Strv){.buf = buf, .len = len};
}

static inline Strv strv_from(const char *c_str) {
  return (Strv){.buf = c_str, .len = strlen(c_str)};
}

static inline Strv strv_from_len(const char *str, size_t len) {
  return (Strv){.buf = str, .len = len};
}

static inline int strv_cmp(const Strv a, const Strv b) {
  if (a.len == 0 && b.len == 0)
    return 0;
  if (a.len == 0)
    return -1;
  if (b.len == 0)
    return 1;

  size_t min_len = MIN(a.len, b.len);
  int cmp = memcmp(a.buf, b.buf, min_len);
  if (cmp != 0)
    return cmp;
  if (a.len < b.len)
    return -1;
  if (a.len > b.len)
    return 1;
  return 0;
}

static inline bool strv_eq(const Strv a, const Strv b) {
  return strv_cmp(a, b) == 0;
}

static inline Strv strv_substr(const Strv strv, size_t start, size_t len) {
  if (start > strv.len)
    start = strv.len;
  if (start + len > strv.len)
    len = strv.len - start;

  return (Strv){.buf = strv.buf + start, .len = len};
}

static inline Strv strv_slice(const Strv strv, size_t start) {
  if (start > strv.len)
    start = strv.len;
  return (Strv){.buf = strv.buf + start, .len = strv.len - start};
}

static inline size_t strv_find(const Strv strv, const Strv substrv) {
  if (substrv.len > strv.len)
    return (size_t)-1;

  for (size_t i = 0; i <= strv.len - substrv.len; i++) {
    if (strv_cmp(strv_substr(strv, i, substrv.len), substrv) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

static inline size_t strv_find_char(const Strv strv, char c) {
  for (size_t i = 0; i < strv.len; i++) {
    if (strv.buf[i] == c) {
      return i;
    }
  }
  return (size_t)-1;
}

static inline size_t strv_rfind(const Strv strv, const Strv substrv) {
  if (substrv.len > strv.len)
    return (size_t)-1;

  for (size_t i = strv.len - substrv.len; i != (size_t)-1; i--) {
    if (strv_cmp(strv_substr(strv, i, substrv.len), substrv) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

static inline bool strv_starts_with(const Strv strv, const Strv prefix) {
  if (strv.len < prefix.len) {
    return false;
  }
  return strv_cmp(strv_substr(strv, 0, prefix.len), prefix) == 0;
}

static inline bool strv_ends_with(const Strv strv, const Strv suffix) {
  if (strv.len < suffix.len) {
    return false;
  }
  return strv_cmp(strv_substr(strv, strv.len - suffix.len, suffix.len),
                  suffix) == 0;
}

static inline Strv strv_trim_left(const Strv strv) {
  size_t start = 0;
  while (start < strv.len &&
         (strv.buf[start] == ' ' || strv.buf[start] == '\t')) {
    start++;
  }
  return strv_substr(strv, start, strv.len - start);
}

static inline Strv strv_trim_right(const Strv strv) {
  size_t end = strv.len;
  while (end > 0 && (strv.buf[end - 1] == ' ' || strv.buf[end - 1] == '\t')) {
    end--;
  }
  return strv_substr(strv, 0, end);
}

static inline Strv strv_trim(const Strv strv) {
  return strv_trim_right(strv_trim_left(strv));
}

static inline Strv *strv_split(const Strv strv, const char delim) {
  Strv *parts = NULL;
  size_t start = 0;

  darr_init(parts);

  if (strv.len == 0)
    return parts;

  for (size_t i = 0; i < strv.len; ++i) {
    if (strv.buf[i] == delim) {
      Strv sub_str = strv_substr(strv, start, i - start);
      darr_append(parts, sub_str);
      start = i + 1;
    }
  }

  Strv sub_strv = strv_substr(strv, start, strv.len - start);
  if (sub_strv.len > 0 || strv.buf[strv.len - 1] == delim) {
    darr_append(parts, sub_strv);
  }

  return parts;
}

static inline Strv *arena_strv_split(Arena *arena, const Strv strv,
                                     const char delim) {
  (void)arena;
  Strv *parts = NULL;
  size_t start = 0;

  arena_darr_init(arena, parts);

  if (strv.len == 0)
    return parts;

  for (size_t i = 0; i < strv.len; ++i) {
    if (strv.buf[i] == delim) {
      Strv sub_str = strv_substr(strv, start, i - start);
      arena_darr_append(arena, parts, sub_str);
      start = i + 1;
    }
  }

  Strv sub_strv = strv_substr(strv, start, strv.len - start);
  if (sub_strv.len > 0 || strv.buf[strv.len - 1] == delim) {
    arena_darr_append(arena, parts, sub_strv);
  }

  return parts;
}

static inline Strv *arena_strv_split_str(Arena *arena, const Strv strv,
                                         const Strv delim) {
  Strv *parts = NULL;
  size_t start = 0;

  arena_darr_init(arena, parts);

  if (delim.len == 0)
    return parts;

  while (start <= strv.len) {
    size_t idx = strv_find(strv_substr(strv, start, strv.len - start), delim);
    if (idx == (size_t)-1) {
      break;
    }
    Strv sub_strv = strv_substr(strv, start, idx);
    arena_darr_append(arena, parts, sub_strv);
    start += idx + delim.len;
  }

  Strv sub_strv = strv_substr(strv, start, strv.len - start);
  if (sub_strv.len > 0 ||
      (strv.len >= delim.len &&
       strv_cmp(strv_substr(strv, strv.len - delim.len, delim.len), delim) ==
           0)) {
    arena_darr_append(arena, parts, sub_strv);
  }

  return parts;
}

static inline Strv *arena_strv_split_any(Arena *arena, const Strv strv,
                                         const Strv char_set) {
  Strv *parts = NULL;
  size_t start = 0;

  arena_darr_init(arena, parts);

  for (size_t i = 0; i < strv.len; ++i) {
    if (strv_find_char(char_set, strv.buf[i]) != (size_t)-1) {
      Strv sub_strv = strv_substr(strv, start, i - start);
      arena_darr_append(arena, parts, sub_strv);
      start = i + 1;
    }
  }

  Strv sub_strv = strv_substr(strv, start, strv.len - start);
  if (sub_strv.len > 0 ||
      (strv.len > 0 &&
       strv_find_char(char_set, strv.buf[strv.len - 1]) != (size_t)-1)) {
    arena_darr_append(arena, parts, sub_strv);
  }

  return parts;
}

static inline Strv *strv_split_str(const Strv strv, const Strv delim) {
  Strv *parts = NULL;
  size_t start = 0;

  darr_init(parts);

  if (delim.len == 0)
    return parts;

  while (start <= strv.len) {
    size_t idx = strv_find(strv_substr(strv, start, strv.len - start), delim);
    if (idx == (size_t)-1) {
      break;
    }
    Strv sub_strv = strv_substr(strv, start, idx);
    darr_append(parts, sub_strv);
    start += idx + delim.len;
  }

  Strv sub_strv = strv_substr(strv, start, strv.len - start);
  if (sub_strv.len > 0 ||
      (strv.len >= delim.len &&
       strv_cmp(strv_substr(strv, strv.len - delim.len, delim.len), delim) ==
           0)) {
    darr_append(parts, sub_strv);
  }

  return parts;
}

static inline Strv *strv_split_any(const Strv strv, const Strv char_set) {
  Strv *parts = NULL;
  size_t start = 0;

  for (size_t i = 0; i < strv.len; ++i) {
    if (strv_find_char(char_set, strv.buf[i]) != (size_t)-1) {
      Strv sub_strv = strv_substr(strv, start, i - start);
      darr_append(parts, sub_strv);
      start = i + 1;
    }
  }

  Strv sub_strv = strv_substr(strv, start, strv.len - start);
  if (sub_strv.len > 0 ||
      (strv.len > 0 &&
       strv_find_char(char_set, strv.buf[strv.len - 1]) != (size_t)-1)) {
    darr_append(parts, sub_strv);
  }

  return parts;
}

#endif // !C_UTILS_STRING_VIEWS
