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
// Message preprocessing

// Date and time functions
char *get_datetime_str() {
    struct timeb now;
    char *timeline, year[5], *tmp;

    // Get time in predefined format
    // NOTE: This is unholy shit! I capture time in string format oriented
    // function. Besides this is NOT the time of actual log, it is also
    // different for each atomic call in one log call. So, I must fix this
    _ftime(&now); // TODO: Capture time in the right way
    tmp = ctime(&(now.time));
    strncpy(year, tmp + 20, 4);
    year[4] = '\0';

    timeline = (char *)xmalloc(strlen(tmp) * sizeof(char));

    // Assemble final string and cut off unused parts
    sprintf(timeline, "%s %.*s.%03.d", year, 18 - 3, tmp + 4, now.millitm);

    return timeline;
}

//////////////////////////////////////////////////////////////////
// Atomic loggers functions
static inline void common_lgg_print(FILE *ostream, log_lvl level, uint16_t line, const char *file_path, const char *func, const char *fmt, va_list argptr) {
    char msg_buf[MAX_LOG_LINE_LEN];
    static const char *warn = "... !!! WARNING !!! Message was truncated!";

    // Assemble user formated string
    int required_len = vsnprintf(msg_buf, MAX_LOG_LINE_LEN, fmt, argptr);

    // Cut off filename from path
    char *end = file_path + strlen(file_path);
    char *ps = end;
    while (*(ps-1) != '\\')
        ps--;
    char *file = (char *)xmalloc((end - ps) * sizeof(char));
    strncpy(file, ps, (end - ps));

    char *timeline = get_datetime_str();
    char *loglvl = log_level_to_str(level);
    // And then print it in logger wrapped format
    // TODO: Make file, line and func optional
    fprintf(ostream, "%s [%-5s] {%s:%d} {%s()} %s%s\n", timeline, loglvl, file, line, func, msg_buf, required_len >= MAX_LOG_LINE_LEN ? warn : "");
}

FILE *file_lgg_output = NULL; // Output file handle

inline void console_lgg_print(log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr) {
    common_lgg_print(stdout, level, line, file, func, fmt, argptr);
}

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
    // TODO: Change files naming by always adding number suffix
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
    }
    else {
        return 0;
    }
}

inline void file_lgg_print(log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr) {
    common_lgg_print(file_lgg_output, level, line, file, func, fmt, argptr);
}

int file_lgg_close() {
    return fclose(file_lgg_output);
}