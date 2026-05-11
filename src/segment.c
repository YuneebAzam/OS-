#include "segment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "network.h"

// Function to calculate optimal segment sizes
void calculateSegments(long fileSize, int numThreads, FileSegment *segments, const char *url, const char *outputPath)
{
    if (fileSize <= 0 || numThreads <= 0 || !segments || !url || !outputPath)
    {
        fprintf(stderr, "Invalid arguments to calculateSegments\n");
        return;
    }

    long segmentSize = fileSize / numThreads;

    for (int i = 0; i < numThreads; i++)
    {
        segments[i].start = i * segmentSize;

        // Distribute remainder across segments
        if (i == numThreads - 1)
        {
            segments[i].end = fileSize - 1; // Last segment gets any remainder
        }
        else
        {
            segments[i].end = segments[i].start + segmentSize - 1;
        }

        segments[i].url = strdup(url);
        if (!segments[i].url)
        {
            fprintf(stderr, "Failed to allocate memory for segment URL\n");
            return;
        }

        // Create unique output path for each segment
        segments[i].outputPath = createSegmentOutputPath(outputPath, i);
        if (!segments[i].outputPath)
        {
            fprintf(stderr, "Failed to allocate memory for segment output path\n");
            freeSegments(segments, i); // Free previously allocated segments
            return;
        }

        segments[i].segmentId = i;
        segments[i].isCompleted = 0;

        printf("Segment %d: Bytes %ld-%ld (%ld bytes)\n",
               i, segments[i].start, segments[i].end,
               segments[i].end - segments[i].start + 1);
    }
}

// Function to check if all segments are completed
int allSegmentsCompleted(FileSegment *segments, int numSegments)
{
    if (!segments || numSegments <= 0)
    {
        fprintf(stderr, "Invalid arguments to allSegmentsCompleted\n");
        return 0;
    }

    for (int i = 0; i < numSegments; i++)
    {
        if (!segments[i].isCompleted)
        {
            return 0; // At least one segment is not completed
        }
    }
    return 1; // All segments are completed
}

// Function to free segment resources
void freeSegments(FileSegment *segments, int numSegments)
{
    if (!segments)
        return;

    for (int i = 0; i < numSegments; i++)
    {
        free(segments[i].url);
        free(segments[i].outputPath);
    }
}

// Function to create segment output path
char *createSegmentOutputPath(const char *baseOutputPath, int segmentId)
{
    if (!baseOutputPath)
    {
        fprintf(stderr, "Invalid base output path\n");
        return NULL;
    }

    char *path = malloc(strlen(baseOutputPath) + 20);
    if (!path)
    {
        fprintf(stderr, "Failed to allocate memory for segment path\n");
        return NULL;
    }

    sprintf(path, "%s.part%d", baseOutputPath, segmentId);
    return path;
}

// Function to verify a segment was downloaded correctly
int verifySegment(FileSegment *segment)
{
    if (!segment || !segment->outputPath)
    {
        fprintf(stderr, "Invalid segment for verification\n");
        return 0;
    }

    FILE *file = fopen(segment->outputPath, "rb");
    if (!file)
    {
        perror("Failed to open segment file for verification");
        return 0;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);

    // Check if the size matches what we expect
    long expectedSize = segment->end - segment->start + 1;
    if (fileSize != expectedSize)
    {
        fprintf(stderr, "Segment %d size mismatch: expected %ld bytes, got %ld bytes\n",
                segment->segmentId, expectedSize, fileSize);
        return 0;
    }

    return 1;
}



// Function to download a segment
void *downloadSegment(void *arg)
{
    FileSegment *segment = (FileSegment *)arg;

    printf("Downloading segment %d: %ld-%ld\n", segment->segmentId, segment->start, segment->end);

    // Use the downloadFileSegment function to download the segment
    if (downloadFileSegment(segment->url, segment->outputPath, segment->start, segment->end) == 0)
    {
        segment->isCompleted = 1;
        printf("Segment %d completed.\n", segment->segmentId);
    }
    else
    {
        fprintf(stderr, "Failed to download segment %d.\n", segment->segmentId);
    }

    return NULL;
}