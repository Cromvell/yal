#ifndef LOGGER_H
#define LOGGER_H

#include "atomic.h"

extern inline void console_lgg_print(log_lvl level, const char *fmt, va_list argptr);

extern int file_lgg_init(const char *log_path, const char *log_name);
extern inline void file_lgg_print(log_lvl level, const char *fmt, va_list argptr);
extern int file_lgg_close();

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

//////////////////////////////////////////////////////////////////
// Logger interaction functions

extern void add__atomic__lgg(logger *lgg, atom_lgg_type type, log_init init_func, log_print print_func, log_close close_func);

extern logger *logger__init(lgg_conf *params);

extern void print__log(logger *lgg, log_lvl level, const char *fmt, ...);

extern int logger__close(logger *lgg);

extern inline int set__log__lvl(logger *lgg, log_lvl level);

#endif