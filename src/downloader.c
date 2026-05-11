#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "network.h"
#include "segment.h"
#include "writer.h"
#include <limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "downloader.h"
#include "monitor.h"
// Create temporary segment file paths
char *createSegmentPath(const char *basePath, int segmentId)
{
    // Allocate memory for the segment path (basePath + ".partN")
    char *segmentPath = malloc(strlen(basePath) + 16);
    if (!segmentPath)
    {
        return NULL;
    }
    sprintf(segmentPath, "%s.part%d", basePath, segmentId);
    return segmentPath;
}

// Function to download a single segment
void *downloadSegmentThread(void *arg)
{
    if (!arg)
        return NULL;

    FileSegment *segment = (FileSegment *)arg;
    downloadSegment(segment);
    return NULL;
}

// Start a download with multiple threads
int startThreadedDownload(const char *url, const char *outputPath, int numThreads, DownloadProgress *progress)
{
    if (!url || !outputPath)
        return -1;

    if (numThreads > MAX_THREADS)
        numThreads = MAX_THREADS;
    if (numThreads <= 0)
        numThreads = 1;

    // Get file size
    long fileSize = getFileSize(url);
    if (fileSize <= 0)
    {
        fprintf(stderr, "Failed to get file size for %s\n", url);
        return -1;
    }

    printf("File size: %ld bytes\n", fileSize);

    // Create segments
    FileSegment *segments = malloc(sizeof(FileSegment) * numThreads);
    if (!segments)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    // Initialize segments
    long segmentSize = fileSize / numThreads;
    for (int i = 0; i < numThreads; i++)
    {
        segments[i].start = i * segmentSize;
        segments[i].end = (i == numThreads - 1) ? fileSize - 1 : (i + 1) * segmentSize - 1;
        segments[i].url = strdup(url);
        segments[i].outputPath = createSegmentPath(outputPath, i);
        segments[i].segmentId = i;
        segments[i].isCompleted = 0;

        if (!segments[i].url || !segments[i].outputPath)
        {
            fprintf(stderr, "Memory allocation failed for segment %d\n", i);
            // Clean up segments that were successfully allocated
            for (int j = 0; j < i; j++)
            {
                free(segments[j].url);
                free(segments[j].outputPath);
            }
            free(segments);
            return -1;
        }
    }

    // Initialize progress tracker
    if (progress)
    {
        pthread_mutex_init(&progress->mutex, NULL);
        progress->downloadedBytes = 0;
        progress->totalBytes = fileSize;
        progress->completedSegments = 0;
        progress->totalSegments = numThreads;
        progress->isPaused = 0;
    }

    // Create threads
    pthread_t *threads = malloc(sizeof(pthread_t) * numThreads);
    if (!threads)
    {
        fprintf(stderr, "Memory allocation failed for threads\n");

        // Clean up segments
        freeSegments(segments, numThreads);
        free(segments);

        return -1;
    }

    // Start download threads
    for (int i = 0; i < numThreads; i++)
    {
        if (pthread_create(&threads[i], NULL, downloadSegmentThread, &segments[i]) != 0)
        {
            fprintf(stderr, "Thread creation failed for segment %d\n", i);
            // Continue with other threads
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Check if all segments were completed
    int allCompleted = 1;
    for (int i = 0; i < numThreads; i++)
    {
        if (!segments[i].isCompleted)
        {
            allCompleted = 0;
            fprintf(stderr, "Segment %d failed to download\n", i);
        }
    }

    // Merge segments if all completed
    if (allCompleted)
    {
        printf("Merging segments...\n");
        if (mergeSegments(outputPath, segments, numThreads) != 0)
        {
            fprintf(stderr, "Error: Failed to merge segments\n");
            allCompleted = 0;
        }
    }

    // Clean up
    free(threads);
    freeSegments(segments, numThreads);
    free(segments);

    if (progress)
    {
        pthread_mutex_destroy(&progress->mutex);
    }

    return allCompleted ? 0 : -1;
}
void startDownload(const char *url, Monitor *monitor)
{
    if (!url || !monitor)
    {
        fprintf(stderr, "Error: Invalid parameters\n");
        return;
    }

    // Initialize CURL globally once
    curl_global_init(CURL_GLOBAL_ALL);

    printf("Getting file size for: %s\n", url);
    long fileSize = getFileSize(url);
    if (fileSize <= 0)
    {
        fprintf(stderr, "Error: Failed to get file size.\n");
        curl_global_cleanup();
        return;
    }

    printf("File size: %ld bytes\n", fileSize);

    // Get system Downloads directory
    const char *homedir = getenv("HOME");
    if (!homedir)
    {
        struct passwd *pw = getpwuid(getuid());
        homedir = pw ? pw->pw_dir : NULL;
    }
    if (!homedir)
    {
        fprintf(stderr, "Error: Could not determine home directory\n");
        curl_global_cleanup();
        return;
    }

    char downloadsDir[PATH_MAX];
    snprintf(downloadsDir, sizeof(downloadsDir), "%s/Downloads", homedir);

    // Optionally, ensure Downloads directory exists
    struct stat st = {0};
    if (stat(downloadsDir, &st) == -1)
    {
        if (mkdir(downloadsDir, 0755) == -1)
        {
            fprintf(stderr, "Error: Could not create Downloads directory\n");
            curl_global_cleanup();
            return;
        }
    }

    // Get base filename for output and prepend Downloads directory
    char *filename = get_filename_from_url(url);
    if (!filename)
    {
        fprintf(stderr, "Error: Could not determine output filename\n");
        curl_global_cleanup();
        return;
    }

    // Create full path with Downloads directory
    char *baseOutputPath = malloc(strlen(downloadsDir) + 1 + strlen(filename) + 1);
    if (!baseOutputPath)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(filename);
        curl_global_cleanup();
        return;
    }
    sprintf(baseOutputPath, "%s/%s", downloadsDir, filename);
    free(filename);

    // Add download to monitor
    addDownload(monitor, baseOutputPath);

    // Determine number of threads based on file size
    int numThreads = MAX_THREADS;
    if (fileSize < 1024 * 1024) // If file is smaller than 1MB
        numThreads = 1;
    else if (fileSize < 10 * 1024 * 1024) // If file is smaller than 10MB
        numThreads = 4;

    // Divide file into segments with proper paths
    FileSegment *segments = malloc(numThreads * sizeof(FileSegment));
    if (!segments)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        free(baseOutputPath);
        curl_global_cleanup();
        return;
    }

    // Calculate segments
    long segmentSize = fileSize / numThreads;
    for (int i = 0; i < numThreads; i++)
    {
        segments[i].start = i * segmentSize;
        segments[i].end = (i == numThreads - 1) ? fileSize - 1 : (i + 1) * segmentSize - 1;
        segments[i].url = strdup(url);
        segments[i].outputPath = createSegmentPath(baseOutputPath, i);
        segments[i].segmentId = i;
        segments[i].isCompleted = 0;

        if (!segments[i].url || !segments[i].outputPath)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            // Cleanup previous segments
            for (int j = 0; j < i; j++)
            {
                free(segments[j].url);
                free(segments[j].outputPath);
            }
            free(segments);
            free(baseOutputPath);
            curl_global_cleanup();
            return;
        }
    }

    // Initialize progress tracking
    DownloadProgress progress;
    if (pthread_mutex_init(&progress.mutex, NULL) != 0)
    {
        fprintf(stderr, "Error: Mutex init failed\n");
        freeSegments(segments, numThreads);
        free(baseOutputPath);
        curl_global_cleanup();
        return;
    }

    progress.downloadedBytes = 0;
    progress.totalBytes = fileSize;
    progress.completedSegments = 0;
    progress.totalSegments = numThreads;
    progress.isPaused = 0;

    // Start download threads
    pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
    if (!threads)
    {
        fprintf(stderr, "Error: Memory allocation failed for threads\n");
        pthread_mutex_destroy(&progress.mutex);
        freeSegments(segments, numThreads);
        free(baseOutputPath);
        curl_global_cleanup();
        return;
    }

    for (int i = 0; i < numThreads; i++)
    {
        if (pthread_create(&threads[i], NULL, downloadSegment, &segments[i]) != 0)
        {
            fprintf(stderr, "Error: Thread creation failed\n");
            // We'll continue with other threads
        }
    }

    // Wait for threads to complete
    for (int i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Check if all segments completed successfully
    if (allSegmentsCompleted(segments, numThreads))
    {
        printf("All segments downloaded successfully. Merging...\n");
        if (mergeSegments(baseOutputPath, segments, numThreads) == 0)
        {
            printf("Download completed successfully: %s\n", baseOutputPath);
            updateProgress(monitor, baseOutputPath, 100);
            printf("File saved to Downloads.\n");
        }
        else
        {
            fprintf(stderr, "Error: Failed to merge segments\n");
            updateProgress(monitor, baseOutputPath, -1);
        }
    }
    else
    {
        fprintf(stderr, "Error: Some segments failed to download\n");
        updateProgress(monitor, baseOutputPath, -1);
    }

    // Cleanup
    pthread_mutex_destroy(&progress.mutex);
    free(threads);
    freeSegments(segments, numThreads);
    free(segments);
    free(baseOutputPath);
    curl_global_cleanup();
}
