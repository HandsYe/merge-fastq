#define _POSIX_C_SOURCE 200809L
#include "utils.h"
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

void* safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        error_exit("Memory allocation failed: %s", strerror(errno));
    }
    return ptr;
}

void* safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL && size > 0) {
        error_exit("Memory reallocation failed: %s", strerror(errno));
    }
    return new_ptr;
}

char* safe_strdup(const char *str) {
    if (str == NULL) {
        return NULL;
    }
    char *dup = strdup(str);
    if (dup == NULL) {
        error_exit("String duplication failed: %s", strerror(errno));
    }
    return dup;
}

void trim_newline(char *str) {
    if (str == NULL) {
        return;
    }
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

void error_exit(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(ERR_MEMORY_ALLOC);
}

void warning_msg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

long get_file_size(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) != 0) {
        return -1;
    }
    return buffer.st_size;
}
