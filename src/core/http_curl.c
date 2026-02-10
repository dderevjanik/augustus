#include "core/http.h"

#include "core/log.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define HTTP_GET_TIMEOUT 30L
#define HTTP_DOWNLOAD_TIMEOUT 300L
#define HTTP_USER_AGENT "Augustus"

struct write_buffer {
    uint8_t *data;
    size_t size;
    size_t capacity;
};

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t real_size = size * nmemb;
    struct write_buffer *buf = (struct write_buffer *) userp;

    if (buf->size + real_size > buf->capacity) {
        size_t new_capacity = buf->capacity ? buf->capacity * 2 : 4096;
        while (new_capacity < buf->size + real_size) {
            new_capacity *= 2;
        }
        uint8_t *new_data = realloc(buf->data, new_capacity);
        if (!new_data) {
            return 0;
        }
        buf->data = new_data;
        buf->capacity = new_capacity;
    }
    memcpy(buf->data + buf->size, contents, real_size);
    buf->size += real_size;
    return real_size;
}

int http_is_available(void)
{
    return 1;
}

int http_get(const char *url, http_response *response)
{
    CURL *curl;
    CURLcode res;
    long http_code = 0;
    struct write_buffer buf = {0};

    memset(response, 0, sizeof(http_response));

    curl = curl_easy_init();
    if (!curl) {
        log_error("HTTP: failed to initialize curl", 0, 0);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_GET_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, HTTP_USER_AGENT);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log_error("HTTP GET failed for url", url, 0);
        free(buf.data);
        curl_easy_cleanup(curl);
        return 0;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    response->data = buf.data;
    response->size = buf.size;
    response->status_code = (int) http_code;

    curl_easy_cleanup(curl);
    return 1;
}

int http_download_file(const char *url, const char *filepath)
{
    CURL *curl;
    CURLcode res;
    FILE *fp;

    fp = fopen(filepath, "wb");
    if (!fp) {
        log_error("HTTP: cannot open file for writing", filepath, 0);
        return 0;
    }

    curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        log_error("HTTP: failed to initialize curl", 0, 0);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_DOWNLOAD_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, HTTP_USER_AGENT);

    res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_error("HTTP download failed for url", url, 0);
        remove(filepath);
        return 0;
    }
    return 1;
}

void http_response_free(http_response *response)
{
    if (response) {
        free(response->data);
        memset(response, 0, sizeof(http_response));
    }
}
