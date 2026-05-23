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

// Callback to handle headers during GET request
static size_t header_callback(char *buffer, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    long *filesize = (long *)userp;
    
    char *contentlen = strstr(buffer, "Content-Length:");
    if (contentlen)
    {
        *filesize = atol(contentlen + 15);
    }
    
    return realsize;
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
    
    // Handle URL parameters - extract filename before '?'
    char *result = strdup(filename);
    char *question = strchr(result, '?');
    if (question)
    {
        *question = '\0';  // Truncate at '?'
    }
    
    // If filename is empty, use default
    if (strlen(result) == 0)
    {
        free(result);
        return strdup("downloaded_file.dat");
    }
    
    return result;
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
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_RANGE, range);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 0L);  // No timeout for downloads
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 30L);  // But connection timeout

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
    long alternativeSize = -1;

    if (!url)
    {
        fprintf(stderr, "Error: URL is NULL\n");
        return -1;
    }

    printf("🔍 Probing file size (Method 1: HEAD request)...\n");

    // ===== METHOD 1: Try HEAD request first (fastest) =====
    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "Error: curl_easy_init() failed\n");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);              // HEAD request
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);      // Follow redirects
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);           // Allow up to 5 redirects
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);            // 10 second timeout
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);      // Don't verify SSL
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "file-downloader/1.0");

    res = curl_easy_perform(curl);
    
    if (res == CURLE_OK)
    {
        // Try to get content length from HEAD response
        #if LIBCURL_VERSION_NUM >= 0x073700
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &fileSize);
        #else
            double tempSize;
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &tempSize);
            fileSize = (res == CURLE_OK && tempSize >= 0) ? (curl_off_t)tempSize : -1;
        #endif

        if (res == CURLE_OK && fileSize > 0)
        {
            printf("✓ File size detected via HEAD request: %ld bytes\n", (long)fileSize);
            curl_easy_cleanup(curl);
            return (long)fileSize;
        }
    }

    printf("⚠️  HEAD request failed or no Content-Length. Trying Method 2: GET request with range...\n");
    curl_easy_cleanup(curl);

    // ===== METHOD 2: Use GET request and read headers =====
    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "Error: curl_easy_init() failed (retry)\n");
        return -1;
    }

    alternativeSize = -1;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);              // GET request (not HEAD)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "file-downloader/1.0");
    curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");            // Request only first byte to get headers
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)&alternativeSize);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // Discard body
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);

    res = curl_easy_perform(curl);
    
    if (res == CURLE_OK && alternativeSize > 0)
    {
        printf("✓ File size detected via GET request: %ld bytes\n", alternativeSize);
        curl_easy_cleanup(curl);
        return alternativeSize;
    }

    printf("⚠️  GET method failed too. Attempting final fallback...\n");
    curl_easy_cleanup(curl);

    // ===== METHOD 3: Direct GET and count bytes (last resort) =====
    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "❌ Error: Could not initialize curl for final attempt\n");
        return -1;
    }

    FILE *temp_file = fopen("/tmp/size_probe.tmp", "wb");
    if (!temp_file)
    {
        fprintf(stderr, "❌ Error: Could not create temporary file\n");
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "file-downloader/1.0");
    curl_easy_setopt(curl, CURLOPT_RANGE, "0-99");           // Get first 100 bytes
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, temp_file);

    res = curl_easy_perform(curl);
    fclose(temp_file);

    if (res == CURLE_OK)
    {
        curl_off_t contentLength = -1;
        #if LIBCURL_VERSION_NUM >= 0x073700
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength);
        #else
            double tempSize;
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &tempSize);
            contentLength = (tempSize >= 0) ? (curl_off_t)tempSize : -1;
        #endif

        if (contentLength > 0)
        {
            printf("✓ File size detected via partial GET: %ld bytes\n", (long)contentLength);
            curl_easy_cleanup(curl);
            remove("/tmp/size_probe.tmp");
            return (long)contentLength;
        }
    }

    curl_easy_cleanup(curl);
    remove("/tmp/size_probe.tmp");

    fprintf(stderr, "❌ Error: Could not determine file size after all methods\n");
    fprintf(stderr, "    Possible reasons:\n");
    fprintf(stderr, "    - Server doesn't send Content-Length header\n");
    fprintf(stderr, "    - Server doesn't support Range requests\n");
    fprintf(stderr, "    - Network connectivity issue\n");
    return -1;
}
