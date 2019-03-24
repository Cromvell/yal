#define CONSOLE_TEST(lvl, msg) (p_ftime(&t), console_lgg_print(&t, (lvl), (uint16_t)__LINE__, __FILENAME__, __FUNCTION__, (msg), NULL))
#define FILE_TEST(lvl, msg) (p_ftime(&t), file_lgg_print(&t, (lvl), (uint16_t)__LINE__, __FILENAME__, __FUNCTION__, (msg), NULL))

static struct timeb t;

// Logger parameters
#ifdef OS_WINDOWS
const char *log_path = "C:\\Users\\Cromvell\\source\\repos\\yaLogger\\bin\\";
#else
const char *log_path = "/home/cromvell/";
#endif

const char *log_name = "testlog";


void atomic_loggers_test() {
    CONSOLE_TEST(ERROR_L, "Just a test message. Error! Praise yourselves!");
    CONSOLE_TEST(WARNING_L, "Second test message. Just warn you");
    CONSOLE_TEST(INFO_L, "One more test message. This is info");
    CONSOLE_TEST(DEBUG_L, "Yes. Test message. The last one. This time debug");

    if (file_lgg_init(TEST__MODE ? log_path : p_getcwd(NULL, 0), "testlog", 0)) {
        fatal("Atomic file logger init error");
    }
    FILE_TEST(ERROR_L, "Just a test message. Error! Praise yourselves!");
    FILE_TEST(WARNING_L, "Second test message. Just warn you");
    FILE_TEST(INFO_L, "One more test message. This is info");
    FILE_TEST(DEBUG_L, "Yes. Test message. The last one, I promise. This time debug");
    FILE_TEST(DEBUG_L, "Very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long message.");
    file_lgg_close();
}

void logger_test() {
    logger *lgg = LOG_INIT(&(lgg_conf) { log_path, log_name, DEBUG_L, 4 });
    //logger *lgg = LOG_INIT(NULL);
    LOG(lgg, ERROR_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, WARNING_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, INFO_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    LOG(lgg, DEBUG_L, "Message: %s, %d, %f", "string", 42, 2.718281828);
    SET_LOG_LVL(lgg, WARNING_L);
    LOG(lgg, INFO_L, "Just an info message that will never be logged.");

    LOG_CLOSE(lgg);
}

#define TEST_FUNC(func, ...) (printf("%s(%s):\n\t= %d\n\n", #func, (#__VA_ARGS__), func(__VA_ARGS__)))

void test_extract_log_num() {
    TEST_FUNC(extract_log_num, "logname.0.log");
    TEST_FUNC(extract_log_num, "logname.1.log");
    TEST_FUNC(extract_log_num, "logname.42.log");
    TEST_FUNC(extract_log_num, "logname..log");
    TEST_FUNC(extract_log_num, "logname.1234567890.log");
    TEST_FUNC(extract_log_num, "logname.-235log");
    TEST_FUNC(extract_log_num, "logname.+11log");
    TEST_FUNC(extract_log_num, "logname.log");
    TEST_FUNC(extract_log_num, "logname.-1.log");
}

void test_main(void) {
    //test_extract_log_num();
    //atomic_loggers_test();
    logger_test();
}
