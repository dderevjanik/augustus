#ifndef MAPGEN_NOISE_H
#define MAPGEN_NOISE_H

#include "mapgen_random.h"

typedef struct {
    uint8_t perm[512];
} mapgen_noise;

void mapgen_noise_init(mapgen_noise *noise, mapgen_rng *rng);
float mapgen_noise_2d(const mapgen_noise *noise, float x, float y);
float mapgen_noise_octave_2d(const mapgen_noise *noise, float x, float y, int octaves, float persistence);

#endif // MAPGEN_NOISE_H
