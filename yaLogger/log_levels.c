#include "log_levels.h"

char *log_level_to_str(log_lvl level) {
    switch (level) {
    case FATAL_L:
        return "FATAL";
    case ALERT_L:
        return "ALERT";
    case CRIT_L:
        return "CRIT";
    case ERROR_L:
        return "ERROR";
    case WARN_L:
        return "WARN";
    case NOTICE_L:
        return "NOTE";
    case INFO_L:
        return "INFO";
    case DEBUG_L:
        return "DEBUG";
    case NOTSET_L:
        return "N/S";
    default:
    case UNKNOWN_L:
        return "UNK";
    }
}