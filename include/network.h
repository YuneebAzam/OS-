#ifndef NETWORK_H
#define NETWORK_H

#include <curl/curl.h>
#include "common.h"

// Write callback to write downloaded data into a file
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);

// Progress callback for download monitoring
int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

// Function to download a file segment
int downloadFileSegment(const char *url, const char *outputPath, long start, long end);

// Get file size from URL
long getFileSize(const char *url);

#endif // NETWORK_H