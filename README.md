# Lightweight dynamic memory manager

LwMEM is a lightweight dynamic memory manager optimized for embedded systems.

[Open documentation](https://docs.majerle.eu/projects/lwmem/)

## Features

* Written in C (C11), compatible with `stdint.h` data types
* Implements standard C library functions for memory allocation, malloc, calloc, realloc and free
* Uses *first-fit* algorithm to search for free block
* Supports multiple allocation instances to split between memories and/or CPU cores
* Supports different memory regions, ideal for use with fragmented or embedded memories
* Highly configurable for memory allocation and reallocation
* Supports automotive applications
* Supports advanced free/realloc algorithms to optimize memory usage
* Supports light implementation with allocation only
* Optional runtime allocation statistics: used/free/minimum-ever-free bytes and alloc/free counts
* Safe free and realloc variants that null the caller's pointer after the operation
* Optional memory wipe on free/realloc for security-sensitive applications
* Query the usable size of an allocated block at runtime
* Operating system ready, thread-safe API, with ready-made ports for CMSIS-OS, pthreads, ThreadX and Win32
* C++ wrapper functions
* User friendly MIT license

## Contribute

Fresh contributions are always welcome. Simple instructions to proceed:

1. Fork Github repository
2. Follow [C style & coding rules](https://github.com/MaJerle/c-code-style) and use `clang-format` to format the code
3. Create a pull request to `develop` branch with new features or bug fixes

Alternatively you may:

1. Report a bug
2. Ask for a feature request
