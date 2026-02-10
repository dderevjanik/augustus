#ifndef CORE_HTTP_H
#define CORE_HTTP_H

#include <stddef.h>
#include <stdint.h>

/**
 * @file
 * HTTP client functions for downloading data from the network.
 */

typedef struct {
    uint8_t *data;
    size_t size;
    int status_code;
} http_response;

/**
 * Checks whether HTTP support is available on this platform.
 * @return 1 if HTTP requests may succeed, 0 if stubbed out.
 */
int http_is_available(void);

/**
 * Performs a synchronous HTTP GET request and returns the response in memory.
 * On success, the caller must free the response using http_response_free().
 * @param url The URL to request (must include scheme, e.g. "https://...").
 * @param response Pointer to an http_response struct to populate.
 * @return 1 on success, 0 on failure.
 */
int http_get(const char *url, http_response *response);

/**
 * Downloads a file from the given URL and saves it to the specified local path.
 * Streams directly to disk to avoid holding the entire file in memory.
 * @param url The URL to download from.
 * @param filepath The local filesystem path to save the file to.
 * @return 1 on success, 0 on failure.
 */
int http_download_file(const char *url, const char *filepath);

/**
 * Frees the memory associated with an HTTP response.
 * Safe to call on a zeroed-out response struct.
 * @param response The response to free.
 */
void http_response_free(http_response *response);

#endif // CORE_HTTP_H
