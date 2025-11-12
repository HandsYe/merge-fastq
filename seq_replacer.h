#ifndef SEQ_REPLACER_H
#define SEQ_REPLACER_H

#include <stdio.h>
#include <stdlib.h>

/* Replacement mode */
typedef enum {
    MODE_RANDOM,         /* Random replacement: one random read at random position */
    MODE_RANDOM_FIXED,   /* Random read at fixed position */
    MODE_POSITION,       /* Position-specific replacement in all sequences */
    MODE_SINGLE          /* Replace only one specific sequence in the entire file */
} ReplacementMode;

/* Replacement configuration */
typedef struct {
    char *input_file;
    char *output_file;
    char *replacement_seq;
    char *log_file;
    ReplacementMode mode;
    size_t position;      /* For position/single mode: 0-based position in sequence */
    size_t target_read_index; /* For single mode: which read to replace (1-based) */
    size_t total_reads;       /* For random mode: total number of reads (set during processing) */
    int verbose;
    unsigned int seed;    /* Random seed */
} ReplacerConfig;

/* Replacement record for logging */
typedef struct {
    char *seq_id;
    size_t position;
    char *original_seq;
    char *new_seq;
} ReplacementRecord;

/* Main replacement function */
int replace_sequences(const ReplacerConfig *config);

/* Helper functions */
int is_fasta_file(const char *filename);
int is_fastq_file(const char *filename);

#endif /* SEQ_REPLACER_H */
