#include "atomic.h"

//////////////////////////////////////////////////////////////////
// Atomic loggers functions
inline void common_lgg_print(FILE *ostream, lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr) {
    char msg_buf[MAX_LOG_LINE_LEN];
    static const char *warn = "... !!! WARNING !!! Message was truncated!";
    int required_len;
	char *timeline;
	char *loglvl;

    // Assemble user formated string
    if (argptr != NULL) {
    	va_list args_copy;
    	va_copy(args_copy, argptr); // Copy just in case if pointed-to structure will be changed
    	required_len = vsnprintf(msg_buf, MAX_LOG_LINE_LEN, fmt, args_copy);
    	va_end(args_copy);
    }
    else
    	required_len = snprintf(msg_buf, MAX_LOG_LINE_LEN, fmt);

    timeline = get_datetime_str(time);
    loglvl = log_level_to_str(level);
    // And then print it in logger wrapped format
    // TODO: Make file, line and func optional
    fprintf(ostream, "%s [%-5s] {%s:%d} {%s()} %s%s\n", timeline, loglvl, file, line, func, msg_buf, required_len >= MAX_LOG_LINE_LEN ? warn : "");
}

FILE *file_lgg_output = NULL; // Output file handle

void console_lgg_print(lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr) {
    common_lgg_print(stdout, time, level, line, file, func, fmt, argptr);
}

int file_lgg_init(const char *log_path, const char *log_name, int max_files) {
    uint64_t log_num;
    uint64_t count = 0;
    uint64_t n_fst = 0;
    char buf[P_MAX_PATH];
    char log_dir[P_MAX_PATH];

    // Check if path exists
    if (!dir_exists(log_path)) {
        return 1;
    }

    // Prepare path string for use. First copy string to buffer, next append slash to the end
    *log_dir = '\0';
    strncat(log_dir, log_path, P_MAX_PATH);
    if (log_dir[strlen(log_path) - 1] != P_PATH_SLASH) {
        strcat(log_dir, P_PATH_SLASH_STR);
    }

    // Search all log files in directory and count how many they
    // TODO: Collect filenames in same structure regardless current OS.
    // 		 That's also will optimize memory usage, execution time and make code clearer.
#ifdef OS_WINDOWS

    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA *namelist = NULL;
    WIN32_FIND_DATA ffd;

    // Prepare path to use with FindFile functions.
    // Append * (joker) to the directory name
    strncpy(buf, log_dir, P_MAX_PATH);
    strncat(buf, "*", P_MAX_PATH);

    // Start extract info about directory
    hFind = FindFirstFile(buf, &ffd);
    if (INVALID_HANDLE_VALUE == hFind) {
        return 1;
    }

    do {
        // If we found file and it starts from log_name, have extension .log, then we count it
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (starts_with(log_name, ffd.cFileName) && ends_with(ffd.cFileName, ".log")) {
                buf_push(namelist, ffd);
                count++;
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);

    // Sort files info by names in alphabetical order (this is required for FAT filesystem)
    qsort(namelist, buf_len(namelist), sizeof(WIN32_FIND_DATA), ffdcmp);


#endif
#ifdef OS_LINUX

    struct dirent **dirent;
    char **namelist = NULL;
    int file_num;

    file_num = scandir(log_dir, &dirent, 0, alphasort);
    if (!file_num) {
        return 1;
    }
    else {
        // Search for files which names starts from log_name and have .log extension and count them
        while (file_num--) {
            if (starts_with(log_name, dirent[file_num]->d_name) && ends_with(dirent[file_num]->d_name, ".log")) {
            	buf_push(namelist, dirent[file_num]->d_name);
            	count++;
            }
        }

        // We'll need to sort in this implementation too, because alphasort isn't natural sorting algorithm and we have numbers in names
        qsort(namelist, buf_len(namelist), sizeof(char *), lognamecmp);
    }


#endif

    // Select from what name start look for free one
    if (count == 0)
        sprintf(buf, "%s%s.0.log", log_dir, log_name);
    else {
        // Extract last logfile (namelist[n_fst + count - 1]) number
#ifdef OS_WINDOWS
        log_num = extract_log_num(namelist[n_fst + count - 1].cFileName);
#endif
#ifdef OS_LINUX
        log_num = extract_log_num(namelist[n_fst + count - 1]);
#endif
        // Remove all files that have wrong names formats
        while (log_num < 0) {
            strncpy(buf, log_dir, P_MAX_PATH);
#ifdef OS_WINDOWS
            strncat(buf, namelist[n_fst + count - 1].cFileName, P_MAX_PATH);
#endif
#ifdef OS_LINUX
            strncat(buf, namelist[n_fst + count - 1], P_MAX_PATH);
#endif
            remove(buf);
            --count;
            if (count == 0) {
                // If there's no logfiles left stop deleting files
                break;
            }
#ifdef OS_WINDOWS
            log_num = extract_log_num(namelist[n_fst + count - 1].cFileName);
#endif
#ifdef OS_LINUX
            log_num = extract_log_num(namelist[n_fst + count - 1]);
#endif
        }

        // Delete extra files
        if (max_files > 0) {
            // We delete files until count = max_files-1, take into account that we have one more file to create
            // and for that new state condition count <= max_files also must be true
            while (count >= max_files) {
                strncpy(buf, log_dir, P_MAX_PATH);
#ifdef OS_WINDOWS
                strncat(buf, namelist[n_fst++].cFileName, P_MAX_PATH);
#endif
#ifdef OS_LINUX
                strncat(buf, namelist[n_fst++], P_MAX_PATH);
#endif
                remove(buf);
                --count;
            }
        }

        if (count > 0) {
            // When we found correct last logfile number, assign next to it to the new file 
            sprintf(buf, "%s%s.%lu.log", log_dir, log_name, log_num + 1);
        } else {
            // If there's no logfiles left, just start with 0 logfile number
            sprintf(buf, "%s%s.0.log", log_dir, log_name);
        }
    }


    file_lgg_output = fopen(buf, "w");
    if (file_lgg_output == NULL) {
        return 1;
    }
    else {
        return 0;
    }
}

void file_lgg_print(lgg_time *time, log_lvl level, uint16_t line, const char *file, const char *func, const char *fmt, va_list argptr) {
    common_lgg_print(file_lgg_output, time, level, line, file, func, fmt, argptr);
}

int file_lgg_close() {
    return fclose(file_lgg_output);
}
