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
    size_t *random_read_indices = NULL;
    size_t *random_positions = NULL;
    int num_to_replace = config->num_replacements;
    
    /* Allocate arrays for multiple replacements */
    random_read_indices = safe_malloc(sizeof(size_t) * num_to_replace);
    random_positions = safe_malloc(sizeof(size_t) * num_to_replace);
    
    /* For random modes: quickly count reads and select multiple random reads */
    if (config->mode == MODE_RANDOM || config->mode == MODE_RANDOM_FIXED) {
        char command[2048];
        FILE *wc_fp;
        
        if (strstr(config->input_file, ".gz") != NULL) {
            snprintf(command, sizeof(command), "gzip -dc '%s' | wc -l", config->input_file);
        } else {
            snprintf(command, sizeof(command), "wc -l < '%s'", config->input_file);
        }
        
        wc_fp = popen(command, "r");
        if (wc_fp != NULL) {
            size_t total_lines = 0;
            if (fscanf(wc_fp, "%zu", &total_lines) == 1) {
                size_t total_reads = total_lines / 4;  /* FASTQ has 4 lines per read */
                if (total_reads > 0) {
                    /* Select multiple random reads (one for each replacement sequence) */
                    for (int i = 0; i < num_to_replace; i++) {
                        random_read_indices[i] = (rand() % total_reads) + 1;
                        random_positions[i] = 0;  /* Will be set later if needed */
                    }
                    if (config->verbose) {
                        printf("Random mode: selected %d reads out of %zu total reads: ", 
                               num_to_replace, total_reads);
                        for (int i = 0; i < num_to_replace; i++) {
                            printf("#%zu%s", random_read_indices[i], 
                                   i < num_to_replace - 1 ? ", " : "\n");
                        }
                    }
                }
            }
            pclose(wc_fp);
        }
        
        if (random_read_indices[0] == 0) {
            fprintf(stderr, "Error: Could not count reads in input file\n");
            free(random_read_indices);
            free(random_positions);
            return ERR_INVALID_FORMAT;
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
    
    while ((read_result = fastq_reader_next(reader, &record)) > 0) {
        record_count++;
        int should_replace = 0;
        size_t replace_pos = 0;
        char *replacement_seq = NULL;
        int replacement_idx = -1;
        
        if (config->mode == MODE_SINGLE) {
            /* Single mode: only replace the target read */
            if (record_count == config->target_read_index) {
                should_replace = 1;
                replace_pos = config->position;
                replacement_seq = config->replacement_seqs[0];
            }
        } else if (config->mode == MODE_RANDOM || config->mode == MODE_RANDOM_FIXED) {
            /* Random modes: check if this read is one of the selected ones */
            for (int i = 0; i < num_to_replace; i++) {
                if (record_count == random_read_indices[i]) {
                    should_replace = 1;
                    replacement_idx = i;
                    replacement_seq = config->replacement_seqs[i];
                    
                    if (config->mode == MODE_RANDOM) {
                        /* Generate random position if not already set */
                        if (random_positions[i] == 0) {
                            random_positions[i] = find_random_position(
                                strlen(record.sequence), strlen(replacement_seq));
                        }
                        replace_pos = random_positions[i];
                    } else {
                        /* Fixed position */
                        replace_pos = config->position;
                    }
                    break;
                }
            }
        } else {
            /* Position mode: replace all reads at specified position */
            should_replace = 1;
            replace_pos = config->position;
            /* Use sequences in rotation */
            replacement_seq = config->replacement_seqs[replacement_count % num_to_replace];
        }
        
        if (!should_replace || replacement_seq == NULL) {
            /* Write record unchanged */
            fprintf(out_fp, "@%s\n%s\n%s\n%s\n",
                    record.seq_id, record.sequence, record.plus_line, record.quality);
            fastq_record_free(&record);
            continue;
        }
        
        size_t repl_len = strlen(replacement_seq);
        
        if (strlen(record.sequence) >= repl_len + replace_pos) {
            /* Save original sequence segment */
            char *original_segment = safe_malloc(repl_len + 1);
            memcpy(original_segment, record.sequence + replace_pos, repl_len);
            original_segment[repl_len] = '\0';
            
            /* Perform replacement */
            size_t actual_pos;
            char *new_sequence = replace_at_position(record.sequence, 
                                                     replacement_seq, 
                                                     replace_pos, &actual_pos);
            
            if (new_sequence != NULL) {
                /* Log replacement */
                if (log_fp != NULL) {
                    ReplacementRecord rep_record;
                    rep_record.seq_id = record.seq_id;
                    rep_record.position = actual_pos;
                    rep_record.original_seq = original_segment;
                    rep_record.new_seq = replacement_seq;
                    log_replacement(log_fp, &rep_record);
                }
                
                if (config->verbose) {
                    printf("Replaced in %s at position %zu: %s -> %s",
                           record.seq_id, actual_pos, original_segment, replacement_seq);
                    if (replacement_idx >= 0) {
                        printf(" (seq #%d)\n", replacement_idx + 1);
                    } else {
                        printf("\n");
                    }
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
    free(random_read_indices);
    free(random_positions);
    
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
    size_t random_seq_index = 0;
    
    /* Select random replacement sequence if multiple provided */
    char *selected_replacement = config->replacement_seqs[0];
    if (config->num_replacements > 1) {
        int selected_idx = rand() % config->num_replacements;
        selected_replacement = config->replacement_seqs[selected_idx];
        if (config->verbose) {
            printf("Selected replacement sequence #%d: %s\n", selected_idx + 1, selected_replacement);
        }
    }
    
    /* For random modes: quickly count sequences using grep */
    if (config->mode == MODE_RANDOM || config->mode == MODE_RANDOM_FIXED) {
        char command[2048];
        FILE *grep_fp;
        
        if (strstr(config->input_file, ".gz") != NULL) {
            snprintf(command, sizeof(command), "gzip -dc '%s' | grep -c '^>'", config->input_file);
        } else {
            snprintf(command, sizeof(command), "grep -c '^>' '%s'", config->input_file);
        }
        
        grep_fp = popen(command, "r");
        if (grep_fp != NULL) {
            size_t total_seqs = 0;
            if (fscanf(grep_fp, "%zu", &total_seqs) == 1 && total_seqs > 0) {
                random_seq_index = (rand() % total_seqs) + 1;
                if (config->verbose) {
                    printf("Random mode: selected sequence #%zu out of %zu total sequences\n", 
                           random_seq_index, total_seqs);
                }
            }
            pclose(grep_fp);
        }
        
        if (random_seq_index == 0) {
            fprintf(stderr, "Error: Could not count sequences in input file\n");
            return ERR_INVALID_FORMAT;
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
    size_t repl_len = strlen(selected_replacement);
    size_t random_position = 0;
    
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
                    /* Random mode: only replace the pre-selected sequence at random position */
                    if (record_count == random_seq_index) {
                        should_replace = 1;
                        if (random_position == 0) {
                            random_position = find_random_position(seq_length, repl_len);
                        }
                        replace_pos = random_position;
                    }
                } else if (config->mode == MODE_RANDOM_FIXED) {
                    /* Random-fixed mode: only replace the pre-selected sequence at fixed position */
                    if (record_count == random_seq_index) {
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
                    memcpy(current_seq + replace_pos, selected_replacement, repl_len);
                    
                    /* Log replacement */
                    if (log_fp != NULL) {
                        ReplacementRecord rep_record;
                        rep_record.seq_id = current_id;
                        rep_record.position = replace_pos;
                        rep_record.original_seq = original_segment;
                        rep_record.new_seq = selected_replacement;
                        log_replacement(log_fp, &rep_record);
                    }
                    
                    if (config->verbose) {
                        printf("Replaced in %s at position %zu: %s -> %s\n",
                               current_id, replace_pos, original_segment, 
                               selected_replacement);
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
            /* Random mode: only replace the pre-selected sequence at random position */
            if (record_count == random_seq_index) {
                should_replace = 1;
                if (random_position == 0) {
                    random_position = find_random_position(seq_length, repl_len);
                }
                replace_pos = random_position;
            }
        } else if (config->mode == MODE_RANDOM_FIXED) {
            /* Random-fixed mode: only replace the pre-selected sequence at fixed position */
            if (record_count == random_seq_index) {
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
            
            memcpy(current_seq + replace_pos, selected_replacement, repl_len);
            
            if (log_fp != NULL) {
                ReplacementRecord rep_record;
                rep_record.seq_id = current_id;
                rep_record.position = replace_pos;
                rep_record.original_seq = original_segment;
                rep_record.new_seq = selected_replacement;
                log_replacement(log_fp, &rep_record);
            }
            
            if (config->verbose) {
                printf("Replaced in %s at position %zu: %s -> %s\n",
                       current_id, replace_pos, original_segment, 
                       selected_replacement);
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
