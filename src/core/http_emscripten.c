#include "core/http.h"

#include "core/log.h"

#include <emscripten.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int http_is_available(void)
{
    return 1;
}

int http_get(const char *url, http_response *response)
{
    void *buf = NULL;
    int size = 0;
    int error = 0;

    memset(response, 0, sizeof(http_response));

    emscripten_wget_data(url, &buf, &size, &error);
    if (error) {
        log_error("HTTP GET failed for url", url, error);
        free(buf);
        return 0;
    }

    response->data = (uint8_t *) buf;
    response->size = (size_t) size;
    response->status_code = 200;
    return 1;
}

int http_download_file(const char *url, const char *filepath)
{
    FILE *fp;

    emscripten_wget(url, filepath);

    fp = fopen(filepath, "rb");
    if (!fp) {
        log_error("HTTP download failed for url", url, 0);
        return 0;
    }
    fclose(fp);
    return 1;
}

void http_response_free(http_response *response)
{
    if (response) {
        free(response->data);
        memset(response, 0, sizeof(http_response));
    }
}
