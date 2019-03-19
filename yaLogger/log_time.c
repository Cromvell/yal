#include "log_time.h"

//////////////////////////////////////////////////////////////////
// Message preprocessing

// Date and time functions
char *get_datetime_str(lgg_time *time) {
    char *timeline, year[5], *tmp;

    if (time == NULL)
        return "0";

    // Format time
    tmp = ctime(time);
    strncpy(year, tmp + 20, 4);
    year[4] = '\0';

    timeline = (char *)xmalloc(strlen(tmp) * sizeof(char));

    // Assemble final string and cut off unused parts
    sprintf(timeline, "%s %.*s.%03d", year, 18 - 3, tmp + 4, time->millitm);

    return timeline;
}
