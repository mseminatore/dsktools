# Copilot Instructions for dsktools

## Build Commands

**CMake (cross-platform, recommended):**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

**Make (Unix):**
```bash
make
```

## Test Commands

**Run all tests:**
```bash
cd build && ctest -C Release
```

**Run a single test:**
```bash
ctest -R <test_name> -C Release
# Example: ctest -R dsk_add -C Release
```

## Architecture

This project provides a C library (`libdsk`) and CLI tools for working with TRS-80 Color Computer virtual disk (.DSK) files in JVC format.

**Core Components:**
- `dsk.h` / `dsk.c` - The libdsk library implementing FAT8-based filesystem operations
- `main.c` - Interactive CLI (`dsktools`) with a command table pattern
- `dsk_*.c` - Standalone tool wrappers (dsk_new, dsk_add, dsk_extract, etc.)

**DSK Filesystem Concepts:**
- Uses 8-bit FAT with granule-based allocation (1 granule = 9 sectors = 2304 bytes)
- Track 17 contains FAT (sector 2) and directory (sectors 3-11)
- Directory entries are 32 bytes; filenames are 8.3 format, uppercase, space-padded
- Multi-byte values (bytes_in_last_sector) stored in big-endian order

## Conventions

- All DSK filenames are converted to uppercase before operations
- File mode/type use single-letter codes: A=ASCII, B=Binary; T=Text, D=Data, M=ML (machine language)
- Library functions return `E_OK` (0) on success, `E_FAIL` (-1) on failure; mount functions return NULL on failure
- Use `dsk_flush()` after modifying FAT/directory; this is called automatically by unmount
- Debug output via `DSK_TRACE` macro (enabled with `-DDSK_DEBUG`)
- The `dsk_set_output_function()` API allows redirecting library output
