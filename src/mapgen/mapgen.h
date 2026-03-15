#ifndef MAPGEN_H
#define MAPGEN_H

#include "mapgen_config.h"

/**
 * Generate a random map based on the given configuration.
 * Must be called after game_file_editor_create_scenario() has created a blank map.
 * @return 1 on success, 0 on failure
 */
int mapgen_generate(const mapgen_config *config);

#endif // MAPGEN_H
