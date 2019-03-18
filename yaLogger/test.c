void atomic_loggers_test() {
    console_lgg_print(ERROR_L, "Just a test message. Error! Praise youselves!", NULL);
    console_lgg_print(WARNING_L, "Second test message. Just warn you", NULL);
    console_lgg_print(INFO_L, "One more test message. This is info", NULL);
    console_lgg_print(DEBUG_L, "Yes. Test message. The last one. This time debug", NULL);

    if (file_lgg_init(TEST__MODE ? "C:\\Users\\Cromvell\\source\\repos\\yaLogger\\bin\\" : _getcwd(NULL, 0), "testlog")) {
        fatal("Atomic file logger init error");
    }
    file_lgg_print(ERROR_L, "Just a test message. Error! Praise youselves!", NULL);
    file_lgg_print(WARNING_L, "Second test message. Just warn you", NULL);
    file_lgg_print(INFO_L, "One more test message. This is info", NULL);
    file_lgg_print(DEBUG_L, "Yes. Test message. The last one, I promice. This time debug", NULL);
    file_lgg_print(DEBUG_L, "Very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long message.", NULL);
    file_lgg_close();
}

// Logger parameters
const char *log_path = "C:\\Users\\Cromvell\\source\\repos\\yaLogger\\bin\\";
const char *log_name = "testlog";

void logger_test() {
    logger *lgg = LOG_INIT(&(lgg_conf) { log_path, log_name, DEBUG_L });
    LOG(lgg, ERROR_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, WARNING_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, INFO_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, DEBUG_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    SET_LOG_LVL(lgg, WARNING_L);
    LOG(lgg, INFO_L, "Just an info message that will never be loged.");

    LOG_CLOSE(lgg);
}

void test_main(void) {
    atomic_loggers_test();
    logger_test();
}