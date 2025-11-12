#ifndef FASTQ_PARSER_H
#define FASTQ_PARSER_H

#include <stdio.h>
#include <stdlib.h>

/* FASTQ record structure */
typedef struct {
    char *seq_id;        /* Sequence ID (without @ symbol) */
    char *sequence;      /* Base sequence */
    char *plus_line;     /* Separator line (usually +) */
    char *quality;       /* Quality scores */
} FastqRecord;

/* FASTQ reader structure */
typedef struct {
    FILE *fp;
    char *filename;
    size_t line_number;
    int is_valid;
    int is_pipe;  /* Flag to indicate if fp is from popen (for gzip) */
} FastqReader;

/* Open FASTQ file for reading */
FastqReader* fastq_reader_open(const char *filename);

/* Read next FASTQ record */
int fastq_reader_next(FastqReader *reader, FastqRecord *record);

/* Validate FASTQ record format */
int fastq_record_validate(const FastqRecord *record, char *error_msg, size_t error_msg_size);

/* Free FASTQ record memory */
void fastq_record_free(FastqRecord *record);

/* Close FASTQ reader */
void fastq_reader_close(FastqReader *reader);

#endif /* FASTQ_PARSER_H */
