YAL is simple cross-platform logger for Windows and Linux. Examples of usage:

```C
// Logger initialization (later it can be done much better with configs)
//    log_path  - path to folder where logs will stored
//    log_name  - name of log files
//    UNKNOWN_L - verbosity level
//    4         - maximum log file number
logger *lgg = LOG_INIT(&(lgg_conf) { log_path, log_name, UNKNOWN_L, 4 });
// Or it can be done with default settings
logger *lgg = LOG_INIT(NULL);

// Logging
LOG(lgg, CRIT_L, "OMG! It's a critical message!");
LOG(lgg, ERROR_L, "Oups, error message. Call admin: %s", "32-85-12");
LOG(lgg, INFO_L, "Just remember to you that e number equal to %f...", 2.718281828);
LOG(lgg, DEBUG_L, "Pass by. It's just debug message.");
SET_LOG_LVL(lgg, WARN_L);
LOG(lgg, INFO_L, "You don't see this message because of its verbosity.");

// Close logger
LOG_CLOSE(lgg);
```

Result of this log will be show in standard terminal output and also write to log file:
```
2019 Apr 29 20:01:18.615 [CRIT ] {test.c:37} {logger_test()} OMG! It's a critical message!
2019 Apr 29 20:01:18.616 [ERROR] {test.c:38} {logger_test()} Oups, error message. Call admin: 32-85-12
2019 Apr 29 20:01:18.616 [INFO ] {test.c:39} {logger_test()} Just remember to you that e number equal to 2.718282...
2019 Apr 29 20:01:18.617 [DEBUG] {test.c:40} {logger_test()} Pass by. It's just debug message.
```
