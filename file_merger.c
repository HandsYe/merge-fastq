#define _POSIX_C_SOURCE 200809L
#include "file_merger.h"
#include "utils.h"
#include <string.h>
#include <errno.h>

#define WRITE_BUFFER_SIZE 8192

/* Helper function to check if output file should be gzipped */
static int is_gzipped_output(const char *filename) {
    size_t len = strlen(filename);
    return (len > 3 && strcmp(filename + len - 3, ".gz") == 0);
}

int write_fastq_record(FILE *out_fp, const char *new_id, const FastqRecord *record) {
    if (out_fp == NULL || new_id == NULL || record == NULL) {
        return ERR_INVALID_PARAM;
    }
    
    /* Write sequence ID line with @ prefix */
    if (fprintf(out_fp, "@%s\n", new_id) < 0) {
        fprintf(stderr, "Error: Failed to write sequence ID: %s\n", strerror(errno));
        return ERR_FILE_WRITE;
    }
    
    /* Write sequence line */
    if (fprintf(out_fp, "%s\n", record->sequence) < 0) {
        fprintf(stderr, "Error: Failed to write sequence: %s\n", strerror(errno));
        return ERR_FILE_WRITE;
    }
    
    /* Write separator line */
    if (fprintf(out_fp, "%s\n", record->plus_line) < 0) {
        fprintf(stderr, "Error: Failed to write separator: %s\n", strerror(errno));
        return ERR_FILE_WRITE;
    }
    
    /* Write quality line */
    if (fprintf(out_fp, "%s\n", record->quality) < 0) {
        fprintf(stderr, "Error: Failed to write quality: %s\n", strerror(errno));
        return ERR_FILE_WRITE;
    }
    
    return SUCCESS;
}

int merge_fastq_files(const MergerConfig *config, MergerStats *stats) {
    if (config == NULL || stats == NULL) {
        return ERR_INVALID_PARAM;
    }
    
    /* Initialize stats */
    stats->total_sequences = 0;
    stats->total_files = 0;
    stats->success = 0;
    
    /* Open output file */
    FILE *out_fp = NULL;
    int is_output_pipe = 0;
    
    if (is_gzipped_output(config->output_file)) {
        /* Use gzip to compress output */
        char command[2048];
        snprintf(command, sizeof(command), "gzip -c > '%s'", config->output_file);
        out_fp = popen(command, "w");
        is_output_pipe = 1;
        
        if (out_fp == NULL) {
            fprintf(stderr, "Error: Cannot open gzipped output file '%s': %s\n", 
                    config->output_file, strerror(errno));
            return ERR_FILE_OPEN;
        }
    } else {
        /* Regular file */
        out_fp = fopen(config->output_file, "w");
        is_output_pipe = 0;
        
        if (out_fp == NULL) {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", 
                    config->output_file, strerror(errno));
            return ERR_FILE_OPEN;
        }
    }
    
    /* Set write buffer for better performance */
    char *write_buffer = safe_malloc(WRITE_BUFFER_SIZE);
    setvbuf(out_fp, write_buffer, _IOFBF, WRITE_BUFFER_SIZE);
    
    /* Process each input file */
    for (int i = 0; i < config->num_input_files; i++) {
        const char *input_file = config->input_files[i];
        
        if (config->verbose) {
            printf("Processing file %d/%d: %s\n", 
                   i + 1, config->num_input_files, input_file);
        }
        
        /* Open input file */
        FastqReader *reader = fastq_reader_open(input_file);
        if (reader == NULL) {
            fprintf(stderr, "Error: Failed to open input file '%s'\n", input_file);
            if (is_output_pipe) {
                pclose(out_fp);
            } else {
                fclose(out_fp);
            }
            free(write_buffer);
            return ERR_FILE_OPEN;
        }
        
        /* Process each record in the file */
        FastqRecord record;
        int read_result;
        size_t file_sequences = 0;
        
        while ((read_result = fastq_reader_next(reader, &record)) > 0) {
            /* Validate record */
            char error_msg[256];
            if (!fastq_record_validate(&record, error_msg, sizeof(error_msg))) {
                fprintf(stderr, "Error: Invalid FASTQ record in '%s' at line %zu: %s\n",
                        input_file, reader->line_number, error_msg);
                fastq_record_free(&record);
                fastq_reader_close(reader);
                if (is_output_pipe) {
                    pclose(out_fp);
                } else {
                    fclose(out_fp);
                }
                free(write_buffer);
                return ERR_INVALID_FORMAT;
            }
            
            /* Generate new ID */
            char *new_id = id_generator_next(config->id_gen);
            if (new_id == NULL) {
                fprintf(stderr, "Error: Failed to generate sequence ID\n");
                fastq_record_free(&record);
                fastq_reader_close(reader);
                if (is_output_pipe) {
                    pclose(out_fp);
                } else {
                    fclose(out_fp);
                }
                free(write_buffer);
                return ERR_MEMORY_ALLOC;
            }
            
            /* Write record with new ID */
            int write_result = write_fastq_record(out_fp, new_id, &record);
            free(new_id);
            
            if (write_result != SUCCESS) {
                fastq_record_free(&record);
                fastq_reader_close(reader);
                if (is_output_pipe) {
                    pclose(out_fp);
                } else {
                    fclose(out_fp);
                }
                free(write_buffer);
                return write_result;
            }
            
            /* Clean up record */
            fastq_record_free(&record);
            
            file_sequences++;
            stats->total_sequences++;
            
            /* Print progress in verbose mode */
            if (config->verbose && file_sequences % 10000 == 0) {
                printf("  Processed %zu sequences...\n", file_sequences);
            }
        }
        
        /* Check for read errors */
        if (read_result < 0) {
            fprintf(stderr, "Error: Failed to read from '%s'\n", input_file);
            fastq_reader_close(reader);
            if (is_output_pipe) {
                pclose(out_fp);
            } else {
                fclose(out_fp);
            }
            free(write_buffer);
            return ERR_FILE_READ;
        }
        
        if (config->verbose) {
            printf("  Completed: %zu sequences from '%s'\n", file_sequences, input_file);
        }
        
        fastq_reader_close(reader);
        stats->total_files++;
    }
    
    /* Close output file */
    if (is_output_pipe) {
        pclose(out_fp);
    } else {
        fclose(out_fp);
    }
    free(write_buffer);
    
    /* Print summary */
    if (config->verbose) {
        printf("\nMerge completed successfully:\n");
        printf("  Files processed: %zu\n", stats->total_files);
        printf("  Total sequences: %zu\n", stats->total_sequences);
        printf("  Output file: %s\n", config->output_file);
    }
    
    stats->success = 1;
    return SUCCESS;
}
