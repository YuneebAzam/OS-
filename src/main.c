#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "downloader.h"
#include "monitor.h"



int main(int argc, char *argv[])
{
    if (argc != 3 || strcmp(argv[1], "download") != 0)
    {
        printf("Usage: %s download \"link_to_download\"\n", argv[0]);
        return 1;
    }

    const char *url = argv[2];
    Monitor monitor;
    initMonitor(&monitor);

    startDownload(url, &monitor);

    destroyMonitor(&monitor);
    return 0;
}
