#include "atomic.h"

char *log_level_to_str(log_lvl level) {
    switch (level) {
    case ERROR_L:
        return "ERROR";
    case WARNING_L:
        return "WARN";
    case INFO_L:
        return "INFO";
    case DEBUG_L:
        return "DEBUG";
    default:
        fatal("Unknown log level identifier: %d", level);
        break;
    }
}

//////////////////////////////////////////////////////////////////
// Atomic loggers functions
static inline void common_lgg_print(FILE *ostream, lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr) {
    char msg_buf[MAX_LOG_LINE_LEN];
    static const char *warn = "... !!! WARNING !!! Message was truncated!";
    int required_len;
	char *timeline;
	char *loglvl;

    // Assemble user formated string
    if (argptr != NULL) {
    	va_list args_copy;
    	va_copy(args_copy, argptr); // Copy just in case if pointed-to structure will be changed
    	required_len = vsnprintf(msg_buf, MAX_LOG_LINE_LEN, fmt, args_copy);
    	va_end(args_copy);
    }
    else
    	required_len = snprintf(msg_buf, MAX_LOG_LINE_LEN, fmt);

    timeline = get_datetime_str(time);
    loglvl = log_level_to_str(level);
    // And then print it in logger wrapped format
    // TODO: Make file, line and func optional
    fprintf(ostream, "%s [%-5s] {%s:%d} {%s()} %s%s\n", timeline, loglvl, file, line, func, msg_buf, required_len >= MAX_LOG_LINE_LEN ? warn : "");
}

FILE *file_lgg_output = NULL; // Output file handle

void console_lgg_print(lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr) {
    common_lgg_print(stdout, time, level, line, file, func, fmt, argptr);
}

int file_lgg_init(const char *log_path, const char *log_name) {
    int log_n = 0;
    char *buf;

    // Check if path exists
    if (!dir_exists(log_path)) {
        return 1;
    }

    if (log_path[strlen(log_path) - 1] != P_PATH_SLASH) {
        strcat(log_path, P_PATH_SLASH_STR);
    }

    // Set initial log filename
    // TODO: Change files naming by always adding number suffix
    buf = (char *)xmalloc(P_MAX_PATH * sizeof(char));
    sprintf(buf, "%s%s.log", log_path, log_name);

    while (file_exists(buf)) {
        // Change filename if previous already exists
        sprintf(buf, "%s%s.%d.log", log_path, log_name, log_n);
        log_n++;
    }

    file_lgg_output = fopen(buf, "w");
    if (file_lgg_output == NULL) {
        return 1;
    }
    else {
        return 0;
    }
}

void file_lgg_print(lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr) {
    common_lgg_print(file_lgg_output, time, level, line, file, func, fmt, argptr);
}

int file_lgg_close() {
    return fclose(file_lgg_output);
}
