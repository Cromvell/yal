/*
Logger TODO:

  + Logger initialization system
  + Change log level ability
  + Logging function
  + Enable writing into different streams
  + Code refactoring
  + Separate src into different files
  + Print file, line and function in log
  + Increase timestamp pricesion. Capture it immediately on log call and then store
  + Enable port to linux (and Mac OS in future(but who cares?))

  - Add custom level creation and categories
  - Enable config files
  - Option "Max file count"

*/

#include "logger.h"

#define __FILENAME__ (strrchr(__FILE__, P_PATH_SLASH) ? strrchr(__FILE__, P_PATH_SLASH) + 1 : __FILE__)

//////////////////////////////////////////////////////////////////
// External interface
#define LOG_INIT(...) (logger__init(__VA_ARGS__))
#define LOG(lgg, lvl, msg, ...) (print__log((lgg), (lvl), (uint16_t)__LINE__, (strrchr(__FILE__, P_PATH_SLASH) ? strrchr(__FILE__, P_PATH_SLASH) + 1 : __FILE__), __FUNCTION__, (msg), ## __VA_ARGS__))
#define LOG_CLOSE(lgg) (logger__close(lgg))
#define SET_LOG_LVL(lgg, lvl) (set__log__lvl(lgg, lvl))

#include "test.c"

int main(int argc, char **argv) {
    test_main();
    //getchar();
}
