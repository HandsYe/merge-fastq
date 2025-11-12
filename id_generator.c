#include "id_generator.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

#define DEFAULT_INSTRUMENT "INSTRUMENT"
#define DEFAULT_RUN_ID "1"
#define DEFAULT_FLOWCELL "FLOWCELL"
#define DEFAULT_LANE 1
#define DEFAULT_TILE 1001
#define DEFAULT_X_POS 1000
#define DEFAULT_Y_POS 1000
#define DEFAULT_READ_NUM 1
#define DEFAULT_IS_FILTERED 'N'
#define DEFAULT_CONTROL_BITS 0
#define DEFAULT_INDEX "ATCG"

IdGenerator* id_generator_init(const IdGeneratorConfig *config) {
    IdGenerator *gen = safe_malloc(sizeof(IdGenerator));
    
    /* Set default values or use provided config */
    if (config != NULL) {
        gen->config.instrument_name = (config->instrument_name != NULL) ? 
            safe_strdup(config->instrument_name) : safe_strdup(DEFAULT_INSTRUMENT);
        gen->config.run_id = (config->run_id != NULL) ? 
            safe_strdup(config->run_id) : safe_strdup(DEFAULT_RUN_ID);
        gen->config.flowcell_id = (config->flowcell_id != NULL) ? 
            safe_strdup(config->flowcell_id) : safe_strdup(DEFAULT_FLOWCELL);
        gen->config.lane = (config->lane > 0) ? config->lane : DEFAULT_LANE;
        gen->config.tile = (config->tile > 0) ? config->tile : DEFAULT_TILE;
        gen->config.x_pos = (config->x_pos > 0) ? config->x_pos : DEFAULT_X_POS;
        gen->config.y_pos = (config->y_pos > 0) ? config->y_pos : DEFAULT_Y_POS;
        gen->config.read_num = (config->read_num > 0) ? config->read_num : DEFAULT_READ_NUM;
        gen->config.is_filtered = (config->is_filtered == 'Y' || config->is_filtered == 'N') ? 
            config->is_filtered : DEFAULT_IS_FILTERED;
        gen->config.control_bits = config->control_bits;
        gen->config.index_seq = (config->index_seq != NULL) ? 
            safe_strdup(config->index_seq) : safe_strdup(DEFAULT_INDEX);
    } else {
        /* Use all defaults */
        gen->config.instrument_name = safe_strdup(DEFAULT_INSTRUMENT);
        gen->config.run_id = safe_strdup(DEFAULT_RUN_ID);
        gen->config.flowcell_id = safe_strdup(DEFAULT_FLOWCELL);
        gen->config.lane = DEFAULT_LANE;
        gen->config.tile = DEFAULT_TILE;
        gen->config.x_pos = DEFAULT_X_POS;
        gen->config.y_pos = DEFAULT_Y_POS;
        gen->config.read_num = DEFAULT_READ_NUM;
        gen->config.is_filtered = DEFAULT_IS_FILTERED;
        gen->config.control_bits = DEFAULT_CONTROL_BITS;
        gen->config.index_seq = safe_strdup(DEFAULT_INDEX);
    }
    
    /* Initialize sequence counter */
    gen->sequence_counter = 0;
    
    return gen;
}

char* id_generator_next(IdGenerator *gen) {
    if (gen == NULL) {
        return NULL;
    }
    
    /* Increment counter */
    gen->sequence_counter++;
    
    /* Calculate coordinates based on counter */
    int x = gen->config.x_pos + (gen->sequence_counter % 1000);
    int y = gen->config.y_pos + (gen->sequence_counter / 1000);
    int tile = gen->config.tile + (gen->sequence_counter / 1000000);
    
    /* Format: @INSTRUMENT:RUN:FLOWCELL:LANE:TILE:X:Y READ:FILTERED:CONTROL:INDEX */
    char *id = safe_malloc(512);
    snprintf(id, 512, "%s:%s:%s:%d:%d:%d:%d %d:%c:%d:%s",
             gen->config.instrument_name,
             gen->config.run_id,
             gen->config.flowcell_id,
             gen->config.lane,
             tile,
             x,
             y,
             gen->config.read_num,
             gen->config.is_filtered,
             gen->config.control_bits,
             gen->config.index_seq);
    
    return id;
}

void id_generator_free(IdGenerator *gen) {
    if (gen == NULL) {
        return;
    }
    
    if (gen->config.instrument_name != NULL) {
        free(gen->config.instrument_name);
    }
    if (gen->config.run_id != NULL) {
        free(gen->config.run_id);
    }
    if (gen->config.flowcell_id != NULL) {
        free(gen->config.flowcell_id);
    }
    if (gen->config.index_seq != NULL) {
        free(gen->config.index_seq);
    }
    
    free(gen);
}
