#ifndef MAPGEN_RANDOM_H
#define MAPGEN_RANDOM_H

#include <stdint.h>

typedef struct {
    uint32_t state[4];
} mapgen_rng;

void mapgen_rng_seed(mapgen_rng *rng, uint32_t seed);
uint32_t mapgen_rng_next(mapgen_rng *rng);
int mapgen_rng_between(mapgen_rng *rng, int min, int max);
float mapgen_rng_float(mapgen_rng *rng);

#endif // MAPGEN_RANDOM_H
