# Dedupe++

A C++17 command-line utility for finding duplicate files in directories. Built with modern C++ and designed for future UI integration.

## Features

- Recursive directory scanning
- Duplicate file detection using file hashing
- Progress reporting API
- Cancellation support
- Modern C++17 implementation

## Building

### Prerequisites

- C++17 compatible compiler
- CMake 3.15 or higher
- Filesystem library (usually included with C++17)

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

```bash
dedupe++ [options] <directory>
```

Options:
- `--help`: Show help message
- `--recursive`: Scan directories recursively (default: true)
- `--min-size <bytes>`: Minimum file size to consider (default: 0)
- `--max-size <bytes>`: Maximum file size to consider (default: unlimited) 