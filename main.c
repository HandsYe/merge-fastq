#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fastq_parser.h"
#include "id_generator.h"
#include "file_merger.h"
#include "utils.h"

#define VERSION "1.0.0"
#define MAX_INPUT_FILES 1000

void print_usage(const char *program_name) {
    printf("Usage: %s -i input1.fq -i input2.fq -o output.fq [options]\n\n", program_name);
    printf("Required arguments:\n");
    printf("  -i, --input <file>     Input FASTQ file (can be specified multiple times)\n");
    printf("                         Supports both plain (.fq) and gzipped (.fq.gz) files\n");
    printf("  -o, --output <file>    Output FASTQ file path\n");
    printf("                         Use .gz extension for compressed output\n\n");
    printf("Optional arguments:\n");
    printf("  -p, --prefix <string>  Sequence ID prefix (default: \"INSTRUMENT\")\n");
    printf("  -r, --run-id <string>  Run number (default: \"1\")\n");
    printf("  -f, --flowcell <string> Flowcell ID (default: \"FLOWCELL\")\n");
    printf("  -l, --lane <int>       Lane number (default: 1)\n");
    printf("  -v, --verbose          Verbose output mode\n");
    printf("  -h, --help             Display help information\n");
    printf("  --version              Display version information\n\n");
    printf("Examples:\n");
    printf("  %s -i file1.fq.gz -i file2.fq.gz -o merged.fq.gz -v\n", program_name);
    printf("  %s -i file1.fq -o output.fq -p MYINST -r 100 -l 2\n", program_name);
}

void print_version() {
    printf("fastq_merger version %s\n", VERSION);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return ERR_INVALID_PARAM;
    }
    
    /* Variables for command line arguments */
    char **input_files = safe_malloc(sizeof(char*) * MAX_INPUT_FILES);
    int num_input_files = 0;
    char *output_file = NULL;
    char *instrument_name = NULL;
    char *run_id = NULL;
    char *flowcell_id = NULL;
    int lane = 0;
    int verbose = 0;
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            free(input_files);
            return SUCCESS;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            free(input_files);
            return SUCCESS;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -i/--input requires a file argument\n");
                free(input_files);
                return ERR_INVALID_PARAM;
            }
            if (num_input_files >= MAX_INPUT_FILES) {
                fprintf(stderr, "Error: Too many input files (max %d)\n", MAX_INPUT_FILES);
                free(input_files);
                return ERR_INVALID_PARAM;
            }
            input_files[num_input_files++] = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o/--output requires a file argument\n");
                free(input_files);
                return ERR_INVALID_PARAM;
            }
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--prefix") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -p/--prefix requires a string argument\n");
                free(input_files);
                return ERR_INVALID_PARAM;
            }
            instrument_name = argv[++i];
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--run-id") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -r/--run-id requires a string argument\n");
                free(input_files);
                return ERR_INVALID_PARAM;
            }
            run_id = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--flowcell") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -f/--flowcell requires a string argument\n");
                free(input_files);
                return ERR_INVALID_PARAM;
            }
            flowcell_id = argv[++i];
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lane") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -l/--lane requires an integer argument\n");
                free(input_files);
                return ERR_INVALID_PARAM;
            }
            lane = atoi(argv[++i]);
            if (lane <= 0) {
                fprintf(stderr, "Error: Lane number must be a positive integer\n");
                free(input_files);
                return ERR_INVALID_PARAM;
            }
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            free(input_files);
            return ERR_INVALID_PARAM;
        }
    }
    
    /* Validate required parameters */
    if (num_input_files == 0) {
        fprintf(stderr, "Error: At least one input file must be specified\n");
        print_usage(argv[0]);
        free(input_files);
        return ERR_INVALID_PARAM;
    }
    
    if (output_file == NULL) {
        fprintf(stderr, "Error: Output file must be specified\n");
        print_usage(argv[0]);
        free(input_files);
        return ERR_INVALID_PARAM;
    }
    
    /* Validate input files exist */
    for (int i = 0; i < num_input_files; i++) {
        if (!file_exists(input_files[i])) {
            fprintf(stderr, "Error: Input file does not exist: %s\n", input_files[i]);
            free(input_files);
            return ERR_FILE_OPEN;
        }
    }
    
    /* Check that input and output files are different */
    for (int i = 0; i < num_input_files; i++) {
        if (strcmp(input_files[i], output_file) == 0) {
            fprintf(stderr, "Error: Input and output files must be different\n");
            free(input_files);
            return ERR_INVALID_PARAM;
        }
    }
    
    /* Initialize ID generator configuration */
    IdGeneratorConfig id_config = {0};
    id_config.instrument_name = instrument_name;
    id_config.run_id = run_id;
    id_config.flowcell_id = flowcell_id;
    id_config.lane = lane;
    id_config.tile = 0;
    id_config.x_pos = 0;
    id_config.y_pos = 0;
    id_config.read_num = 1;
    id_config.is_filtered = 'N';
    id_config.control_bits = 0;
    id_config.index_seq = NULL;
    
    IdGenerator *id_gen = id_generator_init(&id_config);
    if (id_gen == NULL) {
        fprintf(stderr, "Error: Failed to initialize ID generator\n");
        free(input_files);
        return ERR_MEMORY_ALLOC;
    }
    
    /* Create merger configuration */
    MergerConfig merger_config;
    merger_config.input_files = input_files;
    merger_config.num_input_files = num_input_files;
    merger_config.output_file = output_file;
    merger_config.id_gen = id_gen;
    merger_config.verbose = verbose;
    
    /* Execute merge */
    MergerStats stats;
    int result = merge_fastq_files(&merger_config, &stats);
    
    /* Print summary */
    if (result == SUCCESS) {
        printf("\nMerge completed successfully:\n");
        printf("  Files processed: %zu\n", stats.total_files);
        printf("  Total sequences: %zu\n", stats.total_sequences);
        printf("  Output file: %s\n", output_file);
    } else {
        fprintf(stderr, "\nMerge failed with error code: %d\n", result);
    }
    
    /* Clean up */
    id_generator_free(id_gen);
    free(input_files);
    
    return result;
}
