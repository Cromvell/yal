#include "logger.h"
#include "log_time.h"
#include "atomic.h"

void add__atomic__lgg(logger *lgg, atom_lgg_type type, log_init init_func, log_print print_func, log_close close_func) {
    assert(print_func != NULL);
    buf_push(lgg->atom_buf, (atom_lgg) { type, init_func, print_func, close_func });
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
        lgg->conf->log_name = "yaLogger";
        lgg->conf->log_path = p_getcwd(NULL, 0);
        lgg->conf->verbosity = DEBUG_L;
        lgg->conf->max_files = 0;
    }
    lgg->atom_buf = NULL;
    lgg->module_buf = NULL;
    CAPTURE_TIME(lgg); // Initialize llg->timestamp

    // Add atomic loggers
    add__atomic__lgg(lgg, CONSOLE_LGG, NULL, console_lgg_print, NULL);
    add__atomic__lgg(lgg, FILE_LGG, file_lgg_init, file_lgg_print, file_lgg_close);

    // Init all added atomic loggers (currently only file logger)
    assert(lgg->atom_buf != NULL);
    for (i = 0; i < buf_len(lgg->atom_buf); i++) {
        if (lgg->atom_buf[i].init != NULL &&
            lgg->atom_buf[i].type == FILE_LGG)
        {
            if (lgg->atom_buf[i].init(lgg->conf->log_path, lgg->conf->log_name, lgg->conf->max_files)) {
                buf_free(lgg->atom_buf);
                buf_free(lgg->module_buf);
                free(lgg);
                return NULL;
            }
        }
    }

    return lgg;
}

void print__log(logger *lgg, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, ...) {
    va_list args;
    int i;

    if (lgg == NULL && (lgg = logger__init(NULL)) == NULL) {
        fatal("Logger initialization failed");
    } else {
        // If logger initialized - capture time, if not - time will be captured during init
        CAPTURE_TIME(lgg);
    }

    assert(lgg->atom_buf != NULL);
    va_start(args, fmt);
    for (i = 0; i < buf_len(lgg->atom_buf); i++) {
        if (lgg->atom_buf[i].print != NULL && level <= lgg->conf->verbosity)
            lgg->atom_buf[i].print(&(lgg->timestamp), level, line, file, func, fmt, args);
    }
    va_end(args);
}

int logger__close(logger *lgg) {
	int exitcode = 0;

    if (lgg != NULL) {
        if (lgg->atom_buf != NULL) {
            int i;

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

    return exitcode;
}

void set__log__lvl(logger *lgg, log_lvl level) {
    // Not allow user set UNKNOWN log level directly
    if (level >= FATAL_L && level <= NOTSET_L)
        lgg->conf->verbosity = level;
    else
        lgg->conf->verbosity = UNKNOWN_L;
}
