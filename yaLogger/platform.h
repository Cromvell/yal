#ifndef PLATFORM_H
#define PLATFORM_H


#if defined(__WIN32__) || defined (_WIN32) || defined(__CYGWIN32__)

#define OS_WINDOWS

// Windows specific headers
#include <windows.h>
#include <tchar.h>
#include <direct.h>
#include <sys\timeb.h>

// Platform-independent macros
#define P_MAX_PATH MAX_PATH
#define P_PATH_SLASH '\\'
#define P_PATH_SLASH_STR "\\"
#define p_getcwd _getcwd
#define p_ftime _ftime

#elif defined(__linux__) || defined(__gnu_linux__)

#define OS_LINUX

// Linux specific headers
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <linux/limits.h>

// Platform-independent macros
#define P_MAX_PATH PATH_MAX
#define P_PATH_SLASH '/'
#define P_PATH_SLASH_STR "/"
#define p_getcwd getcwd
#define p_ftime ftime


#elif defined(__APPLE__) || defined(__MACH__)

// Someday... Maybe some...day...

#endif


#endif // PLATFORM_H
