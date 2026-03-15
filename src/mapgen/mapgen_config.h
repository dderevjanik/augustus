#ifndef MAPGEN_CONFIG_H
#define MAPGEN_CONFIG_H

#include <stdint.h>

typedef struct {
    uint32_t seed;
    int map_size;            // 0-5 index: 40, 60, 80, 100, 120, 160

    // Terrain density (0-100%)
    int tree_density;        // default 40
    int rock_density;        // default 20
    int shrub_density;       // default 15
    int meadow_density;      // default 25

    // Elevation
    int elevation_enabled;   // 0 or 1
    int max_elevation;       // 1-5, default 3
    int flatness;            // 0-100, how much flat buildable land (default 50)

    // Water
    int river_count;         // 0-2, default 0
    int lake_count;          // 0-4, default 1
    int lake_size;           // 1-5 (small to large), default 2
    int coastal;             // 0=none, 1=one side, 2=corner (L-shaped coast)

    // Game features (auto-placed)
    int place_entry_exit;    // default 1
    int place_fishing;       // default 1
    int place_herds;         // default 1
} mapgen_config;

void mapgen_config_init_defaults(mapgen_config *config);

#endif // MAPGEN_CONFIG_H
