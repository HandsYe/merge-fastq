#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Error codes */
#define SUCCESS 0
#define ERR_FILE_OPEN 1
#define ERR_FILE_READ 2
#define ERR_FILE_WRITE 3
#define ERR_INVALID_FORMAT 4
#define ERR_MEMORY_ALLOC 5
#define ERR_INVALID_PARAM 6

/* Memory management functions */
void* safe_malloc(size_t size);
void* safe_realloc(void *ptr, size_t size);

/* String processing functions */
char* safe_strdup(const char *str);
void trim_newline(char *str);

/* Error handling functions */
void error_exit(const char *format, ...);
void warning_msg(const char *format, ...);

/* File operation functions */
int file_exists(const char *filename);
long get_file_size(const char *filename);

#endif /* UTILS_H */
