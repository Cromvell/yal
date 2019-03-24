#ifndef LOG_LEVELS_H
#define LOG_LEVELS_H

typedef enum {
    FATAL_L,
    ALERT_L,
    CRIT_L,
    ERROR_L,
    WARN_L,
    NOTICE_L,
    INFO_L,
    DEBUG_L,
    NOTSET_L,
    UNKNOWN_L
} log_lvl;

char *log_level_to_str(log_lvl level);

#endif // LOG_LEVELS_H
