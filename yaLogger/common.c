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

#ifdef OS_WINDOWS

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

int ffdcmp(const void *ffd1, const void *ffd2) {
    WIN32_FIND_DATA *ff1_ = (WIN32_FIND_DATA *)ffd1;
    WIN32_FIND_DATA *ff2_ = (WIN32_FIND_DATA *)ffd2;
    uint64_t n1 = extract_log_num(ff1_->cFileName);
    uint64_t n2 = extract_log_num(ff2_->cFileName);

    if (n1 == n2)
        return 0;
    else if (n1 < n2)
        return -1;
    else
        return 1;
}

#endif
#ifdef OS_LINUX

bool file_exists(const char* filename) {
	struct stat buffer;
	int exist = stat(filename, &buffer);
	return exist == 0 ? true : false;
}

bool dir_exists(const char* dirname) {
	DIR *dir = opendir(dirname);
	if (dir)
		return true;
	else if (ENOENT == errno)
		return false;
	else
		fatal("Directory exists check fails");
}

int lognamecmp(const void *f1, const void *f2) {
	const char *f1_ = *(const char **)f1;
	const char *f2_ = *(const char **)f2;
    uint64_t n1 = extract_log_num(f1_);
    uint64_t n2 = extract_log_num(f2_);

	if (n1 == n2)
		return 0;
	else if (n1 < n2)
		return -1;
	else
		return 1;
}

#endif

bool starts_with(const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

bool ends_with(const char *str, const char *end) {
    size_t lstr = strlen(str);
    size_t lend = strlen(end);
    if (lstr >= lend) {
        // Point str to where end should start and compare the strings from there
        return (0 == memcmp(end, str + (lstr - lend), lend));
    }
    return false;
}

uint64_t extract_log_num(const char *filename) {
    char *start;
    char *end = filename + strlen(filename) - 1;
    char *ptr;
    int result_num;

    // Find first point from right
    ptr = end + 1;
    while (*(--ptr) != '.' && *ptr != '\0')
        ;

    // Find next point from right
    while (*(--ptr) != '.' && *ptr != '\0')
        ;
    start = ptr + 1;

    if (start == end) {
        // Error: there's nothing between dots
        return -1;
    } else {
        result_num = (uint64_t)strtoul(start, &end, 10);
        if (end == start) {
            // Error: can't convert string to number
            return -1;
        } else {
            return result_num;
        }
    }
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
