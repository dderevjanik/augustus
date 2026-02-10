#ifndef GAME_SCENARIO_DOWNLOAD_H
#define GAME_SCENARIO_DOWNLOAD_H

/**
 * @file
 * Scenario download list management - fetch, parse, and download community scenarios.
 */

#define SCENARIO_DL_MAX_SCENARIOS 512
#define SCENARIO_DL_FIELD_MAX 256
#define SCENARIO_DL_API_URL "http://localhost:8080/api/scenarios"

typedef struct {
    char name[SCENARIO_DL_FIELD_MAX];
    char author[SCENARIO_DL_FIELD_MAX];
    char rating[32];
    char updated_at[32];
    char released_at[32];
    char checksum[64];
    char download_url[SCENARIO_DL_FIELD_MAX];
    int is_downloaded;
} scenario_download_entry;

typedef struct {
    scenario_download_entry entries[SCENARIO_DL_MAX_SCENARIOS];
    int count;
    int fetched; /**< 0=not fetched, 1=success, -1=error */
} scenario_download_list;

/**
 * Fetches the scenario list from the remote server and parses the CSV response.
 * Also checks local files for matching checksums.
 * @return 1 on success, 0 on failure.
 */
int scenario_download_fetch(void);

/**
 * Returns pointer to the current scenario download list.
 */
const scenario_download_list *scenario_download_get_list(void);

/**
 * Downloads the scenario at the given index to the local scenario directory.
 * @param index Index into the entries array.
 * @return 1 on success, 0 on failure.
 */
int scenario_download_file(int index);

#endif // GAME_SCENARIO_DOWNLOAD_H
