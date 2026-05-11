#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>

typedef struct
{
    char fileName[256]; // Name of the file being downloaded
    int progress;       // Progress percentage
    int isPaused;       // Flag to indicate if the download is paused
    int retries;        // Number of retries for failed segments
} Download;

typedef struct
{
    Download downloads[100]; // Fixed-size array for simplicity
    int downloadCount;       // Number of active downloads
    pthread_mutex_t mutex;   // Mutex for thread-safe operations
} Monitor;

// Initializes the monitor
void initMonitor(Monitor *monitor);

// Cleans up the monitor
void destroyMonitor(Monitor *monitor);

// Adds a new download to the monitor
void addDownload(Monitor *monitor, const char *fileName);

// Updates the progress of a download
void updateProgress(Monitor *monitor, const char *fileName, int progress);

// Pauses a download
void pauseDownload(Monitor *monitor, const char *fileName);

// Resumes a paused download
void resumeDownload(Monitor *monitor, const char *fileName);

// Removes a download from the monitor
void removeDownload(Monitor *monitor, const char *fileName);

// Displays the status of all downloads
void displayStatus(const Monitor *monitor);

#endif // MONITOR_H