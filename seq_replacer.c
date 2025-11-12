#define _POSIX_C_SOURCE 200809L
#include "seq_replacer.h"
#include "utils.h"
#include "fastq_parser.h"
#include <string.h>
#include <time.h>
#include <ctype.h>

int is_fasta_file(const char *filename) {
    size_t len = strlen(filename);
    if (len > 3) {
        const char *ext = filename + len - 3;
        if (strcmp(ext, ".fa") == 0 || strcmp(ext, ".gz") == 0) {
            return 1;
        }
    }
    if (len > 6) {
        const char *ext = filename + len - 6;
        if (strcmp(ext, ".fasta") == 0) {
            return 1;
        }
    }
    if (len > 9) {
        const char *ext = filename + len - 9;
        if (strcmp(ext, ".fasta.gz") == 0) {
            return 1;
        }
    }
    return 0;
}

int is_fastq_file(const char *filename) {
    size_t len = strlen(filename);
    if (len > 3) {
        const char *ext = filename + len - 3;
        if (strcmp(ext, ".fq") == 0) {
            return 1;
        }
    }
    if (len > 6) {
        const char *ext = filename + len - 6;
        if (strcmp(ext, ".fastq") == 0 || strcmp(ext, ".fq.gz") == 0) {
            return 1;
        }
    }
    if (len > 9) {
        const char *ext = filename + len - 9;
        if (strcmp(ext, ".fastq.gz") == 0) {
            return 1;
        }
    }
    return 0;
}

/* Replace sequence at specified position */
static char* replace_at_position(const char *original, const char *replacement, 
                                 size_t position, size_t *actual_pos) {
    size_t orig_len = strlen(original);
    size_t repl_len = strlen(replacement);
    
    if (position + repl_len > orig_len) {
        /* Replacement would exceed sequence length */
        *actual_pos = 0;
        return NULL;
    }
    
    char *result = safe_strdup(original);
    memcpy(result + position, replacement, repl_len);
    *actual_pos = position;
    
    return result;
}

/* Find random valid position for replacement */
static size_t find_random_position(size_t seq_len, size_t repl_len) {
    if (seq_len < repl_len) {
        return 0;
    }
    
    size_t max_pos = seq_len - repl_len;
    if (max_pos == 0) {
        return 0;
    }
    
    return rand() % (max_pos + 1);
}

/* Log replacement to file */
static void log_replacement(FILE *log_fp, const ReplacementRecord *record) {
    fprintf(log_fp, "Sequence ID: %s\n", record->seq_id);
    fprintf(log_fp, "Position: %zu\n", record->position);
    fprintf(log_fp, "Original: %s\n", record->original_seq);
    fprintf(log_fp, "Replaced: %s\n", record->new_seq);
    fprintf(log_fp, "---\n");
}

/* Process FASTQ file */
static int process_fastq(const ReplacerConfig *config) {
    size_t random_read_index = 0;
    
    /* For random modes: first pass to count reads and select random one */
    if (config->mode == MODE_RANDOM || config->mode == MODE_RANDOM_FIXED) {
        FastqReader *counter = fastq_reader_open(config->input_file);
        if (counter == NULL) {
            return ERR_FILE_OPEN;
        }
        
        FastqRecord record;
        size_t total_reads = 0;
        int read_result;
        
        while ((read_result = fastq_reader_next(counter, &record)) > 0) {
            total_reads++;
            fastq_record_free(&record);
        }
        fastq_reader_close(counter);
        
        if (total_reads == 0) {
            fprintf(stderr, "Error: No reads found in input file\n");
            return ERR_INVALID_FORMAT;
        }
        
        /* Select random read (1-based) */
        random_read_index = (rand() % total_reads) + 1;
        
        if (config->verbose) {
            printf("Random mode: selected read #%zu out of %zu total reads\n", 
                   random_read_index, total_reads);
        }
    }
    
    /* Open input file for processing */
    FastqReader *reader = fastq_reader_open(config->input_file);
    if (reader == NULL) {
        return ERR_FILE_OPEN;
    }
    
    /* Open output file */
    FILE *out_fp = NULL;
    int is_output_pipe = 0;
    
    if (strstr(config->output_file, ".gz") != NULL) {
        char command[2048];
        snprintf(command, sizeof(command), "gzip -c > '%s'", config->output_file);
        out_fp = popen(command, "w");
        is_output_pipe = 1;
    } else {
        out_fp = fopen(config->output_file, "w");
    }
    
    if (out_fp == NULL) {
        fastq_reader_close(reader);
        return ERR_FILE_OPEN;
    }
    
    /* Open log file */
    FILE *log_fp = fopen(config->log_file, "w");
    if (log_fp == NULL) {
        fprintf(stderr, "Warning: Cannot open log file '%s'\n", config->log_file);
    }
    
    /* Process records */
    FastqRecord record;
    int read_result;
    size_t record_count = 0;
    size_t replacement_count = 0;
    size_t repl_len = strlen(config->replacement_seq);
    
    while ((read_result = fastq_reader_next(reader, &record)) > 0) {
        record_count++;
        int should_replace = 0;
        size_t replace_pos = 0;
        
        if (config->mode == MODE_SINGLE) {
            /* Single mode: only replace the target read */
            if (record_count == config->target_read_index) {
                should_replace = 1;
                replace_pos = config->position;
            }
        } else if (config->mode == MODE_RANDOM) {
            /* Random mode: only replace the randomly selected read at random position */
            if (record_count == random_read_index) {
                should_replace = 1;
                replace_pos = find_random_position(strlen(record.sequence), repl_len);
            }
        } else if (config->mode == MODE_RANDOM_FIXED) {
            /* Random fixed mode: only replace the randomly selected read at fixed position */
            if (record_count == random_read_index) {
                should_replace = 1;
                replace_pos = config->position;
            }
        } else {
            /* Position mode: always replace at specified position */
            should_replace = 1;
            replace_pos = config->position;
        }
        
        if (should_replace && strlen(record.sequence) >= repl_len + replace_pos) {
            /* Save original sequence segment */
            char *original_segment = safe_malloc(repl_len + 1);
            memcpy(original_segment, record.sequence + replace_pos, repl_len);
            original_segment[repl_len] = '\0';
            
            /* Perform replacement */
            size_t actual_pos;
            char *new_sequence = replace_at_position(record.sequence, 
                                                     config->replacement_seq, 
                                                     replace_pos, &actual_pos);
            
            if (new_sequence != NULL) {
                /* Log replacement */
                if (log_fp != NULL) {
                    ReplacementRecord rep_record;
                    rep_record.seq_id = record.seq_id;
                    rep_record.position = actual_pos;
                    rep_record.original_seq = original_segment;
                    rep_record.new_seq = config->replacement_seq;
                    log_replacement(log_fp, &rep_record);
                }
                
                if (config->verbose) {
                    printf("Replaced in %s at position %zu: %s -> %s\n",
                           record.seq_id, actual_pos, original_segment, 
                           config->replacement_seq);
                }
                
                /* Update sequence */
                free(record.sequence);
                record.sequence = new_sequence;
                replacement_count++;
            }
            
            free(original_segment);
        }
        
        /* Write record */
        fprintf(out_fp, "@%s\n%s\n%s\n%s\n",
                record.seq_id, record.sequence, record.plus_line, record.quality);
        
        fastq_record_free(&record);
    }
    
    /* Cleanup */
    fastq_reader_close(reader);
    if (is_output_pipe) {
        pclose(out_fp);
    } else {
        fclose(out_fp);
    }
    if (log_fp != NULL) {
        fclose(log_fp);
    }
    
    printf("\nReplacement completed:\n");
    printf("  Total sequences: %zu\n", record_count);
    printf("  Replacements made: %zu\n", replacement_count);
    printf("  Output file: %s\n", config->output_file);
    printf("  Log file: %s\n", config->log_file);
    
    return SUCCESS;
}

/* Process FASTA file */
static int process_fasta(const ReplacerConfig *config) {
    size_t random_read_index = 0;
    
    /* For random modes: first pass to count sequences */
    if (config->mode == MODE_RANDOM || config->mode == MODE_RANDOM_FIXED) {
        FILE *counter_fp = NULL;
        int is_counter_pipe = 0;
        
        if (strstr(config->input_file, ".gz") != NULL) {
            char command[2048];
            snprintf(command, sizeof(command), "gzip -dc '%s'", config->input_file);
            counter_fp = popen(command, "r");
            is_counter_pipe = 1;
        } else {
            counter_fp = fopen(config->input_file, "r");
        }
        
        if (counter_fp == NULL) {
            return ERR_FILE_OPEN;
        }
        
        char *line = NULL;
        size_t line_size = 0;
        ssize_t read;
        size_t total_seqs = 0;
        
        while ((read = getline(&line, &line_size, counter_fp)) != -1) {
            if (line[0] == '>') {
                total_seqs++;
            }
        }
        
        if (line != NULL) free(line);
        
        if (is_counter_pipe) pclose(counter_fp);
        else fclose(counter_fp);
        
        if (total_seqs == 0) {
            fprintf(stderr, "Error: No sequences found in input file\n");
            return ERR_INVALID_FORMAT;
        }
        
        /* Select random sequence (1-based) */
        random_read_index = (rand() % total_seqs) + 1;
        
        if (config->verbose) {
            printf("Random mode: selected sequence #%zu out of %zu total sequences\n", 
                   random_read_index, total_seqs);
        }
    }
    
    /* Open input file */
    FILE *in_fp = NULL;
    int is_input_pipe = 0;
    
    if (strstr(config->input_file, ".gz") != NULL) {
        char command[2048];
        snprintf(command, sizeof(command), "gzip -dc '%s'", config->input_file);
        in_fp = popen(command, "r");
        is_input_pipe = 1;
    } else {
        in_fp = fopen(config->input_file, "r");
    }
    
    if (in_fp == NULL) {
        return ERR_FILE_OPEN;
    }
    
    /* Open output file */
    FILE *out_fp = NULL;
    int is_output_pipe = 0;
    
    if (strstr(config->output_file, ".gz") != NULL) {
        char command[2048];
        snprintf(command, sizeof(command), "gzip -c > '%s'", config->output_file);
        out_fp = popen(command, "w");
        is_output_pipe = 1;
    } else {
        out_fp = fopen(config->output_file, "w");
    }
    
    if (out_fp == NULL) {
        if (is_input_pipe) pclose(in_fp);
        else fclose(in_fp);
        return ERR_FILE_OPEN;
    }
    
    /* Open log file */
    FILE *log_fp = fopen(config->log_file, "w");
    if (log_fp == NULL) {
        fprintf(stderr, "Warning: Cannot open log file '%s'\n", config->log_file);
    }
    
    /* Process FASTA records */
    char *line = NULL;
    size_t line_size = 0;
    ssize_t read;
    char *current_id = NULL;
    char *current_seq = NULL;
    size_t seq_capacity = 1024;
    size_t seq_length = 0;
    size_t record_count = 0;
    size_t replacement_count = 0;
    size_t repl_len = strlen(config->replacement_seq);
    
    current_seq = safe_malloc(seq_capacity);
    current_seq[0] = '\0';
    
    while ((read = getline(&line, &line_size, in_fp)) != -1) {
        trim_newline(line);
        
        if (line[0] == '>') {
            /* Process previous sequence if exists */
            if (current_id != NULL && seq_length > 0) {
                record_count++;
                int should_replace = 0;
                size_t replace_pos = 0;
                
                if (config->mode == MODE_SINGLE) {
                    if (record_count == config->target_read_index) {
                        should_replace = 1;
                        replace_pos = config->position;
                    }
                } else if (config->mode == MODE_RANDOM) {
                    /* Random mode: only replace the randomly selected sequence at random position */
                    if (record_count == random_read_index) {
                        should_replace = 1;
                        replace_pos = find_random_position(seq_length, repl_len);
                    }
                } else if (config->mode == MODE_RANDOM_FIXED) {
                    /* Random fixed mode: only replace the randomly selected sequence at fixed position */
                    if (record_count == random_read_index) {
                        should_replace = 1;
                        replace_pos = config->position;
                    }
                } else {
                    should_replace = 1;
                    replace_pos = config->position;
                }
                
                if (should_replace && seq_length >= repl_len + replace_pos) {
                    /* Save original segment */
                    char *original_segment = safe_malloc(repl_len + 1);
                    memcpy(original_segment, current_seq + replace_pos, repl_len);
                    original_segment[repl_len] = '\0';
                    
                    /* Perform replacement */
                    memcpy(current_seq + replace_pos, config->replacement_seq, repl_len);
                    
                    /* Log replacement */
                    if (log_fp != NULL) {
                        ReplacementRecord rep_record;
                        rep_record.seq_id = current_id;
                        rep_record.position = replace_pos;
                        rep_record.original_seq = original_segment;
                        rep_record.new_seq = config->replacement_seq;
                        log_replacement(log_fp, &rep_record);
                    }
                    
                    if (config->verbose) {
                        printf("Replaced in %s at position %zu: %s -> %s\n",
                               current_id, replace_pos, original_segment, 
                               config->replacement_seq);
                    }
                    
                    free(original_segment);
                    replacement_count++;
                }
                
                /* Write sequence */
                fprintf(out_fp, ">%s\n%s\n", current_id, current_seq);
            }
            
            /* Start new sequence */
            if (current_id != NULL) free(current_id);
            current_id = safe_strdup(line + 1);  /* Skip '>' */
            seq_length = 0;
            current_seq[0] = '\0';
        } else {
            /* Append to current sequence */
            size_t line_len = strlen(line);
            if (seq_length + line_len >= seq_capacity) {
                seq_capacity = (seq_length + line_len + 1) * 2;
                current_seq = safe_realloc(current_seq, seq_capacity);
            }
            strcpy(current_seq + seq_length, line);
            seq_length += line_len;
        }
    }
    
    /* Process last sequence */
    if (current_id != NULL && seq_length > 0) {
        record_count++;
        int should_replace = 0;
        size_t replace_pos = 0;
        
        if (config->mode == MODE_SINGLE) {
            if (record_count == config->target_read_index) {
                should_replace = 1;
                replace_pos = config->position;
            }
        } else if (config->mode == MODE_RANDOM) {
            /* Random mode: only replace the randomly selected sequence at random position */
            if (record_count == random_read_index) {
                should_replace = 1;
                replace_pos = find_random_position(seq_length, repl_len);
            }
        } else if (config->mode == MODE_RANDOM_FIXED) {
            /* Random fixed mode: only replace the randomly selected sequence at fixed position */
            if (record_count == random_read_index) {
                should_replace = 1;
                replace_pos = config->position;
            }
        } else {
            should_replace = 1;
            replace_pos = config->position;
        }
        
        if (should_replace && seq_length >= repl_len + replace_pos) {
            char *original_segment = safe_malloc(repl_len + 1);
            memcpy(original_segment, current_seq + replace_pos, repl_len);
            original_segment[repl_len] = '\0';
            
            memcpy(current_seq + replace_pos, config->replacement_seq, repl_len);
            
            if (log_fp != NULL) {
                ReplacementRecord rep_record;
                rep_record.seq_id = current_id;
                rep_record.position = replace_pos;
                rep_record.original_seq = original_segment;
                rep_record.new_seq = config->replacement_seq;
                log_replacement(log_fp, &rep_record);
            }
            
            if (config->verbose) {
                printf("Replaced in %s at position %zu: %s -> %s\n",
                       current_id, replace_pos, original_segment, 
                       config->replacement_seq);
            }
            
            free(original_segment);
            replacement_count++;
        }
        
        fprintf(out_fp, ">%s\n%s\n", current_id, current_seq);
    }
    
    /* Cleanup */
    if (line != NULL) free(line);
    if (current_id != NULL) free(current_id);
    if (current_seq != NULL) free(current_seq);
    
    if (is_input_pipe) pclose(in_fp);
    else fclose(in_fp);
    
    if (is_output_pipe) pclose(out_fp);
    else fclose(out_fp);
    
    if (log_fp != NULL) fclose(log_fp);
    
    printf("\nReplacement completed:\n");
    printf("  Total sequences: %zu\n", record_count);
    printf("  Replacements made: %zu\n", replacement_count);
    printf("  Output file: %s\n", config->output_file);
    printf("  Log file: %s\n", config->log_file);
    
    return SUCCESS;
}

int replace_sequences(const ReplacerConfig *config) {
    if (config == NULL) {
        return ERR_INVALID_PARAM;
    }
    
    /* Initialize random seed */
    if (config->mode == MODE_RANDOM) {
        srand(config->seed);
    }
    
    /* Determine file type and process */
    if (is_fastq_file(config->input_file)) {
        return process_fastq(config);
    } else if (is_fasta_file(config->input_file)) {
        return process_fasta(config);
    } else {
        fprintf(stderr, "Error: Unknown file format. Use .fq, .fastq, .fa, or .fasta extensions\n");
        return ERR_INVALID_FORMAT;
    }
}
