#ifndef ATOMIC_H
#define ATOMIC_H

#include "common.h"
#include "log_time.h"
#include "log_levels.h"

#define MAX_LOG_LINE_LEN 1024

typedef int(*log_init)();
typedef int(*log_close)(void);
typedef void(*log_print)(lgg_time *, log_lvl, uint16_t, const char *, const char *, const char *, va_list);

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
// Atomic loggers functions

static inline void common_lgg_print(FILE *ostream, lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr);

extern void console_lgg_print(lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr);

extern FILE *file_lgg_output;
extern int file_lgg_init(const char *log_path, const char *log_name, int max_files);
extern void file_lgg_print(lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr);
extern int file_lgg_close();

#endif // ATOMIC_H
