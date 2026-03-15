#include "mapgen.h"

#include "mapgen_noise.h"
#include "mapgen_random.h"

#include "core/image.h"
#include "core/image_group.h"
#include "editor/tool_restriction.h"
#include "map/building_tiles.h"
#include "map/elevation.h"
#include "map/grid.h"
#include "map/image.h"
#include "map/image_context.h"
#include "map/property.h"
#include "map/random.h"
#include "map/routing_terrain.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "scenario/editor_map.h"

#include <math.h>

#define NOISE_SCALE_TERRAIN 0.05f
#define NOISE_SCALE_ELEVATION 0.03f
#define NOISE_SCALE_WATER 0.04f
#define NOISE_OFFSET_ROCK 100.0f
#define NOISE_OFFSET_TREE 200.0f
#define NOISE_OFFSET_SHRUB 300.0f
#define NOISE_OFFSET_MEADOW 400.0f
#define NOISE_OFFSET_WATER 500.0f
#define NOISE_OFFSET_ELEVATION 600.0f
#define LAKE_FLOOD_MAX 10000

static float density_to_threshold(int density)
{
    // density 0 -> threshold 1.0 (nothing passes)
    // density 100 -> threshold -1.0 (everything passes)
    return 1.0f - (density / 50.0f);
}

static int is_map_edge(int x, int y, int width, int height)
{
    return x <= 1 || y <= 1 || x >= width - 2 || y >= height - 2;
}

static void generate_terrain(const mapgen_config *config, const mapgen_noise *noise)
{
    int width = map_grid_width();
    int height = map_grid_height();

    float rock_threshold = density_to_threshold(config->rock_density);
    float tree_threshold = density_to_threshold(config->tree_density);
    float shrub_threshold = density_to_threshold(config->shrub_density);
    float meadow_threshold = density_to_threshold(config->meadow_density);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (!map_grid_is_valid_offset(grid_offset)) {
                continue;
            }
            if (is_map_edge(x, y, width, height)) {
                continue;
            }
            // Skip tiles already occupied by water or elevation
            if (map_terrain_is(grid_offset, TERRAIN_WATER | TERRAIN_ELEVATION)) {
                continue;
            }

            float fx = (float)x;
            float fy = (float)y;

            float rock_val = mapgen_noise_octave_2d(noise,
                fx * NOISE_SCALE_TERRAIN + NOISE_OFFSET_ROCK,
                fy * NOISE_SCALE_TERRAIN + NOISE_OFFSET_ROCK, 4, 0.5f);
            if (rock_val > rock_threshold) {
                map_terrain_add(grid_offset, TERRAIN_ROCK);
                continue;
            }

            float tree_val = mapgen_noise_octave_2d(noise,
                fx * NOISE_SCALE_TERRAIN + NOISE_OFFSET_TREE,
                fy * NOISE_SCALE_TERRAIN + NOISE_OFFSET_TREE, 4, 0.5f);
            if (tree_val > tree_threshold) {
                map_terrain_add(grid_offset, TERRAIN_TREE);
                continue;
            }

            float shrub_val = mapgen_noise_octave_2d(noise,
                fx * NOISE_SCALE_TERRAIN + NOISE_OFFSET_SHRUB,
                fy * NOISE_SCALE_TERRAIN + NOISE_OFFSET_SHRUB, 3, 0.5f);
            if (shrub_val > shrub_threshold) {
                map_terrain_add(grid_offset, TERRAIN_SHRUB);
                continue;
            }

            float meadow_val = mapgen_noise_octave_2d(noise,
                fx * NOISE_SCALE_TERRAIN + NOISE_OFFSET_MEADOW,
                fy * NOISE_SCALE_TERRAIN + NOISE_OFFSET_MEADOW, 3, 0.6f);
            if (meadow_val > meadow_threshold) {
                map_terrain_add(grid_offset, TERRAIN_MEADOW);
            }
        }
    }
}

static void generate_lakes(const mapgen_config *config, const mapgen_noise *noise, mapgen_rng *rng)
{
    if (config->lake_count <= 0) {
        return;
    }

    int width = map_grid_width();
    int height = map_grid_height();
    int margin = width / 6;

    for (int lake = 0; lake < config->lake_count; lake++) {
        int cx = mapgen_rng_between(rng, margin, width - margin);
        int cy = mapgen_rng_between(rng, margin, height - margin);

        int target_size = (config->lake_size * config->lake_size * width * height) / 400;
        if (target_size < 16) {
            target_size = 16;
        }

        // Simple flood-fill from center using noise as height
        // Use a queue (array-based BFS)
        static int queue_x[LAKE_FLOOD_MAX];
        static int queue_y[LAKE_FLOOD_MAX];
        int queue_head = 0;
        int queue_tail = 0;
        int filled = 0;

        float center_noise = mapgen_noise_octave_2d(noise,
            cx * NOISE_SCALE_WATER + NOISE_OFFSET_WATER,
            cy * NOISE_SCALE_WATER + NOISE_OFFSET_WATER, 3, 0.5f);
        float water_level = center_noise + 0.3f;

        int center_offset = map_grid_offset(cx, cy);
        if (!map_grid_is_valid_offset(center_offset)) {
            continue;
        }

        // Clear any existing terrain at center and set water
        map_terrain_set(center_offset, TERRAIN_WATER);
        queue_x[queue_tail] = cx;
        queue_y[queue_tail] = cy;
        queue_tail++;
        filled++;

        static const int dx[] = {0, 1, 0, -1};
        static const int dy[] = {-1, 0, 1, 0};

        while (queue_head < queue_tail && filled < target_size) {
            int qx = queue_x[queue_head];
            int qy = queue_y[queue_head];
            queue_head++;

            for (int d = 0; d < 4; d++) {
                int nx = qx + dx[d];
                int ny = qy + dy[d];

                if (is_map_edge(nx, ny, width, height)) {
                    continue;
                }

                int offset = map_grid_offset(nx, ny);
                if (!map_grid_is_valid_offset(offset)) {
                    continue;
                }
                if (map_terrain_is(offset, TERRAIN_WATER)) {
                    continue;
                }

                float n = mapgen_noise_octave_2d(noise,
                    nx * NOISE_SCALE_WATER + NOISE_OFFSET_WATER,
                    ny * NOISE_SCALE_WATER + NOISE_OFFSET_WATER, 3, 0.5f);

                if (n < water_level && queue_tail < LAKE_FLOOD_MAX) {
                    map_terrain_set(offset, TERRAIN_WATER);
                    queue_x[queue_tail] = nx;
                    queue_y[queue_tail] = ny;
                    queue_tail++;
                    filled++;
                }
            }
        }
    }
}

static void generate_rivers(const mapgen_config *config, const mapgen_noise *noise, mapgen_rng *rng)
{
    if (config->river_count <= 0) {
        return;
    }

    int width = map_grid_width();
    int height = map_grid_height();

    for (int river = 0; river < config->river_count && river < 2; river++) {
        int start_x, start_y, end_x, end_y;
        int side = mapgen_rng_between(rng, 0, 4);

        // Pick start on one edge, end on opposite edge (exactly on map boundary)
        switch (side) {
            case 0: // top to bottom
                start_x = mapgen_rng_between(rng, width / 4, 3 * width / 4);
                start_y = 0;
                end_x = mapgen_rng_between(rng, width / 4, 3 * width / 4);
                end_y = height - 1;
                break;
            case 1: // bottom to top
                start_x = mapgen_rng_between(rng, width / 4, 3 * width / 4);
                start_y = height - 1;
                end_x = mapgen_rng_between(rng, width / 4, 3 * width / 4);
                end_y = 0;
                break;
            case 2: // left to right
                start_x = 0;
                start_y = mapgen_rng_between(rng, height / 4, 3 * height / 4);
                end_x = width - 1;
                end_y = mapgen_rng_between(rng, height / 4, 3 * height / 4);
                break;
            default: // right to left
                start_x = width - 1;
                start_y = mapgen_rng_between(rng, height / 4, 3 * height / 4);
                end_x = 0;
                end_y = mapgen_rng_between(rng, height / 4, 3 * height / 4);
                break;
        }

        // Set river entry/exit points in scenario
        if (river == 0) {
            scenario_editor_set_river_entry_point(start_x, start_y);
            scenario_editor_set_river_exit_point(end_x, end_y);
        }

        // Walk from start to end with noise perturbation
        int steps = width + height;
        // Half-width: 1 gives 3 tiles wide, 2 gives 5 tiles wide
        float river_width = 1.0f + mapgen_rng_float(rng) * 1.5f;

        for (int step = 0; step <= steps; step++) {
            float t = (float)step / (float)steps;
            float base_x = start_x + (end_x - start_x) * t;
            float base_y = start_y + (end_y - start_y) * t;

            // Add noise perturbation perpendicular to river direction
            // Fade perturbation to zero near start/end so river meets the edge cleanly
            float edge_fade = 1.0f;
            if (t < 0.15f) {
                edge_fade = t / 0.15f;
            } else if (t > 0.85f) {
                edge_fade = (1.0f - t) / 0.15f;
            }
            float perturb = mapgen_noise_octave_2d(noise,
                base_x * 0.08f + river * 50.0f,
                base_y * 0.08f + river * 50.0f, 2, 0.5f) * (width / 8.0f) * edge_fade;

            // Perpendicular direction
            float dx = (float)(end_x - start_x);
            float dy = (float)(end_y - start_y);
            float len = sqrtf(dx * dx + dy * dy);
            if (len < 1.0f) len = 1.0f;
            float perp_x = -dy / len;
            float perp_y = dx / len;

            float rx = base_x + perturb * perp_x;
            float ry = base_y + perturb * perp_y;

            // Fill river width
            int rxi = (int)rx;
            int ryi = (int)ry;
            int w = (int)river_width;

            for (int wy = -w; wy <= w; wy++) {
                for (int wx = -w; wx <= w; wx++) {
                    int px = rxi + wx;
                    int py = ryi + wy;
                    if (px < 0 || px >= width || py < 0 || py >= height) {
                        continue;
                    }
                    int offset = map_grid_offset(px, py);
                    if (map_grid_is_valid_offset(offset)) {
                        map_terrain_set(offset, TERRAIN_WATER);
                    }
                }
            }
        }
    }
}

static void generate_coastal(const mapgen_config *config, const mapgen_noise *noise)
{
    if (config->coastal <= 0) {
        return;
    }

    int width = map_grid_width();
    int height = map_grid_height();
    float shore_depth = width * 0.2f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (!map_grid_is_valid_offset(grid_offset)) {
                continue;
            }

            float dist_to_edge;
            if (config->coastal == 1) {
                // One side (left edge) - distance from x=0
                dist_to_edge = (float)x;
            } else {
                // Corner (top-left) - distance from nearest of x=0 or y=0
                dist_to_edge = (float)(x < y ? x : y);
            }

            float noise_val = mapgen_noise_octave_2d(noise,
                x * 0.06f + 700.0f, y * 0.06f + 700.0f, 3, 0.5f);
            float adjusted_depth = shore_depth + noise_val * shore_depth * 0.5f;

            if (dist_to_edge < adjusted_depth) {
                map_terrain_set(grid_offset, TERRAIN_WATER);
            }
        }
    }
}

static void generate_elevation(const mapgen_config *config, const mapgen_noise *noise)
{
    if (!config->elevation_enabled) {
        return;
    }

    int width = map_grid_width();
    int height = map_grid_height();
    float flat_threshold = config->flatness / 100.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (!map_grid_is_valid_offset(grid_offset)) {
                continue;
            }

            // Don't place elevation on water
            if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
                continue;
            }

            if (is_map_edge(x, y, width, height)) {
                continue;
            }

            float noise_val = mapgen_noise_octave_2d(noise,
                x * NOISE_SCALE_ELEVATION + NOISE_OFFSET_ELEVATION,
                y * NOISE_SCALE_ELEVATION + NOISE_OFFSET_ELEVATION, 4, 0.5f);

            // Normalize from [-1,1] to [0,1]
            float normalized = (noise_val + 1.0f) * 0.5f;

            // Apply flatness: only elevate tiles above the flatness threshold
            if (normalized <= flat_threshold) {
                continue;
            }

            // Remap remaining range to elevation levels
            float elevation_ratio = (normalized - flat_threshold) / (1.0f - flat_threshold);
            int elevation = (int)(elevation_ratio * config->max_elevation + 0.5f);
            if (elevation < 1) elevation = 1;
            if (elevation > config->max_elevation) elevation = config->max_elevation;

            map_elevation_set(grid_offset, elevation);
            map_terrain_add(grid_offset, TERRAIN_ELEVATION);

            // Clear trees/shrubs on elevated terrain (rocks can stay)
            map_terrain_remove(grid_offset, TERRAIN_TREE | TERRAIN_SHRUB | TERRAIN_MEADOW);
        }
    }

    // Smooth cliffs to enforce max 1 level difference
    map_elevation_remove_cliffs();
}

static int find_edge_position(int edge_coord, int is_x_edge, int scan_start, int scan_max, mapgen_rng *rng)
{
    // Find a passable tile on the given map edge
    // is_x_edge: if 1, edge_coord is x and we scan y; if 0, edge_coord is y and we scan x
    int start = mapgen_rng_between(rng, scan_max / 4, 3 * scan_max / 4);
    for (int attempts = 0; attempts < scan_max; attempts++) {
        int pos = (start + attempts) % scan_max;
        if (pos < 1 || pos >= scan_max - 1) continue;

        int x = is_x_edge ? edge_coord : pos;
        int y = is_x_edge ? pos : edge_coord;
        int offset = map_grid_offset(x, y);
        if (map_grid_is_valid_offset(offset) &&
            !map_terrain_is(offset, TERRAIN_WATER | TERRAIN_ROCK | TERRAIN_ELEVATION | TERRAIN_BUILDING)) {
            return pos;
        }
    }
    return scan_max / 2; // fallback to center
}

static void clear_terrain_at(int x, int y, int radius)
{
    // Clear obstacles in a small area to ensure the point is usable
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int offset = map_grid_offset(x + dx, y + dy);
            if (map_grid_is_valid_offset(offset)) {
                map_terrain_remove(offset, TERRAIN_TREE | TERRAIN_ROCK | TERRAIN_SHRUB |
                    TERRAIN_MEADOW | TERRAIN_ELEVATION);
                map_elevation_set(offset, 0);
            }
        }
    }
}

static void place_road_tile(int x, int y)
{
    int offset = map_grid_offset(x, y);
    if (!map_grid_is_valid_offset(offset)) {
        return;
    }
    // Clear vegetation obstacles but preserve elevation
    if (map_terrain_is(offset, TERRAIN_TREE | TERRAIN_ROCK | TERRAIN_SHRUB | TERRAIN_MEADOW)) {
        map_terrain_remove(offset, TERRAIN_TREE | TERRAIN_ROCK | TERRAIN_SHRUB | TERRAIN_MEADOW);
    }
    if (!map_terrain_is(offset, TERRAIN_WATER | TERRAIN_ACCESS_RAMP)) {
        map_terrain_add(offset, TERRAIN_ROAD);
    }
}

static int try_place_access_ramp(int x, int y)
{
    // Try all 4 tiles of a potential 2x2 ramp placement around (x, y)
    // The ramp top-left corner can be at (x, y), (x-1, y), (x, y-1), or (x-1, y-1)
    int offsets_x[] = {0, -1, 0, -1};
    int offsets_y[] = {0, 0, -1, -1};

    for (int i = 0; i < 4; i++) {
        int rx = x + offsets_x[i];
        int ry = y + offsets_y[i];

        if (!map_grid_is_inside(rx, ry, 2)) {
            continue;
        }

        map_tile tile;
        tile.x = rx;
        tile.y = ry;
        tile.grid_offset = map_grid_offset(rx, ry);

        int orientation = 0;
        if (editor_tool_can_place_access_ramp(&tile, &orientation)) {
            // Clear terrain on the 2x2 area
            int terrain_mask = ~(TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING |
                TERRAIN_GARDEN | TERRAIN_AQUEDUCT | TERRAIN_TREE | TERRAIN_SHRUB | TERRAIN_MEADOW);
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < 2; dx++) {
                    int grid_offset = tile.grid_offset + map_grid_delta(dx, dy);
                    map_terrain_set(grid_offset, map_terrain_get(grid_offset) & terrain_mask);
                    map_property_clear_future_earthquake(grid_offset);
                }
            }
            // Place the access ramp
            map_building_tiles_add(0, rx, ry, 2,
                image_group(GROUP_TERRAIN_ACCESS_RAMP) + orientation, TERRAIN_ACCESS_RAMP);
            return 1;
        }
    }
    return 0;
}

static void flatten_tile(int x, int y)
{
    int offset = map_grid_offset(x, y);
    if (!map_grid_is_valid_offset(offset)) {
        return;
    }
    map_terrain_remove(offset, TERRAIN_ELEVATION | TERRAIN_TREE | TERRAIN_ROCK |
        TERRAIN_SHRUB | TERRAIN_MEADOW);
    map_elevation_set(offset, 0);
}

static void place_road_straight(int *cx, int *cy, int tx, int ty, int move_x)
{
    // Walk in a straight line along one axis only
    // When hitting elevation changes, try to place access ramps (stairs)
    if (move_x) {
        int step = (tx > *cx) ? 1 : -1;
        while (*cx != tx) {
            int cur_offset = map_grid_offset(*cx, *cy);
            int next_offset = map_grid_offset(*cx + step, *cy);
            int cur_elev = map_grid_is_valid_offset(cur_offset) ? map_elevation_at(cur_offset) : 0;
            int next_elev = map_grid_is_valid_offset(next_offset) ? map_elevation_at(next_offset) : 0;

            if (cur_elev != next_elev) {
                // Elevation transition - try to place access ramp
                if (!try_place_access_ramp(*cx, *cy) &&
                    !try_place_access_ramp(*cx + step, *cy)) {
                    // Fallback: flatten the next tile
                    flatten_tile(*cx + step, *cy);
                }
            }

            place_road_tile(*cx, *cy);
            *cx += step;
        }
    } else {
        int step = (ty > *cy) ? 1 : -1;
        while (*cy != ty) {
            int cur_offset = map_grid_offset(*cx, *cy);
            int next_offset = map_grid_offset(*cx, *cy + step);
            int cur_elev = map_grid_is_valid_offset(cur_offset) ? map_elevation_at(cur_offset) : 0;
            int next_elev = map_grid_is_valid_offset(next_offset) ? map_elevation_at(next_offset) : 0;

            if (cur_elev != next_elev) {
                // Elevation transition - try to place access ramp
                if (!try_place_access_ramp(*cx, *cy) &&
                    !try_place_access_ramp(*cx, *cy + step)) {
                    // Fallback: flatten the next tile
                    flatten_tile(*cx, *cy + step);
                }
            }

            place_road_tile(*cx, *cy);
            *cy += step;
        }
    }
}

#define MAX_TURNS 10

static void build_road_between(int x1, int y1, int x2, int y2, mapgen_rng *rng)
{
    int width = map_grid_width();
    int height = map_grid_height();

    // Generate 2-5 random turn points for 90-degree bends
    int num_turns = mapgen_rng_between(rng, 2, 6);

    // Turn points: each is an (x, y) where the road makes a 90-degree turn
    // Road alternates between horizontal and vertical segments
    int turn_x[MAX_TURNS];
    int turn_y[MAX_TURNS];

    // Distribute turn x-positions evenly between start and end with some randomness
    for (int i = 0; i < num_turns; i++) {
        float t = (float)(i + 1) / (float)(num_turns + 1);
        int base_x = x1 + (int)((x2 - x1) * t);
        // Add random lateral offset for the y position
        int spread_y = height / 4;
        int mid_y = y1 + (int)((y2 - y1) * t);
        int offset_y = mapgen_rng_between(rng, -spread_y, spread_y + 1);

        turn_x[i] = base_x;
        turn_y[i] = mid_y + offset_y;

        // Clamp to map bounds
        if (turn_x[i] < 2) turn_x[i] = 2;
        if (turn_x[i] > width - 3) turn_x[i] = width - 3;
        if (turn_y[i] < 2) turn_y[i] = 2;
        if (turn_y[i] > height - 3) turn_y[i] = height - 3;
    }

    // Build the road: start -> turn1 -> turn2 -> ... -> end
    // Each segment is an L-shape: horizontal then vertical (or vice versa, alternating)
    int cx = x1;
    int cy = y1;

    for (int i = 0; i < num_turns; i++) {
        int tx = turn_x[i];
        int ty = turn_y[i];

        if (i % 2 == 0) {
            // Horizontal first, then vertical
            place_road_straight(&cx, &cy, tx, cy, 1);
            place_road_straight(&cx, &cy, tx, ty, 0);
        } else {
            // Vertical first, then horizontal
            place_road_straight(&cx, &cy, cx, ty, 0);
            place_road_straight(&cx, &cy, tx, ty, 1);
        }
    }

    // Final segment to destination: horizontal then vertical
    place_road_straight(&cx, &cy, x2, cy, 1);
    place_road_straight(&cx, &cy, x2, y2, 0);
    place_road_tile(x2, y2);
}

static void place_entry_exit(const mapgen_config *config, mapgen_rng *rng)
{
    if (!config->place_entry_exit) {
        return;
    }

    int width = map_grid_width();
    int height = map_grid_height();

    // Entry point on the right edge (x = width - 1), exact map boundary
    int entry_x = width - 1;
    int entry_y = find_edge_position(entry_x, 1, 0, height, rng);

    // Exit point on the left edge (x = 0), exact map boundary
    int exit_x = 0;
    int exit_y = find_edge_position(exit_x, 1, 0, height, rng);

    // Clear terrain around entry and exit so flags can be placed
    clear_terrain_at(entry_x, entry_y, 1);
    clear_terrain_at(exit_x, exit_y, 1);

    // Set the scenario entry/exit points
    scenario_editor_set_entry_point(entry_x, entry_y);
    scenario_editor_set_exit_point(exit_x, exit_y);

    // Build a curving 1-tile-wide road connecting entry to exit
    // Road starts 1 tile inward from each edge
    int road_start_x = entry_x > 0 ? entry_x - 1 : entry_x + 1;
    int road_end_x = exit_x < width - 1 ? exit_x + 1 : exit_x - 1;
    build_road_between(road_start_x, entry_y, road_end_x, exit_y, rng);
}

static void place_fishing_points(const mapgen_config *config)
{
    if (!config->place_fishing) {
        return;
    }

    int width = map_grid_width();
    int height = map_grid_height();
    int fish_count = 0;

    for (int y = 3; y < height - 3 && fish_count < 8; y += 8) {
        for (int x = 3; x < width - 3 && fish_count < 8; x += 8) {
            int offset = map_grid_offset(x, y);
            if (!map_grid_is_valid_offset(offset)) {
                continue;
            }
            if (!map_terrain_is(offset, TERRAIN_WATER)) {
                continue;
            }

            // Fish must be at least 2 tiles from shore (all water within radius 2)
            int all_water = 1;
            for (int dy = -2; dy <= 2 && all_water; dy++) {
                for (int dx = -2; dx <= 2 && all_water; dx++) {
                    int n_offset = map_grid_offset(x + dx, y + dy);
                    if (!map_grid_is_valid_offset(n_offset) || !map_terrain_is(n_offset, TERRAIN_WATER)) {
                        all_water = 0;
                    }
                }
            }
            if (!all_water) {
                continue;
            }

            // But shore should be reachable nearby (within 5 tiles)
            int near_shore = 0;
            for (int dy = -5; dy <= 5 && !near_shore; dy++) {
                for (int dx = -5; dx <= 5 && !near_shore; dx++) {
                    int n_offset = map_grid_offset(x + dx, y + dy);
                    if (map_grid_is_valid_offset(n_offset) && !map_terrain_is(n_offset, TERRAIN_WATER)) {
                        near_shore = 1;
                    }
                }
            }

            if (near_shore) {
                scenario_editor_set_fishing_point(fish_count, x, y);
                fish_count++;
            }
        }
    }
}

static void place_herd_points(const mapgen_config *config, mapgen_rng *rng)
{
    if (!config->place_herds) {
        return;
    }

    int width = map_grid_width();
    int height = map_grid_height();
    int herd_count = 0;

    // Try random positions to find meadow/grass areas
    for (int attempts = 0; attempts < 200 && herd_count < 4; attempts++) {
        int x = mapgen_rng_between(rng, 5, width - 5);
        int y = mapgen_rng_between(rng, 5, height - 5);

        int offset = map_grid_offset(x, y);
        if (!map_grid_is_valid_offset(offset)) {
            continue;
        }

        // Prefer meadow, but accept clear terrain
        int terrain = map_terrain_get(offset);
        if (terrain & (TERRAIN_WATER | TERRAIN_ROCK | TERRAIN_BUILDING | TERRAIN_ELEVATION)) {
            continue;
        }

        // Check there's enough open space around
        int open_count = 0;
        for (int dy = -3; dy <= 3; dy++) {
            for (int dx = -3; dx <= 3; dx++) {
                int n_offset = map_grid_offset(x + dx, y + dy);
                if (map_grid_is_valid_offset(n_offset) &&
                    !map_terrain_is(n_offset, TERRAIN_WATER | TERRAIN_ROCK | TERRAIN_ELEVATION)) {
                    open_count++;
                }
            }
        }

        if (open_count >= 30) {
            scenario_editor_set_herd_point(herd_count, x, y);
            herd_count++;
        }
    }
}

static void update_tile_images(void)
{
    map_elevation_remove_cliffs();
    map_image_context_reset_water();
    map_image_context_reset_elevation();
    map_tiles_update_all_elevation_editor();
    map_tiles_update_all_water();
    map_tiles_update_all_rocks();
    map_tiles_update_all_empty_land();
    map_tiles_update_all_meadow();
    map_tiles_update_all_roads();
    map_routing_update_all();
}

void mapgen_config_init_defaults(mapgen_config *config)
{
    config->seed = 12345;
    config->map_size = 2; // 80x80

    config->tree_density = 40;
    config->rock_density = 20;
    config->shrub_density = 15;
    config->meadow_density = 25;

    config->elevation_enabled = 0;
    config->max_elevation = 3;
    config->flatness = 50;

    config->river_count = 0;
    config->lake_count = 1;
    config->lake_size = 2;
    config->coastal = 0;

    config->place_entry_exit = 1;
    config->place_fishing = 1;
    config->place_herds = 1;
}

int mapgen_generate(const mapgen_config *config)
{
    mapgen_rng rng;
    mapgen_rng_seed(&rng, config->seed);

    mapgen_noise noise;
    mapgen_noise_init(&noise, &rng);

    // Phase 1: Water first (so terrain avoids water areas)
    generate_coastal(config, &noise);
    generate_lakes(config, &noise, &rng);
    generate_rivers(config, &noise, &rng);

    // Phase 2: Elevation (before vegetation, so we can clear vegetation on heights)
    generate_elevation(config, &noise);

    // Phase 3: Vegetation and terrain features (skip water tiles)
    generate_terrain(config, &noise);

    // Phase 4: Game features
    place_entry_exit(config, &rng);
    place_fishing_points(config);
    place_herd_points(config, &rng);

    // Phase 5: Update all tile images using existing pipeline
    update_tile_images();

    return 1;
}
