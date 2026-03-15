#include "mapgen_random.h"

static uint32_t splitmix32(uint32_t *state)
{
    uint32_t z = (*state += 0x9e3779b9);
    z = (z ^ (z >> 16)) * 0x85ebca6b;
    z = (z ^ (z >> 13)) * 0xc2b2ae35;
    return z ^ (z >> 16);
}

static uint32_t rotl(uint32_t x, int k)
{
    return (x << k) | (x >> (32 - k));
}

void mapgen_rng_seed(mapgen_rng *rng, uint32_t seed)
{
    uint32_t sm = seed;
    rng->state[0] = splitmix32(&sm);
    rng->state[1] = splitmix32(&sm);
    rng->state[2] = splitmix32(&sm);
    rng->state[3] = splitmix32(&sm);
}

uint32_t mapgen_rng_next(mapgen_rng *rng)
{
    uint32_t *s = rng->state;
    uint32_t result = rotl(s[1] * 5, 7) * 9;
    uint32_t t = s[1] << 9;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl(s[3], 11);

    return result;
}

int mapgen_rng_between(mapgen_rng *rng, int min, int max)
{
    if (min >= max) {
        return min;
    }
    uint32_t range = (uint32_t)(max - min);
    return min + (int)(mapgen_rng_next(rng) % range);
}

float mapgen_rng_float(mapgen_rng *rng)
{
    return (mapgen_rng_next(rng) >> 8) / 16777216.0f;
}
