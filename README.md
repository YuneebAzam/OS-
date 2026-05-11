# Multithreaded Segmented File Downloader
A concurrent file downloader implemented in C that splits files into segments and downloads them using multiple threads. This project demonstrates the use of POSIX threads, mutex locks, and libcurl for efficient file downloading.

## Features
- **Concurrent Downloads**: Downloads file segments in parallel using multiple threads
- **Segment Management**: Splits large files into smaller segments for efficient downloading
- **Progress Tracking**: Shows download progress for each segment
- **Error Handling**: Implements robust error checking and reporting
- **System Integration**: Available as a Debian package for easy installation
- **Simple Command**: Use the simple `download` command to fetch files

## Installation
You can either install using the pre-built Debian package or build from source.

### Option 1: Installing from Debian Package
The easiest way to install the downloader is using the provided .deb package:

```shell
# Install the package
sudo dpkg -i downloader_1.0.0_amd64.deb

# If you encounter dependency issues
sudo apt-get install -f
```

### Option 2: Building from Source

#### Prerequisites
- GCC compiler
- POSIX Threads library
- libcurl development package
- Make build system

#### Installing Dependencies
For Ubuntu/Debian:
```shell
sudo apt-get update
sudo apt-get install build-essential libcurl4-openssl-dev
```

For Fedora/RHEL:
```shell
sudo dnf install gcc make libcurl-devel
```

#### Clone and Build
```shell
# Clone the repository
git clone https://github.com/OS-OGs/Multithreaded-downloader
cd Multithreaded-downloader

# Build the project
make clean
make
```

## Usage

### If You Installed the Debian Package
After installing the .deb package, you can use the downloader from any directory:

```shell
# Syntax
download "URL_TO_DOWNLOAD"

# Example
download "https://example.com/large-file.zip"
```



### If You Built from Source
If you built from source, you need to run the executable from the project directory:

```shell
# Syntax
./file_download_manager download "URL"

# Example
./file_download_manager download "https://example.com/file.txt"
```

Files are saved to the `Home/Downloads` directory

## Project Structure
```
.
├── include/          # Header files
│   ├── common.h      # Common definitions and structures
│   ├── downloader.h  # Downloader functions
│   ├── monitor.h     # Download monitoring
│   ├── network.h     # Network operations
│   ├── segment.h     # File segmentation
│   └── writer.h      # File writing operations
│
├── src/              # Source files
│   ├── downloader.c  # Thread management implementation
│   ├── main.c        # Main program entry point
│   ├── monitor.c     # Progress tracking implementation
│   ├── network.c     # HTTP/HTTPS download implementation
│   ├── segment.c     # Segmentation logic
│   └── writer.c      # File operations implementation
│
├── Makefile          # Build configuration
├── LICENSE           # MIT License
└── README.md         # Project documentation
```

## Implementation Details
- Uses POSIX threads for concurrent downloading
- Implements mutex locks for thread synchronization
- Uses libcurl for HTTP/HTTPS downloads
- Handles file segments with proper error checking
- Merges segments into the final file

## Synchronization Features
- **Dining Philosophers**: Thread resource management
- **Reader-Writer**: Segment file access control
- **Producer-Consumer**: Download buffer management

## Error Handling
The program includes comprehensive error handling for:
- Network connectivity issues
- Invalid URLs
- File system errors
- Memory allocation failures
- Thread creation/management issues

## Contributing
Feel free to submit issues and pull requests for:
- Bug fixes
- Feature improvements
- Documentation updates
- Code optimization

## License
This project is licensed under the MIT License - see the LICENSE file for details.
