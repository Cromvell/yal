// Windows specific headers
#include <windows.h>
#include <tchar.h>
#include <direct.h>

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
  - Add custom level creation and categories
  - Enable config files
  - Option "Max file count"

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
    if(found) {
        FindClose(handle);
    }
    return found;
}

BOOL dir_exists(TCHAR * sz_path) {
    DWORD dw_attrib = GetFileAttributes(sz_path);
    return (dw_attrib != INVALID_FILE_ATTRIBUTES &&
           (dw_attrib & FILE_ATTRIBUTE_DIRECTORY));
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

//////////////////////////////////////////////////////////////////
//
// Dumb logger
//

#define TEST__MODE 1

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
typedef int (*log_close)(void);
typedef void (*log_print)(log_lvl, const char *, va_list);

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

//////////////////////////////////////////////////////////////////
// Message preprocessing

// Date and time functions
char *get_datetime_str() {
    struct timeb now;
    char *timeline, year[5], *tmp;

    // Get time in predefined format
    _ftime(&now);
    tmp = ctime(&(now.time));
    strncpy(year, tmp + 20, 4);
    year[4] = '\0';
    
    timeline = (char *)xmalloc(strlen(tmp) * sizeof(char));

    // Assemble final string and cut off unused parts
    sprintf(timeline, "%s %.*s.%3.d", year, 18 - 3, tmp + 4, now.millitm);

    return timeline;
}

//////////////////////////////////////////////////////////////////
// Atomic loggers functions
void common_lgg_print(FILE *ostream, log_lvl level, const char *fmt, va_list argptr) {
    char msg[MAX_LOG_LINE_LEN];
    static const char *warn = "... !!! WARNING !!! Message was truncated!";
    int required_len;

    // Assemble user formated string
    required_len = vsnprintf(msg, MAX_LOG_LINE_LEN, fmt, argptr);

    char *timeline = get_datetime_str();
    char *loglvl = log_level_to_str(level);
    // And then print it in logger wrapped format
    fprintf(ostream, "%s %s: %s%s\n", timeline, loglvl, msg, required_len >= MAX_LOG_LINE_LEN ? warn : "");
}

static inline void console_lgg_print(log_lvl level, const char *fmt, va_list argptr) {
    common_lgg_print(stdout, level, fmt, argptr);
}

static FILE *file_lgg_output = NULL;
int file_lgg_init(const char *log_path, const char *log_name) {
    int log_n = 0;
    char *buf;

    // Check if path exists
    if (!dir_exists(log_path)) {
        return 1;
    }

    if (log_path[strlen(log_path) - 1] != '\\') {
        strcat(log_path, "\\");
    }
    
    // Set initial log filename
    buf = (char *)xmalloc(MAX_PATH * sizeof(char));
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

    if (file_lgg_init(TEST__MODE ? "C:\\Users\\Cromvell\\source\\repos\\yaLogger\\bin\\" : _getcwd(NULL, 0), "testlog")) {
        fatal("Atomic file logger init error");
    }
    file_lgg_print(ERROR_L, "Just a test message. Error! Praise youselves!", NULL);
    file_lgg_print(WARNING_L, "Second test message. Just warn you", NULL);
    file_lgg_print(INFO_L, "One more test message. This is info", NULL);
    file_lgg_print(DEBUG_L, "Yes. Test message. The last one, I promice. This time debug", NULL);
    file_lgg_print(DEBUG_L, "Very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long message.", NULL);
    file_lgg_close();
}

//////////////////////////////////////////////////////////////////
// Logger itself
typedef struct {
    char *log_path;
    char *log_name;
    log_lvl verbosity;
} lgg_conf;

typedef struct {
    char *name;
    log_lvl verbosity;
} lgg_module;

typedef struct {
    lgg_conf *conf;
    atom_lgg *atom_buf;
    lgg_module *module_buf;
} logger;


void add__atomic__lgg(logger *lgg, atom_lgg_type type, log_init init_func, log_print print_func, log_close close_func) {
    assert(print_func != NULL);
    buf_push(lgg->atom_buf, (atom_lgg){ type, init_func, print_func, close_func });
}

logger *logger__init(lgg_conf *params) {
    int i;

    logger *lgg = (logger *)malloc(sizeof(logger));
    if (lgg == NULL) {
        return NULL;
    }
    if (params != NULL)
        lgg->conf = params;
    else {
        lgg->conf = (lgg_conf *)malloc(sizeof(lgg_conf));
        if (lgg->conf == NULL) {
            free(lgg);
            return NULL;
        }

        // Default logger settings
        lgg->conf->log_name = "yaLogger"; // TODO: Make name more original
        lgg->conf->log_path = _getcwd(NULL, 0);
        lgg->conf->verbosity = DEBUG_L;
    }
    lgg->atom_buf = NULL;
    lgg->module_buf = NULL;

    // Add atomic loggers
    add__atomic__lgg(lgg, CONSOLE_LGG, NULL, console_lgg_print, NULL);
    add__atomic__lgg(lgg, FILE_LGG, file_lgg_init, file_lgg_print, file_lgg_close);

    // Init all added atomic loggers (currently only file logger)
    assert(lgg->atom_buf != NULL);
    for (i = 0; i < buf_len(lgg->atom_buf); i++) {
        if (lgg->atom_buf[i].init != NULL &&
            lgg->atom_buf[i].type == FILE_LGG)
        {
            if (lgg->atom_buf[i].init(lgg->conf->log_path, lgg->conf->log_name)) {
                buf_free(lgg->atom_buf);
                buf_free(lgg->module_buf);
                free(lgg);
                return NULL;
            }
        }
    }

    return lgg;
}

void print__log(logger *lgg, log_lvl level, const char *fmt, ...) {
    va_list args;
    int i;

    if (lgg == NULL && (lgg = logger__init(NULL)) == NULL) {
        fatal("Logger initialization failed");
    }

    assert(lgg->atom_buf != NULL);
    va_start(args, fmt);
    for (i = 0; i < buf_len(lgg->atom_buf); i++) {
        if (lgg->atom_buf[i].print != NULL && level <= lgg->conf->verbosity)
            lgg->atom_buf[i].print(level, fmt, args);
    }
    va_end(args);
}

int logger__close(logger *lgg) {
    if (lgg != NULL) {
        if (lgg->atom_buf != NULL) {
            int i;
            int result = 0;

            for (i = 0; i < buf_len(lgg->atom_buf); i++) {
                if (lgg->atom_buf[i].close != NULL)
                    lgg->atom_buf[i].close();
            }
        }

        buf_free(lgg->atom_buf);
        buf_free(lgg->module_buf);
        free(lgg);

        lgg = NULL;
    }
}

static inline int set__log__lvl(logger *lgg, log_lvl level) {
    if (level >= ERROR_L && level <= DEBUG_L)
        lgg->conf->verbosity = level;
    else
        // TODO: Assign UNKNOWN_L when it will be added
        fatal("Unknown log level identifier: %d", level);
}

//////////////////////////////////////////////////////////////////
// External interface
#define LOG_INIT(...) (logger__init(__VA_ARGS__))
#define LOG(lgg, lvl, msg, ...) (print__log((lgg), (lvl), (msg), __VA_ARGS__))
#define LOG_CLOSE(lgg) (logger__close(lgg))
#define SET_LOG_LVL(lgg, lvl) (set__log__lvl(lgg, lvl))

// Logger parameters
const char *log_path = "C:\\Users\\Cromvell\\source\\repos\\yaLogger\\bin\\";
const char *log_name = "testlog";

void logger_test() {
    logger *lgg = LOG_INIT(&(lgg_conf){ log_path, log_name, DEBUG_L });
    LOG(lgg, ERROR_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, WARNING_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, INFO_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, DEBUG_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    SET_LOG_LVL(lgg, WARNING_L);
    LOG(lgg, INFO_L, "Just an info message that will never be loged.");
    
    LOG_CLOSE(lgg);
}

int main(int argc, char **argv) {
    atomic_loggers_test();
    logger_test();
    //getchar();
}
