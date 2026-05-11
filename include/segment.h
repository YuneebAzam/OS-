#ifndef SEGMENT_H
#define SEGMENT_H

#include "common.h"
#include "network.h"  // Include network.h for getFileSize() declaration

// Function to calculate segments for download
void calculateSegments(long fileSize, int numThreads, FileSegment* segments, const char* url, const char* outputPath);

// Function to check if all segments are completed
int allSegmentsCompleted(FileSegment* segments, int numSegments);

// Function to free segment resources
void freeSegments(FileSegment* segments, int numSegments);

// Function to create segment output path
char* createSegmentOutputPath(const char* baseOutputPath, int segmentId);

// Function to verify a segment was downloaded correctly
int verifySegment(FileSegment* segment);

// Function to download a segment
void *downloadSegment(void *arg);

#endif // SEGMENT_H