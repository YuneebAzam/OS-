#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <curl/curl.h>
#include <stddef.h>

// Maximum number of threads for downloading
#define MAX_THREADS 8

// Buffer size for file operations
#define BUFFER_SIZE 8192

// Structure to represent a file segment
typedef struct
{
    long start;       // Start byte of the segment
    long end;         // End byte of the segment
    char *url;        // URL of the file, remove const to allow freeing
    char *outputPath; // Path to save the downloaded segment, remove const to allow freeing
    int segmentId;    // Segment identifier
    int isCompleted;  // Flag to indicate if the segment is downloaded
} FileSegment;

// Structure to hold download progress
typedef struct
{
    pthread_mutex_t mutex; // Mutex for thread synchronization
    long downloadedBytes;  // Total downloaded bytes
    long totalBytes;       // Total bytes to download
    int completedSegments; // Number of completed segments
    int totalSegments;     // Total number of segments
    int isPaused;          // Flag to indicate if the download is paused
} DownloadProgress;

// Function declarations
long getFileSize(const char *url);

// Extract filename from URL
char *get_filename_from_url(const char *url);

#endif // COMMON_H