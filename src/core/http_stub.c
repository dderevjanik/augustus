#include "core/http.h"

#include "core/log.h"

#include <string.h>

int http_is_available(void)
{
    return 0;
}

int http_get(const char *url, http_response *response)
{
    memset(response, 0, sizeof(http_response));
    log_info("HTTP not supported on this platform", url, 0);
    return 0;
}

int http_download_file(const char *url, const char *filepath)
{
    log_info("HTTP not supported on this platform", url, 0);
    return 0;
}

void http_response_free(http_response *response)
{
    if (response) {
        memset(response, 0, sizeof(http_response));
    }
}
