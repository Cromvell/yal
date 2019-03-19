#ifndef LOG_TIME_H
#define LOG_TIME_H

#include "common.h"

typedef struct timeb lgg_time;

#define CAPTURE_TIME(lgg) (_ftime(&((lgg)->timestamp)))

char *get_datetime_str(lgg_time *);

#endif // LOG_TIME_H