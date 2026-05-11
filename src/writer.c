#include "writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define BUFFER_SIZE 8192

// Function to ensure the downloads directory exists
int ensureDownloadDirectory(void)
{
    const char *dir = "downloads";
    struct stat st = {0};

    // Check if directory exists
    if (stat(dir, &st) == -1)
    {
        // Create directory with permissions 0755
        if (mkdir(dir, 0755) == -1)
        {
            fprintf(stderr, "Error: Could not create downloads directory: %s\n", strerror(errno));
            return -1;
        }
        printf("Created downloads directory\n");
    }
    return 0;
}

// Function to write a file segment to disk
int writeSegmentToFile(const FileSegment *segment, const char *buffer, size_t bufferSize)
{
    FILE *file = fopen(segment->outputPath, "wb");
    if (!file)
    {
        perror("Failed to open segment file for writing");
        return -1;
    }

    size_t written = fwrite(buffer, 1, bufferSize, file);
    fclose(file);

    if (written != bufferSize)
    {
        fprintf(stderr, "Failed to write all data to segment file\n");
        return -1;
    }

    return 0;
}

// Function to merge all file segments into a single file
int mergeSegments(const char *outputPath, FileSegment *segments, int numSegments)
{
    FILE *outFile = fopen(outputPath, "wb");
    if (!outFile)
    {
        perror("Failed to create output file");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    int success = 1;

    for (int i = 0; i < numSegments && success; i++)
    {
        FILE *segFile = fopen(segments[i].outputPath, "rb");
        if (!segFile)
        {
            perror("Failed to open segment file");
            success = 0;
            break;
        }

        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, segFile)) > 0)
        {
            if (fwrite(buffer, 1, bytesRead, outFile) != bytesRead)
            {
                perror("Failed to write to output file");
                success = 0;
                break;
            }
        }

        fclose(segFile);
        remove(segments[i].outputPath); // Delete segment file after merging
    }

    fclose(outFile);
    return success ? 0 : -1;
}