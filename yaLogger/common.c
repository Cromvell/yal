#include "common.h"

void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}

void *xrealloc(void *ptr, size_t num_bytes) {
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        perror("xrealloc failed");
        exit(1);
    }
    return ptr;
}

void *xmalloc(size_t num_bytes) {
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

BOOL file_exists(TCHAR * file) {
    WIN32_FIND_DATA find_file_data;
    HANDLE handle = FindFirstFile(file, &find_file_data);
    int found = handle != INVALID_HANDLE_VALUE;
    if (found) {
        FindClose(handle);
    }
    return found;
}

BOOL dir_exists(TCHAR * sz_path) {
    DWORD dw_attrib = GetFileAttributes(sz_path);
    return (dw_attrib != INVALID_FILE_ATTRIBUTES &&
           (dw_attrib & FILE_ATTRIBUTE_DIRECTORY));
}

void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
    assert(buf_cap(buf) <= (SIZE_MAX - 1) / 2);
    size_t new_cap = CLAMP_MIN(2 * buf_cap(buf), MAX(new_len, 16));
    assert(new_len <= new_cap);
    assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf)) / elem_size);
    size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
    BufHdr *new_hdr;
    if (buf) {
        new_hdr = xrealloc(buf__hdr(buf), new_size);
    }
    else {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}