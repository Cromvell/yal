// Windows specific headers
#include <windows.h>
#include <tchar.h>

// Portable headers
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

/*
Logger TODO:

  + Logger initialization system
  + Change log level ability
  + Logging function
  + Enable writing into different streams
  + Code refactoring

  - Separate src into different files
  - Enable port to linux (and Mac OS in future(but who cares?))
  - Create different process and/or thread for logger
  - Add custom level creation and categories
  - Enable config files

*/

// Common stuff

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)

void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}

void *xcalloc(size_t num_elems, size_t elem_size) {
    void *ptr = calloc(num_elems, elem_size);
    if (!ptr) {
        perror("xcalloc failed");
        exit(1);
    }
    return ptr;
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

int file_exists(TCHAR * file) {
  WIN32_FIND_DATA find_file_data;
  HANDLE handle = FindFirstFile(file, &find_file_data);
  int found = handle != INVALID_HANDLE_VALUE;
  if(found) {
    FindClose(handle);
  }
  return found;
}

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

void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
    assert(buf_cap(buf) <= (SIZE_MAX - 1)/2);
    size_t new_cap = CLAMP_MIN(2*buf_cap(buf), MAX(new_len, 16));
    assert(new_len <= new_cap);
    assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf))/elem_size);
    size_t new_size = offsetof(BufHdr, buf) + new_cap*elem_size;
    BufHdr *new_hdr;
    if (buf) {
        new_hdr = xrealloc(buf__hdr(buf), new_size);
    } else {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

// Dumb logger

#define MAX_LOG_LINE_LEN 1024

typedef enum {
    ERROR_L,
    WARNING_L,
    INFO_L,
    DEBUG_L
} log_lvl;

char *log_level_to_str(log_lvl level) {
    switch(level) {
    case ERROR_L:
        return "[ERROR]";
    case WARNING_L:
        return " [WARN]";
    case INFO_L:
        return " [INFO]";
    case DEBUG_L:
        return "[DEBUG]";
    default:
        fatal("Unknown log level identifier: %d", level);
        break;
    }
}

typedef int (*log_init)();
typedef int (*log_close)();
typedef void (*log_print)(log_lvl, const char *, va_list args);

typedef enum {
    CONSOLE_LGG,
    FILE_LGG
} atom_lgg_type;

typedef struct atomic_lgg {
    atom_lgg_type type;
    log_init init;
    log_print print;
    log_close close;
} atom_lgg;

// Message preprocessing

// Date and time functions
// TODO: Make more appropriate datetime format
char *get_datetime_str() {
    struct timeb now;
    char *timeline, *endstr;

    // Get time in predefined format
    _ftime(&now);
    timeline = ctime(&(now.time));

    // Remove '\n' on line's end
    endstr = timeline;
    while (*endstr != '\0') endstr++;
    *(endstr - 1) = '\0';

    return timeline;
}


// Atomic loggers functions
void common_lgg_print(FILE *ostream, log_lvl level, const char *fmt, va_list argptr) {
    char *msg[MAX_LOG_LINE_LEN];
    static const char *warn = "... !!! WARNING !!! Message was truncated!";
    int required_len;

    // Assemble user formated string
    required_len = vsnprintf(msg, MAX_LOG_LINE_LEN, fmt, argptr);

    // And then print it in logger wrapped format
    fprintf(ostream, "%s %s: %s%s\n", get_datetime_str(), log_level_to_str(level), msg, required_len >= MAX_LOG_LINE_LEN ? warn : "");
}

static inline void console_lgg_print(log_lvl level, const char *fmt, va_list argptr) {
    common_lgg_print(stdout, level, fmt, argptr);
}

// TEMP: Assign path on logger init
const char *log_path = "C:\\Users\\Cromvell\\source\\repos\\yaLogger\\bin\\"; // DIRTY
const char *log_name = "testlog";

static FILE *file_lgg_output = NULL;
int file_lgg_init() {
    int log_n = 0;
    char *buf = (char *)malloc(MAX_PATH * sizeof(char));
    
    // Set initial log filename
    sprintf(buf, "%s%s.log", log_path, log_name);

    while (file_exists(buf)) {
        // Change filename if previous already exists
        sprintf(buf, "%s%s.%d.log", log_path, log_name, log_n);
        log_n++;
    }
  
    file_lgg_output = fopen(buf, "w");
    if (file_lgg_output == NULL) {
        return 1;
    } else {
        return 0;
    }
}

static inline void file_lgg_print(log_lvl level, const char *fmt, va_list argptr) {
    common_lgg_print(file_lgg_output, level, fmt, argptr);
}

int file_lgg_close() {
    return fclose(file_lgg_output);
}

void atomic_loggers_test() {
    console_lgg_print(ERROR_L, "Just a test message. Error! Praise youselves!", NULL);
    console_lgg_print(WARNING_L, "Second test message. Just warn you", NULL);
    console_lgg_print(INFO_L, "One more test message. This is info", NULL);
    console_lgg_print(DEBUG_L, "Yes. Test message. The last one. This time debug", NULL);

    file_lgg_init();
    file_lgg_print(ERROR_L, "Just a test message. Error! Praise youselves!", NULL);
    file_lgg_print(WARNING_L, "Second test message. Just warn you", NULL);
    file_lgg_print(INFO_L, "One more test message. This is info", NULL);
    file_lgg_print(DEBUG_L, "Yes. Test message. The last one, I promice. This time debug", NULL);
    file_lgg_print(DEBUG_L, "Very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long message.", NULL);
    file_lgg_close();
}

// Logger itself
static atom_lgg *lgg_buf = NULL;
static log_lvl glob_log_level = DEBUG_L;

void add__atomic__lgg(atom_lgg_type type, log_init init_func, log_print print_func, log_close close_func) {
    assert(print_func != NULL);
    buf_push(lgg_buf, (atom_lgg){ type, init_func, print_func, close_func });
}

// TODO: Init logger in another process and/or thread
static bool logger_initialized = false;
int logger__init() {
    if (!logger_initialized) {
        int i;
  
        add__atomic__lgg(CONSOLE_LGG, NULL, console_lgg_print, NULL);
        add__atomic__lgg(FILE_LGG, file_lgg_init, file_lgg_print, file_lgg_close);

        assert(lgg_buf != NULL);
        for (i = 0; i < buf_len(lgg_buf); i++) {
            if (lgg_buf[i].init != NULL && lgg_buf[i].init())
                return 1;
        }

        logger_initialized = true;
    }

    return 0;
}

static inline void print__log(log_lvl level, const char *fmt, ...) {
    va_list args;
    int i;
  
    if (!logger_initialized && logger__init()) {
        fatal("Logger initialization error");
    }

    assert(lgg_buf != NULL);
    va_start(args, fmt);
    for (i = 0; i < buf_len(lgg_buf); i++) {
        if (lgg_buf[i].print != NULL && level <= glob_log_level)
            lgg_buf[i].print(level, fmt, args);
    }
    va_end(args);
}

int logger__close() {
    if (lgg_buf != NULL) {
        int i;
        int result = 0;
    
        for (i = 0; i < buf_len(lgg_buf); i++) {
            if (lgg_buf[i].close != NULL)
                lgg_buf[i].close();
        }
    }
}

static inline int setlog__lvl(log_lvl level) {
    if (level >= ERROR_L && level <= DEBUG_L)
        glob_log_level = level;
    else
        fatal("Unknown log level identifier: %d", level);
}

// External interface
#define LOG_INIT() (logger__init())
#define LOG(lvl, msg, ...) (print__log((lvl), (msg), __VA_ARGS__))
#define LOG_CLOSE() (logger__close())
#define SET_LOG_LVL(lvl) (setlog__lvl(lvl))

void logger_test() {
    LOG(ERROR_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(WARNING_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(INFO_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(DEBUG_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    SET_LOG_LVL(WARNING_L);
    LOG(INFO_L, "Just an info message that will never be loged.");
}

int main(int argc, char **argv) {
    atomic_loggers_test();
    logger_test();
}
