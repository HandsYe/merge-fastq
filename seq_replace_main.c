#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "seq_replacer.h"
#include "utils.h"

#define VERSION "1.0.0"

void print_usage(const char *program_name) {
    printf("Sequence Replacer - Replace sequences in FASTA/FASTQ files\n\n");
    printf("Usage: %s -i input.fq -o output.fq -s SEQUENCE [options]\n\n", program_name);
    printf("Required arguments:\n");
    printf("  -i, --input <file>     Input FASTA/FASTQ file (.fa, .fq, .fasta, .fastq)\n");
    printf("                         Supports gzipped files (.gz extension)\n");
    printf("  -o, --output <file>    Output file path\n");
    printf("  -s, --sequence <seq>   Replacement sequence (can be specified multiple times)\n\n");
    printf("Replacement mode (choose one):\n");
    printf("  -r, --random           Random mode: replace one random read at random position\n");
    printf("  -R, --random-pos <pos> Random-fixed mode: one random read at position <pos> (FASTEST)\n");
    printf("  -p, --position <pos>   Position mode: replace at position <pos> in all sequences\n");
    printf("                         (0-based position)\n");
    printf("  -1, --single <n> <pos> Single mode: replace only read #n at position <pos>\n");
    printf("                         (read number is 1-based, position is 0-based)\n\n");
    printf("Optional arguments:\n");
    printf("  -l, --log <file>       Log file for replacement records (default: replacements.log)\n");
    printf("  --seed <n>             Random seed for reproducibility (default: current time)\n");
    printf("  -v, --verbose          Verbose output mode\n");
    printf("  -h, --help             Display help information\n");
    printf("  --version              Display version information\n\n");
    printf("Examples:\n");
    printf("  # Replace only the 3rd read at position 10\n");
    printf("  %s -i input.fq.gz -o output.fq.gz -s ATCGATCG -1 3 10 -v\n\n", program_name);
    printf("  # Random read at position 20 with multiple sequences (FASTEST)\n");
    printf("  %s -i input.fq.gz -o output.fq.gz -s ATCGATCG -s GCGCGCGC -R 20 -v\n\n", program_name);
    printf("  # Random replacement: one random read at random position\n");
    printf("  %s -i input.fq.gz -o output.fq.gz -s ATCGATCG -r -v\n\n", program_name);
    printf("  # Replace at position 50 in all sequences with multiple sequences\n");
    printf("  %s -i input.fa -o output.fa -s NNNNNNNN -s XXXXXXXX -p 50 -l changes.log\n\n", program_name);
    printf("  # Reproducible random replacement with seed\n");
    printf("  %s -i input.fq -o output.fq -s GCGCGCGC -r --seed 12345\n", program_name);
}

void print_version() {
    printf("seq_replacer version %s\n", VERSION);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return ERR_INVALID_PARAM;
    }
    
    /* Variables for command line arguments */
    char *input_file = NULL;
    char *output_file = NULL;
    char **replacement_seqs = safe_malloc(sizeof(char*) * 100);  /* Max 100 sequences */
    int num_replacement_seqs = 0;
    char *log_file = "replacements.log";
    ReplacementMode mode = MODE_RANDOM;
    size_t position = 0;
    size_t target_read_index = 1;
    int verbose = 0;
    unsigned int seed = (unsigned int)time(NULL);
    int mode_set = 0;
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return SUCCESS;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            return SUCCESS;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -i/--input requires a file argument\n");
                return ERR_INVALID_PARAM;
            }
            input_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o/--output requires a file argument\n");
                return ERR_INVALID_PARAM;
            }
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sequence") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -s/--sequence requires a sequence argument\n");
                free(replacement_seqs);
                return ERR_INVALID_PARAM;
            }
            if (num_replacement_seqs >= 100) {
                fprintf(stderr, "Error: Too many replacement sequences (max 100)\n");
                free(replacement_seqs);
                return ERR_INVALID_PARAM;
            }
            replacement_seqs[num_replacement_seqs++] = argv[++i];
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--random") == 0) {
            mode = MODE_RANDOM;
            mode_set = 1;
        } else if (strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "--random-pos") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -R/--random-pos requires a position argument\n");
                return ERR_INVALID_PARAM;
            }
            mode = MODE_RANDOM_FIXED;
            position = (size_t)atoi(argv[++i]);
            mode_set = 1;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--position") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -p/--position requires a position argument\n");
                return ERR_INVALID_PARAM;
            }
            mode = MODE_POSITION;
            position = (size_t)atoi(argv[++i]);
            mode_set = 1;
        } else if (strcmp(argv[i], "-1") == 0 || strcmp(argv[i], "--single") == 0) {
            if (i + 2 >= argc) {
                fprintf(stderr, "Error: -1/--single requires read number and position arguments\n");
                return ERR_INVALID_PARAM;
            }
            mode = MODE_SINGLE;
            target_read_index = (size_t)atoi(argv[++i]);
            position = (size_t)atoi(argv[++i]);
            mode_set = 1;
            if (target_read_index == 0) {
                fprintf(stderr, "Error: Read number must be >= 1\n");
                return ERR_INVALID_PARAM;
            }
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -l/--log requires a file argument\n");
                return ERR_INVALID_PARAM;
            }
            log_file = argv[++i];
        } else if (strcmp(argv[i], "--seed") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --seed requires a number argument\n");
                return ERR_INVALID_PARAM;
            }
            seed = (unsigned int)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return ERR_INVALID_PARAM;
        }
    }
    
    /* Validate required parameters */
    if (input_file == NULL) {
        fprintf(stderr, "Error: Input file must be specified\n");
        print_usage(argv[0]);
        return ERR_INVALID_PARAM;
    }
    
    if (output_file == NULL) {
        fprintf(stderr, "Error: Output file must be specified\n");
        print_usage(argv[0]);
        return ERR_INVALID_PARAM;
    }
    
    if (num_replacement_seqs == 0) {
        fprintf(stderr, "Error: At least one replacement sequence must be specified\n");
        print_usage(argv[0]);
        free(replacement_seqs);
        return ERR_INVALID_PARAM;
    }
    
    if (!mode_set) {
        fprintf(stderr, "Error: Must specify either -r/--random, -p/--position, or -1/--single\n");
        print_usage(argv[0]);
        return ERR_INVALID_PARAM;
    }
    
    /* Validate input file exists */
    if (!file_exists(input_file)) {
        fprintf(stderr, "Error: Input file does not exist: %s\n", input_file);
        return ERR_FILE_OPEN;
    }
    
    /* Create configuration */
    ReplacerConfig config;
    config.input_file = input_file;
    config.output_file = output_file;
    config.replacement_seqs = replacement_seqs;
    config.num_replacements = num_replacement_seqs;
    config.log_file = log_file;
    config.mode = mode;
    config.position = position;
    config.target_read_index = target_read_index;
    config.total_reads = 0;
    config.verbose = verbose;
    config.seed = seed;
    
    /* Print configuration */
    if (verbose) {
        printf("Configuration:\n");
        printf("  Input: %s\n", input_file);
        printf("  Output: %s\n", output_file);
        printf("  Replacement sequences (%d): ", num_replacement_seqs);
        for (int i = 0; i < num_replacement_seqs; i++) {
            printf("%s%s", replacement_seqs[i], i < num_replacement_seqs - 1 ? ", " : "\n");
        }
        printf("  Mode: %s\n", 
               mode == MODE_SINGLE ? "Single" : 
               (mode == MODE_RANDOM ? "Random" : 
               (mode == MODE_RANDOM_FIXED ? "Random-Fixed" : "Position")));
        if (mode == MODE_SINGLE) {
            printf("  Target read: #%zu\n", target_read_index);
            printf("  Position in read: %zu\n", position);
        } else if (mode == MODE_RANDOM) {
            printf("  Random seed: %u\n", seed);
        } else if (mode == MODE_RANDOM_FIXED) {
            printf("  Position: %zu\n", position);
            printf("  Random seed: %u\n", seed);
        } else {
            printf("  Position: %zu\n", position);
        }
        printf("  Log file: %s\n\n", log_file);
    }
    
    /* Execute replacement */
    int result = replace_sequences(&config);
    
    if (result != SUCCESS) {
        fprintf(stderr, "\nReplacement failed with error code: %d\n", result);
    }
    
    /* Cleanup */
    free(replacement_seqs);
    
    return result;
}
