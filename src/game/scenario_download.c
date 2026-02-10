#include "game/scenario_download.h"

#include "core/dir.h"
#include "core/file.h"
#include "core/http.h"
#include "core/log.h"
#include "platform/file_manager.h"

#include "miniz/miniz.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ_BUFFER_SIZE 65536

static scenario_download_list data;

static void copy_csv_field(const char *start, int length, char *dest, int dest_size)
{
    if (length >= dest_size) {
        length = dest_size - 1;
    }
    if (length > 0) {
        memcpy(dest, start, length);
    }
    dest[length] = '\0';
}

static int parse_csv_line(const char *line, scenario_download_entry *entry)
{
    const char *field_start = line;
    int col = 0;

    memset(entry, 0, sizeof(scenario_download_entry));

    while (*line && col < 7) {
        if (*line == ',' || *line == '\n' || *line == '\r' || *(line + 1) == '\0') {
            int length = (int) (line - field_start);
            if (*(line + 1) == '\0' && *line != ',' && *line != '\n' && *line != '\r') {
                length++;
            }
            switch (col) {
                case 0: copy_csv_field(field_start, length, entry->name, SCENARIO_DL_FIELD_MAX); break;
                case 1: copy_csv_field(field_start, length, entry->author, SCENARIO_DL_FIELD_MAX); break;
                case 2: copy_csv_field(field_start, length, entry->rating, 32); break;
                case 3: copy_csv_field(field_start, length, entry->updated_at, 32); break;
                case 4: copy_csv_field(field_start, length, entry->released_at, 32); break;
                case 5: copy_csv_field(field_start, length, entry->checksum, 64); break;
                case 6: copy_csv_field(field_start, length, entry->download_url, SCENARIO_DL_FIELD_MAX); break;
            }
            col++;
            if (*line == ',') {
                field_start = line + 1;
            }
        }
        line++;
    }
    return col >= 7 && entry->name[0] != '\0';
}

static unsigned long compute_file_crc32(const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        return 0;
    }

    unsigned char buf[READ_BUFFER_SIZE];
    mz_ulong crc = MZ_CRC32_INIT;
    size_t bytes_read;

    while ((bytes_read = fread(buf, 1, READ_BUFFER_SIZE, fp)) > 0) {
        crc = mz_crc32(crc, buf, bytes_read);
    }
    fclose(fp);
    return crc;
}

static void check_local_scenarios(void)
{
    const dir_listing *map_files = dir_find_files_with_extension_at_location(PATH_LOCATION_SCENARIO, "map");
    const dir_listing *all_files = dir_append_files_with_extension("mapx");

    for (int i = 0; i < data.count; i++) {
        data.entries[i].is_downloaded = 0;

        if (data.entries[i].checksum[0] == '\0') {
            continue;
        }

        unsigned long expected_crc = strtoul(data.entries[i].checksum, NULL, 16);

        for (int f = 0; f < all_files->num_files; f++) {
            const char *full_path = dir_get_file_at_location(
                all_files->files[f].name, PATH_LOCATION_SCENARIO);
            if (!full_path) {
                continue;
            }
            unsigned long file_crc = compute_file_crc32(full_path);
            if (file_crc == expected_crc) {
                data.entries[i].is_downloaded = 1;
                break;
            }
        }
    }
}

int scenario_download_fetch(void)
{
    http_response response;

    data.count = 0;
    data.fetched = 0;

    if (!http_is_available()) {
        log_info("Scenario download: HTTP not available", 0, 0);
        data.fetched = -1;
        return 0;
    }

    if (!http_get(SCENARIO_DL_API_URL, &response)) {
        log_error("Scenario download: failed to fetch list", 0, 0);
        data.fetched = -1;
        return 0;
    }

    if (response.status_code != 200) {
        log_error("Scenario download: server returned status", 0, response.status_code);
        http_response_free(&response);
        data.fetched = -1;
        return 0;
    }

    // Debug: print raw response
    log_info("Scenario download: response size", 0, (int) response.size);
    if (response.data && response.size > 0) {
        // Print first 2000 chars of response for debugging
        int print_len = response.size > 2000 ? 2000 : (int) response.size;
        char debug_buf[2048];
        memcpy(debug_buf, response.data, print_len);
        debug_buf[print_len] = '\0';
        printf("=== SCENARIO LIST RESPONSE ===\n%s\n=== END RESPONSE ===\n", debug_buf);
    }

    // Parse CSV - skip header line, then parse each subsequent line
    const char *cursor = (const char *) response.data;
    const char *end = cursor + response.size;
    int line_num = 0;

    while (cursor < end && data.count < SCENARIO_DL_MAX_SCENARIOS) {
        const char *line_start = cursor;
        // Find end of line
        while (cursor < end && *cursor != '\n') {
            cursor++;
        }

        if (line_num > 0) {
            // Make a null-terminated copy of the line
            int line_len = (int) (cursor - line_start);
            if (line_len > 0 && line_start[line_len - 1] == '\r') {
                line_len--;
            }
            if (line_len > 0) {
                char line_buf[SCENARIO_DL_FIELD_MAX * 7 + 7];
                if (line_len >= (int) sizeof(line_buf)) {
                    line_len = (int) sizeof(line_buf) - 1;
                }
                memcpy(line_buf, line_start, line_len);
                line_buf[line_len] = '\0';

                if (parse_csv_line(line_buf, &data.entries[data.count])) {
                    data.count++;
                }
            }
        }

        line_num++;
        if (cursor < end) {
            cursor++; // skip '\n'
        }
    }

    http_response_free(&response);
    data.fetched = 1;

    log_info("Scenario download: parsed scenarios", 0, data.count);

    check_local_scenarios();

    return 1;
}

const scenario_download_list *scenario_download_get_list(void)
{
    return &data;
}

int scenario_download_file(int index)
{
    if (index < 0 || index >= data.count) {
        return 0;
    }

    const scenario_download_entry *entry = &data.entries[index];
    if (entry->download_url[0] == '\0') {
        log_error("Scenario download: no download URL for entry", entry->name, 0);
        return 0;
    }

    // Build destination path: scenario directory + filename from name field
    const char *scenario_dir = platform_file_manager_get_directory_for_location(
        PATH_LOCATION_SCENARIO, 0);

    // Extract filename from the download URL (last component) or use name + .map
    char filename[SCENARIO_DL_FIELD_MAX];
    const char *url_last_slash = strrchr(entry->download_url, '/');
    if (url_last_slash && *(url_last_slash + 1)) {
        snprintf(filename, SCENARIO_DL_FIELD_MAX, "%s", url_last_slash + 1);
    } else {
        snprintf(filename, SCENARIO_DL_FIELD_MAX, "%s.map", entry->name);
    }

    char filepath[SCENARIO_DL_FIELD_MAX * 2];
    snprintf(filepath, sizeof(filepath), "%s%s", scenario_dir, filename);

    log_info("Scenario download: downloading to", filepath, 0);

    if (!http_download_file(entry->download_url, filepath)) {
        log_error("Scenario download: download failed", entry->name, 0);
        return 0;
    }

    data.entries[index].is_downloaded = 1;
    return 1;
}
