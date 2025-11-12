#define _POSIX_C_SOURCE 200809L
#include "fastq_parser.h"
#include "utils.h"
#include <string.h>
#include <errno.h>

#define INITIAL_BUFFER_SIZE 256

/* Helper function to read a line with dynamic buffer */
static char* read_line_dynamic(FILE *fp, size_t *line_len) {
    size_t buffer_size = INITIAL_BUFFER_SIZE;
    char *buffer = safe_malloc(buffer_size);
    size_t pos = 0;
    int c;
    
    while ((c = fgetc(fp)) != EOF) {
        if (pos >= buffer_size - 1) {
            buffer_size *= 2;
            buffer = safe_realloc(buffer, buffer_size);
        }
        
        buffer[pos++] = (char)c;
        
        if (c == '\n') {
            break;
        }
    }
    
    if (pos == 0 && c == EOF) {
        free(buffer);
        return NULL;
    }
    
    buffer[pos] = '\0';
    trim_newline(buffer);
    
    if (line_len != NULL) {
        *line_len = pos;
    }
    
    return buffer;
}

/* Helper function to check if file is gzipped */
static int is_gzipped(const char *filename) {
    size_t len = strlen(filename);
    return (len > 3 && strcmp(filename + len - 3, ".gz") == 0);
}

FastqReader* fastq_reader_open(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }
    
    FILE *fp = NULL;
    int is_pipe = 0;
    
    /* Check if file is gzipped */
    if (is_gzipped(filename)) {
        /* Use gzip to decompress on the fly */
        char command[2048];
        snprintf(command, sizeof(command), "gzip -dc '%s'", filename);
        fp = popen(command, "r");
        is_pipe = 1;
        
        if (fp == NULL) {
            fprintf(stderr, "Error: Cannot open gzipped file '%s': %s\n", 
                    filename, strerror(errno));
            return NULL;
        }
    } else {
        /* Regular file */
        fp = fopen(filename, "r");
        is_pipe = 0;
        
        if (fp == NULL) {
            fprintf(stderr, "Error: Cannot open file '%s': %s\n", 
                    filename, strerror(errno));
            return NULL;
        }
    }
    
    FastqReader *reader = safe_malloc(sizeof(FastqReader));
    reader->fp = fp;
    reader->filename = safe_strdup(filename);
    reader->line_number = 0;
    reader->is_valid = 1;
    reader->is_pipe = is_pipe;
    
    return reader;
}

int fastq_reader_next(FastqReader *reader, FastqRecord *record) {
    if (reader == NULL || record == NULL || !reader->is_valid) {
        return 0;
    }
    
    /* Initialize record fields */
    record->seq_id = NULL;
    record->sequence = NULL;
    record->plus_line = NULL;
    record->quality = NULL;
    
    /* Read line 1: sequence ID (starts with @) */
    record->seq_id = read_line_dynamic(reader->fp, NULL);
    if (record->seq_id == NULL) {
        return 0; /* End of file */
    }
    reader->line_number++;
    
    /* Remove @ prefix if present */
    if (record->seq_id[0] == '@') {
        char *temp = safe_strdup(record->seq_id + 1);
        free(record->seq_id);
        record->seq_id = temp;
    }
    
    /* Read line 2: sequence */
    record->sequence = read_line_dynamic(reader->fp, NULL);
    if (record->sequence == NULL) {
        fastq_record_free(record);
        fprintf(stderr, "Error: Incomplete FASTQ record at line %zu in '%s'\n", 
                reader->line_number, reader->filename);
        reader->is_valid = 0;
        return -1;
    }
    reader->line_number++;
    
    /* Read line 3: plus line (separator) */
    record->plus_line = read_line_dynamic(reader->fp, NULL);
    if (record->plus_line == NULL) {
        fastq_record_free(record);
        fprintf(stderr, "Error: Incomplete FASTQ record at line %zu in '%s'\n", 
                reader->line_number, reader->filename);
        reader->is_valid = 0;
        return -1;
    }
    reader->line_number++;
    
    /* Read line 4: quality scores */
    record->quality = read_line_dynamic(reader->fp, NULL);
    if (record->quality == NULL) {
        fastq_record_free(record);
        fprintf(stderr, "Error: Incomplete FASTQ record at line %zu in '%s'\n", 
                reader->line_number, reader->filename);
        reader->is_valid = 0;
        return -1;
    }
    reader->line_number++;
    
    return 1; /* Successfully read a record */
}

int fastq_record_validate(const FastqRecord *record, char *error_msg, size_t error_msg_size) {
    if (record == NULL) {
        if (error_msg != NULL && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, "Record is NULL");
        }
        return 0;
    }
    
    /* Check that all fields are present */
    if (record->seq_id == NULL || record->sequence == NULL || 
        record->plus_line == NULL || record->quality == NULL) {
        if (error_msg != NULL && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, "Record has NULL fields");
        }
        return 0;
    }
    
    /* Check that plus line starts with + */
    if (record->plus_line[0] != '+') {
        if (error_msg != NULL && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, "Separator line must start with '+'");
        }
        return 0;
    }
    
    /* Check that sequence and quality have the same length */
    size_t seq_len = strlen(record->sequence);
    size_t qual_len = strlen(record->quality);
    
    if (seq_len != qual_len) {
        if (error_msg != NULL && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, 
                    "Sequence length (%zu) does not match quality length (%zu)", 
                    seq_len, qual_len);
        }
        return 0;
    }
    
    return 1; /* Valid record */
}

void fastq_record_free(FastqRecord *record) {
    if (record == NULL) {
        return;
    }
    
    if (record->seq_id != NULL) {
        free(record->seq_id);
        record->seq_id = NULL;
    }
    if (record->sequence != NULL) {
        free(record->sequence);
        record->sequence = NULL;
    }
    if (record->plus_line != NULL) {
        free(record->plus_line);
        record->plus_line = NULL;
    }
    if (record->quality != NULL) {
        free(record->quality);
        record->quality = NULL;
    }
}

void fastq_reader_close(FastqReader *reader) {
    if (reader == NULL) {
        return;
    }
    
    if (reader->fp != NULL) {
        if (reader->is_pipe) {
            pclose(reader->fp);  /* Close pipe for gzipped files */
        } else {
            fclose(reader->fp);  /* Close regular file */
        }
        reader->fp = NULL;
    }
    if (reader->filename != NULL) {
        free(reader->filename);
        reader->filename = NULL;
    }
    
    free(reader);
}
