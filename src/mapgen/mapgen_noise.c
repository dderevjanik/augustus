#include "mapgen_noise.h"

#include <math.h>

#define F2 0.3660254037844386f  // 0.5 * (sqrt(3) - 1)
#define G2 0.21132486540518713f // (3 - sqrt(3)) / 6

static const int grad2[][2] = {
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}
};

void mapgen_noise_init(mapgen_noise *noise, mapgen_rng *rng)
{
    uint8_t p[256];
    for (int i = 0; i < 256; i++) {
        p[i] = (uint8_t)i;
    }
    // Fisher-Yates shuffle using the seeded RNG
    for (int i = 255; i > 0; i--) {
        int j = mapgen_rng_between(rng, 0, i + 1);
        uint8_t tmp = p[i];
        p[i] = p[j];
        p[j] = tmp;
    }
    for (int i = 0; i < 256; i++) {
        noise->perm[i] = p[i];
        noise->perm[i + 256] = p[i];
    }
}

static float dot2(const int g[2], float x, float y)
{
    return g[0] * x + g[1] * y;
}

static int fast_floor(float x)
{
    int xi = (int)x;
    return x < xi ? xi - 1 : xi;
}

float mapgen_noise_2d(const mapgen_noise *noise, float x, float y)
{
    float s = (x + y) * F2;
    int i = fast_floor(x + s);
    int j = fast_floor(y + s);

    float t = (i + j) * G2;
    float x0 = x - (i - t);
    float y0 = y - (j - t);

    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    int ii = i & 255;
    int jj = j & 255;

    float n0 = 0, n1 = 0, n2 = 0;

    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 >= 0) {
        int gi0 = noise->perm[ii + noise->perm[jj]] % 8;
        t0 *= t0;
        n0 = t0 * t0 * dot2(grad2[gi0], x0, y0);
    }

    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 >= 0) {
        int gi1 = noise->perm[ii + i1 + noise->perm[jj + j1]] % 8;
        t1 *= t1;
        n1 = t1 * t1 * dot2(grad2[gi1], x1, y1);
    }

    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 >= 0) {
        int gi2 = noise->perm[ii + 1 + noise->perm[jj + 1]] % 8;
        t2 *= t2;
        n2 = t2 * t2 * dot2(grad2[gi2], x2, y2);
    }

    // Scale to [-1, 1]
    return 70.0f * (n0 + n1 + n2);
}

float mapgen_noise_octave_2d(const mapgen_noise *noise, float x, float y, int octaves, float persistence)
{
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float max_value = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += mapgen_noise_2d(noise, x * frequency, y * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / max_value;
}
