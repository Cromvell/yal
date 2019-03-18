#ifndef COMMON_H
#define COMMON_H

#include "platform.h"

#include <stdio.h>
#include <string.h>
#include <sys\timeb.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)

void fatal(const char *fmt, ...);

void *xrealloc(void *ptr, size_t num_bytes);
void *xmalloc(size_t num_bytes);

BOOL file_exists(TCHAR * file);
BOOL dir_exists(TCHAR * sz_path);

// Stretchy buffer

typedef struct BufHdr {
    size_t len;
    size_t cap;
    char buf[];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)(b) - offsetof(BufHdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b)*sizeof(*b) : 0)

#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)
#define buf_fit(b, n) ((n) <= buf_cap(b) ? 0 : ((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...) (buf_fit((b), 1 + buf_len(b)), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_clear(b) ((b) ? buf__hdr(b)->len = 0 : 0)

void *buf__grow(const void *buf, size_t new_len, size_t elem_size);

#endif