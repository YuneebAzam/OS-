# Parallel Chunk-Based File Downloader

A high-performance file retrieval tool built in C that accelerates downloads by dividing files into byte-range chunks and fetching them simultaneously across multiple threads. The implementation leverages POSIX threading primitives, mutex-based synchronisation, and the libcurl networking library.

## Key Capabilities

- **Parallel Chunk Fetching** — File data is split across threads that run simultaneously, maximising throughput
- **Intelligent Thread Scaling** — Thread count adapts automatically based on file size (1 / 4 / 8 threads)
- **Real-Time Progress Tracking** — Each chunk reports its status independently during the download
- **Robust Error Recovery** — Covers network failures, bad URLs, memory errors, and filesystem issues
- **Easy Deployment** — Ships as a ready-to-install Debian `.deb` package
- **Clean CLI Interface** — Single `download` command with no configuration required

## Installation

### Option A — Debian Package (Recommended)

```shell
sudo dpkg -i downloader_1.0.0_amd64.deb

# Fix missing dependencies if needed
sudo apt-get install -f
```

### Option B — Build from Source

**Required dependencies:**
- GCC compiler
- GNU Make
- POSIX Threads (pthreads)
- libcurl development headers

**Install dependencies:**

Ubuntu / Debian:
```shell
sudo apt-get update && sudo apt-get install build-essential libcurl4-openssl-dev
```

Fedora / RHEL:
```shell
sudo dnf install gcc make libcurl-devel
```

**Clone and compile:**
```shell
git clone https://github.com/YuneebAzam/OS-
cd OS-
make clean && make
```

## Usage

**Installed via package:**
```shell
download "https://your-url-here.com/file.zip"
```

**Built from source:**
```shell
./file_download_manager download "https://your-url-here.com/file.zip"
```

Downloaded files are saved to `~/Downloads/` automatically.

## Project Layout

```
.
├── include/
│   ├── common.h        # Shared types: FileSegment, DownloadProgress
│   ├── downloader.h    # Thread orchestration interface
│   ├── monitor.h       # Download state management
│   ├── network.h       # HTTP/HTTPS request layer
│   ├── segment.h       # Byte-range partitioning logic
│   └── writer.h        # Disk write and segment merge operations
│
├── src/
│   ├── main.c          # Entry point — argument parsing, lifecycle
│   ├── downloader.c    # Thread pool creation, join, and merge dispatch
│   ├── monitor.c       # Mutex-protected progress and pause/resume state
│   ├── network.c       # libcurl range requests and file size probing
│   ├── segment.c       # Chunk calculation, verification, and download
│   └── writer.c        # Segment file assembly into final output
│
├── Makefile
├── LICENSE
└── downloader_1.0.0_amd64.deb
```

## How It Works

1. The file size is probed via an HTTP HEAD request using libcurl
2. The total size is divided into N equal byte ranges (N = thread count)
3. Each thread opens its own curl handle and requests its assigned range using `CURLOPT_RANGE`
4. Completed chunks are written to `.partN` temporary files
5. Once all threads finish, the parts are merged sequentially into the final file
6. Temporary part files are deleted after a successful merge

## Concurrency Model

| Pattern | Where Applied |
|---|---|
| Thread-per-chunk | Each byte range runs in its own `pthread_t` |
| Mutex guards | Monitor state (progress, pause flag) protected by `pthread_mutex_t` |
| Producer-Consumer | curl write callbacks feed data into buffered file writes |
| Reader-Writer | Segment files written once, read only after the thread signals completion |

## Error Handling Coverage

- Network timeout or unreachable host
- Server returns no `Content-Length` header
- Memory allocation failure at any stage
- Thread creation failure (falls back gracefully)
- Segment size mismatch detected during verification
- Output directory creation failure

## License

Released under the MIT License. See `LICENSE` for details.
