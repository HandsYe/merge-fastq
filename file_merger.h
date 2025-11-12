#ifndef FILE_MERGER_H
#define FILE_MERGER_H

#include <stdio.h>
#include "id_generator.h"
#include "fastq_parser.h"

/* Merger configuration structure */
typedef struct {
    char **input_files;      /* Input file path array */
    int num_input_files;     /* Number of input files */
    char *output_file;       /* Output file path */
    IdGenerator *id_gen;     /* ID generator */
    int verbose;             /* Verbose output flag */
} MergerConfig;

/* Merger statistics structure */
typedef struct {
    size_t total_sequences;  /* Total sequences processed */
    size_t total_files;      /* Total files processed */
    int success;             /* Success flag */
} MergerStats;

/* Execute file merge */
int merge_fastq_files(const MergerConfig *config, MergerStats *stats);

/* Write FASTQ record to output file */
int write_fastq_record(FILE *out_fp, const char *new_id, const FastqRecord *record);

#endif /* FILE_MERGER_H */
