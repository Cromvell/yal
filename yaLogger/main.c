// Windows specific headers
#include <windows.h>
#include <tchar.h>

// Portable headers
#include <stdio.h>
#include <string.h>
#include <sys\timeb.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/*
Logger TODO:

  + Logger initialization system
  + Change log level ability
  + Logging function
  + Enable writing into different streams

  - Code refactoring
  - Increase datetime precision (add format_str abilities)
  - Separate src into different files
  - Enable port to linux (and Mac OS in future(but who cares?))
  - Create different process and/or thread for logger
  - Enable configure loggers parameters

*/

// Common stuff

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)

void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}

void *xcalloc(size_t num_elems, size_t elem_size) {
    void *ptr = calloc(num_elems, elem_size);
    if (!ptr) {
        perror("xcalloc failed");
        exit(1);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t num_bytes) {
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        perror("xrealloc failed");
        exit(1);
    }
    return ptr;
}

void *xmalloc(size_t num_bytes) {
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

int file_exists(TCHAR * file) {
  WIN32_FIND_DATA find_file_data;
  HANDLE handle = FindFirstFile(file, &find_file_data);
  int found = handle != INVALID_HANDLE_VALUE;
  if(found) {
    FindClose(handle);
  }
  return found;
}

// Stretchy buffer

typedef struct BufHdr {
    size_t len;
    size_t cap;
    char buf[];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)(b) - offsetof(BufHdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b)*sizeof(*b) : 0)

#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)
#define buf_fit(b, n) ((n) <= buf_cap(b) ? 0 : ((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...) (buf_fit((b), 1 + buf_len(b)), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_printf(b, ...) ((b) = buf__printf((b), __VA_ARGS__))
#define buf_clear(b) ((b) ? buf__hdr(b)->len = 0 : 0)

void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
    assert(buf_cap(buf) <= (SIZE_MAX - 1)/2);
    size_t new_cap = CLAMP_MIN(2*buf_cap(buf), MAX(new_len, 16));
    assert(new_len <= new_cap);
    assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf))/elem_size);
    size_t new_size = offsetof(BufHdr, buf) + new_cap*elem_size;
    BufHdr *new_hdr;
    if (buf) {
        new_hdr = xrealloc(buf__hdr(buf), new_size);
    } else {
        new_hdr = xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

// Dumb logger

typedef enum {
  ERROR_L,
  WARNING_L,
  INFO_L,
  DEBUG_L
} _log_level;

char *log_leveltostr(_log_level level) {
  switch(level) {
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

typedef int (*_log_init)();
typedef int (*_log_close)();
typedef void (*_log_func)(const char *, _log_level);

typedef enum {
  CONSOLE_LGG,
  FILE_LGG
} _atomic_lgg_type;

typedef struct {
  _atomic_lgg_type type;
  _log_init init;
  _log_func func;
  _log_close close;
} atomic_lgg;

// Message preprocessing

// Date and time functions
// TODO: Make more appropriate datetime format
char *get_datetime_str() {
  struct timeb now;
  char *timeline, *endstr;

  // Get time in predefined format
  _ftime(&now);
  timeline = ctime(&(now.time));

  // Remove '\n' on line's end
  endstr = timeline;
  while (*endstr != '\0') endstr++;
  *(endstr - 1) = '\0';

  return timeline;
}

// TODO: Use vprintf and this implementation take out for another project
char *format_str(const char *format, ...) {
  va_list ap;
  size_t str_len;
  const char *sp, *last_sp;
  char *temp;
  // TODO: find a better way to allocate memory for string
  char *result_str = (char *) xmalloc(strlen(format)+256);
  char *result_str_ptr = result_str;
  
  va_start(ap, format);
  for (sp = format, last_sp = format; *sp != '\0'; sp++) {
    if (*sp == '%') {
      // Copy everything before current %
      str_len = sp - last_sp;
      memcpy(result_str_ptr, last_sp, str_len);
      result_str_ptr += str_len;
      
      switch(*(sp + 1)) {
      case 's':
	temp = va_arg(ap, char *);
	str_len = strlen(temp);
	memcpy(result_str_ptr, temp, str_len);
	result_str_ptr += str_len;
	break;
      case 'd':
	str_len = sp - last_sp;
	_itoa(va_arg(ap, int), result_str_ptr, 10);
	while (*result_str_ptr != '\0')
	  result_str_ptr++;
	break;
      case '%':
	*result_str_ptr = '%';
	result_str_ptr++;
	break;
      default:
	    // TODO: Output warning about unspecified token
	    break;
      }

      last_sp = sp + 2;
      sp = last_sp - 1;
    }
  }

  // Copy everything after last %
  str_len = strlen(last_sp);
  memcpy(result_str_ptr, last_sp, str_len);
  result_str_ptr += str_len;
  *result_str_ptr = '\0';
  
  va_end(ap);
  return result_str;
}

void format_str_test() {
  printf(format_str("LOL %s - This is string\n", "some string"));
  printf(format_str("%s\n", "string consist of chars"));
  printf(format_str("%s comon!\n", "Oh,"));
  printf(format_str("Yes, I %s\n", "will."));
  printf(format_str("this is number: %d\n", 123456));
  printf(format_str("%d - number\n", 654321));
  printf(format_str("%d\n", 111111));
  printf(format_str("NUM: %d, STR: %s\n", 42, "42"));
  printf(format_str("%s%s\n", "one", "two"));
  printf(format_str("%d%d\n", 42, 24));
}


// Atomic loggers functions
void _console_lgg(const char *msg, _log_level level) {
  fputs(format_str("%s [%s]: %s\n", get_datetime_str(), log_leveltostr(level), msg), stdout);
}

// TEMP: Assign path on logger init
const char *log_path = "C:\\Users\\Cromvell\\source\\repos\\yaLogger\\bin\\"; // DIRTY
const char *log_name = "testlog";

static FILE *_file_lgg_output = NULL;
int _file_lgg_init() {
   int log_n = 0;
   char *buf = format_str("%s%s.log", log_path, log_name);

   while (file_exists(buf)) {
     free(buf);
     buf = format_str("%s%s.%d.log", log_path, log_name, log_n);
     log_n++;
   }
  
   _file_lgg_output = fopen(buf, "w");
   if (_file_lgg_output == NULL) {
     return 1;
   } else {
     return 0;
   }
}

void _file_lgg(const char *msg, _log_level level) {
  fputs(format_str("%s [%s]: %s\n", get_datetime_str(), log_leveltostr(level), msg), _file_lgg_output);
}

int _file_lgg_close() {
  return fclose(_file_lgg_output);
}

void atomic_loggers_test() {
  _console_lgg("Just a test message. Error! Praise youselves!", ERROR_L);
  _console_lgg("Second test message. Just warn you", WARNING_L);
  _console_lgg("One more test message. This is info", INFO_L);
  _console_lgg("Yes. Test message. The last one. This time debug", DEBUG_L);

  _file_lgg_init();
  _file_lgg("Just a test message. Error! Praise youselves!", ERROR_L);
  _file_lgg("Second test message. Just warn you", WARNING_L);
  _file_lgg("One more test message. This is info", INFO_L);
  _file_lgg("Yes. Test message. The last one. This time debug", DEBUG_L);
  _file_lgg_close();
}

// Logger itself
static atomic_lgg *lgg_buf = NULL;
static _log_level glob_log_level = DEBUG_L;

void _add_atomic_lgg(_atomic_lgg_type type, _log_init init, _log_func func, _log_close close) {
  assert(func != NULL);
  buf_push(lgg_buf, (atomic_lgg){ type, init, func, close });
}

// TODO: Init logger in another process and/or thread
bool _logger_initialized = false;
int _logger_init() {
  if (!_logger_initialized) {
    int i;
  
    _add_atomic_lgg(CONSOLE_LGG, NULL, _console_lgg, NULL);
    _add_atomic_lgg(FILE_LGG, _file_lgg_init, _file_lgg, _file_lgg_close);

    assert(lgg_buf != NULL);
    for (i = 0; i < buf_len(lgg_buf); i++) {
      if (lgg_buf[i].init != NULL && lgg_buf[i].init())
	return 1;
    }

    _logger_initialized = true;
  }

  return 0;
}

void _print_log(const char *msg, _log_level level) {
  int i;
  
  if (!_logger_initialized && _logger_init()) {
    fatal("Logger initialization error");
  }

  assert(lgg_buf != NULL);
  for (i = 0; i < buf_len(lgg_buf); i++) {
    if (lgg_buf[i].func != NULL && level <= glob_log_level)
      lgg_buf[i].func(msg, level);
  }
}

int _logger_close() {
  if (lgg_buf != NULL) {
    int i;
    int result = 0;
    
    for (i = 0; i < buf_len(lgg_buf); i++) {
      if (lgg_buf[i].close != NULL)
	lgg_buf[i].close();
    }
  }
}

int _set_log_level(_log_level level) {
  if (level >= ERROR_L && level <= DEBUG_L)
    glob_log_level = level;
  else
    fatal("Unknown log level identifier: %d", level);
}

// External interface
#define LOG_INIT() (_logger_init())
#define LOG(lvl, msg) (_print_log((msg), (lvl)))
#define LOG_CLOSE() (_logger_close())
#define SET_LOG_LVL(lvl) (_set_log_level(lvl))

void logger_test() {
	LOG(ERROR_L, "Msg");
	LOG(WARNING_L, "Msg");
	LOG(INFO_L, "Msg");
	LOG(DEBUG_L, "Msg");
	SET_LOG_LVL(WARNING_L);
	LOG(INFO_L, "Just an info message that will never be loged.");
}

int main(int argc, char **argv) {
  format_str_test();
  atomic_loggers_test();
  logger_test();
}
