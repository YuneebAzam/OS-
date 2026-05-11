#ifndef WRITER_H
#define WRITER_H

#include "common.h"

// Function to write a file segment to disk
int writeSegmentToFile(const FileSegment *segment, const char *buffer, size_t bufferSize);

/**
 * Merges downloaded segments into the final file
 * @param outputPath Path to the final output file
 * @param segments Array of FileSegment structures
 * @param numSegments Number of segments to merge
 * @return 0 on success, -1 on failure
 */
int mergeSegments(const char *outputPath, FileSegment *segments, int numSegments);

int ensureDownloadDirectory(void);

#endif // WRITER_H