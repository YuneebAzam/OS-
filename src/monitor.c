#include "monitor.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void initMonitor(Monitor *monitor)
{
    if (!monitor)
        return;
        
    monitor->downloadCount = 0;
    pthread_mutex_init(&monitor->mutex, NULL);
}

void destroyMonitor(Monitor *monitor)
{
    if (!monitor)
        return;
        
    pthread_mutex_destroy(&monitor->mutex);
}

void addDownload(Monitor *monitor, const char *fileName)
{
    if (!monitor || !fileName)
        return;
        
    pthread_mutex_lock(&monitor->mutex);
    if (monitor->downloadCount < 100)
    {
        strncpy(monitor->downloads[monitor->downloadCount].fileName, fileName, 255);
        monitor->downloads[monitor->downloadCount].fileName[255] = '\0';
        monitor->downloads[monitor->downloadCount].progress = 0;
        monitor->downloads[monitor->downloadCount].isPaused = 0;
        monitor->downloads[monitor->downloadCount].retries = 0;
        monitor->downloadCount++;
    }
    pthread_mutex_unlock(&monitor->mutex);
}

void updateProgress(Monitor *monitor, const char *fileName, int progress)
{
    if (!monitor || !fileName)
        return;
        
    pthread_mutex_lock(&monitor->mutex);
    for (int i = 0; i < monitor->downloadCount; i++)
    {
        if (strcmp(monitor->downloads[i].fileName, fileName) == 0)
        {
            monitor->downloads[i].progress = progress;
            break;
        }
    }
    pthread_mutex_unlock(&monitor->mutex);
}

void pauseDownload(Monitor *monitor, const char *fileName)
{
    if (!monitor || !fileName)
        return;
        
    pthread_mutex_lock(&monitor->mutex);
    for (int i = 0; i < monitor->downloadCount; i++)
    {
        if (strcmp(monitor->downloads[i].fileName, fileName) == 0)
        {
            monitor->downloads[i].isPaused = 1;
            break;
        }
    }
    pthread_mutex_unlock(&monitor->mutex);
}

void resumeDownload(Monitor *monitor, const char *fileName)
{
    if (!monitor || !fileName)
        return;
        
    pthread_mutex_lock(&monitor->mutex);
    for (int i = 0; i < monitor->downloadCount; i++)
    {
        if (strcmp(monitor->downloads[i].fileName, fileName) == 0)
        {
            monitor->downloads[i].isPaused = 0;
            break;
        }
    }
    pthread_mutex_unlock(&monitor->mutex);
}

void removeDownload(Monitor *monitor, const char *fileName)
{
    if (!monitor || !fileName)
        return;
        
    pthread_mutex_lock(&monitor->mutex);
    for (int i = 0; i < monitor->downloadCount; i++)
    {
        if (strcmp(monitor->downloads[i].fileName, fileName) == 0)
        {
            for (int j = i; j < monitor->downloadCount - 1; j++)
            {
                monitor->downloads[j] = monitor->downloads[j + 1];
            }
            monitor->downloadCount--;
            break;
        }
    }
    pthread_mutex_unlock(&monitor->mutex);
}

void displayStatus(const Monitor *monitor)
{
    if (!monitor)
        return;
        
    // Create a local non-const copy of the mutex pointer for locking
    pthread_mutex_t *mutex_ptr = (pthread_mutex_t *)&monitor->mutex;
    
    pthread_mutex_lock(mutex_ptr);
    printf("Download Status:\n");
    for (int i = 0; i < monitor->downloadCount; i++)
    {
        printf("File: %s, Progress: %d%%, Paused: %s, Retries: %d\n",
               monitor->downloads[i].fileName,
               monitor->downloads[i].progress,
               monitor->downloads[i].isPaused ? "Yes" : "No",
               monitor->downloads[i].retries);
    }
    pthread_mutex_unlock(mutex_ptr);
}