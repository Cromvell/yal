#ifndef LOGGER_H
#define LOGGER_H

#include "atomic.h"


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
    struct timeb timestamp;
} logger;

//////////////////////////////////////////////////////////////////
// Logger interaction functions

void add__atomic__lgg(logger *lgg, atom_lgg_type type, log_init init_func, log_print print_func, log_close close_func);

logger *logger__init(lgg_conf *params);

void print__log(logger *lgg, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, ...);

int logger__close(logger *lgg);

int set__log__lvl(logger *lgg, log_lvl level);

#endif
