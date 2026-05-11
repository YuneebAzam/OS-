#include "network.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

size_t write_data(void *contents, size_t size, size_t nmemb, void *userp)
{
    FILE *file = (FILE *)userp;
    size_t written = fwrite(contents, size, nmemb, file);

    return written * size;
}

int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    (void)dltotal;
    (void)ultotal;
    (void)ulnow;

    DownloadProgress *progress = (DownloadProgress *)clientp;

    if (!progress)
        return 0;

    pthread_mutex_lock(&progress->mutex);
    if (!progress->isPaused)
        progress->downloadedBytes = dlnow;

    int shouldPause = progress->isPaused;
    pthread_mutex_unlock(&progress->mutex);

    return shouldPause ? 1 : 0;
}

char *get_filename_from_url(const char *url)
{
    if (!url)
        return strdup("downloaded_file.dat");

    const char *last_slash = strrchr(url, '/');
    const char *filename = (last_slash && *(last_slash + 1)) ? last_slash + 1 : "downloaded_file.dat";
    return strdup(filename);
}

int downloadFileSegment(const char *url, const char *outputPath, long start, long end)
{
    CURL *curl_handle = NULL;
    CURLcode res;
    FILE *file = fopen(outputPath, "wb");

    if (!file)
    {
        perror("Error opening output file");
        return 1;
    }

    curl_handle = curl_easy_init();

    if (!curl_handle)
    {
        fprintf(stderr, "Failed to initialize libcurl\n");
        fclose(file);
        return 1;
    }

    char range[64];
    snprintf(range, sizeof(range), "%ld-%ld", start, end);

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_RANGE, range);

    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl_handle);
        fclose(file);
        return 1;
    }

    curl_easy_cleanup(curl_handle);
    fclose(file);
    return 0;
}

long getFileSize(const char *url)
{
    CURL *curl = NULL;
    CURLcode res;
    curl_off_t fileSize = -1;

    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "Error: curl_easy_init() failed\n");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "file-downloader/1.0");

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "Error: CURL request failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }

#if LIBCURL_VERSION_NUM >= 0x073700
    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &fileSize);
#else
    double tempSize;
    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &tempSize);
    fileSize = (res == CURLE_OK && tempSize >= 0) ? (curl_off_t)tempSize : -1;
#endif

    if (res != CURLE_OK || fileSize < 0)
    {
        fprintf(stderr, "Error: Could not determine file size: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_cleanup(curl);
    return (long)fileSize;
}