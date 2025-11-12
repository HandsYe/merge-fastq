#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

#include <stdlib.h>

/* ID generator configuration structure */
typedef struct {
    char *instrument_name;  /* Instrument name */
    char *run_id;           /* Run number */
    char *flowcell_id;      /* Flowcell ID */
    int lane;               /* Lane number */
    int tile;               /* Tile number */
    int x_pos;              /* X coordinate */
    int y_pos;              /* Y coordinate */
    int read_num;           /* Read number (1 or 2) */
    char is_filtered;       /* Filtered flag (Y/N) */
    int control_bits;       /* Control bits */
    char *index_seq;        /* Index sequence */
} IdGeneratorConfig;

/* ID generator structure */
typedef struct {
    IdGeneratorConfig config;
    size_t sequence_counter;  /* Sequence counter */
} IdGenerator;

/* Initialize ID generator */
IdGenerator* id_generator_init(const IdGeneratorConfig *config);

/* Generate next sequence ID */
char* id_generator_next(IdGenerator *gen);

/* Free ID generator */
void id_generator_free(IdGenerator *gen);

#endif /* ID_GENERATOR_H */
