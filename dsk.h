#ifndef __DSK_H
#define __DSK_H

#include <stdint.h>

//------------------------------------
// DSK Drive properties
//------------------------------------
#define NUM_TRACKS              35
#define SECTORS_PER_TRACK       18
#define BYTES_DATA_PER_SECTOR   256
#define BYTES_DATA_PER_TRACK    (SECTORS_PER_TRACK * BYTES_DATA_PER_SECTOR)
#define DSK_DIR_TRACK           17
#define DSK_FAT_SECTOR          2
#define DSK_DIRECTORY_SECTOR    3
#define GRANULES_PER_TRACK      2
#define SECTORS_PER_GRANULE     (SECTORS_PER_TRACK / GRANULES_PER_TRACK)
#define BYTES_PER_GRANULE       (SECTORS_PER_GRANULE * BYTES_DATA_PER_SECTOR)
#define TOTAL_GRANULES          ((NUM_TRACKS - 1) * GRANULES_PER_TRACK)
#define DSK_MAX_FILENAME        8
#define DSK_MAX_EXT             3
#define DSK_MAX_DIR_ENTRIES     72
#define DSK_DIRENT_FREE         0xFF
#define DSK_DIRENT_DELETED      0
#define DSK_GRANULE_FREE        0xFF

// error return codes
#ifndef E_OK
#   define E_OK 0
#endif

#ifndef E_FAIL
#   define E_FAIL -1
#endif

// offset in DSK to start of track t
#define DSK_TRACK_OFFSET(t) ((t) * BYTES_DATA_PER_TRACK)

// offset in track to start of sector s
#define DSK_SECTOR_OFFSET(s) (((s) - 1) * BYTES_DATA_PER_SECTOR)

// offset in DSK to start of track/sector
#define DSK_OFFSET(track, sector) (DSK_TRACK_OFFSET(track) + DSK_SECTOR_OFFSET(sector))

typedef enum {DSK_UNMOUNTED, DSK_MOUNTED} DSK_DRIVE_STATUS;

typedef struct
{
    char data[BYTES_DATA_PER_SECTOR];
} DSK_Sector;

typedef struct
{
    DSK_Sector sectors[SECTORS_PER_TRACK];
} DSK_Track;

//--------------------------------------
// individual directory entry, 32 bytes
//--------------------------------------
typedef struct
{
    char filename[8];
    char ext[3];
    uint8_t type;
    uint8_t binary_ascii;
    uint8_t first_granule;
    uint16_t bytes_in_last_sector;  // NB: in big endian order?
    char reserved[16];
} DSK_DirEntry;

//--------------------------------------
// FAT - file allocation table
//--------------------------------------
typedef struct
{
    uint8_t granule_map[TOTAL_GRANULES];       // 68 = 34 data tracks * 2 granules / track
    char reserved[256 - TOTAL_GRANULES];    // 256 - 68 = 188 reserved
} DSK_FAT;

//--------------------------------------
// represents a mounted disk drive
//--------------------------------------
typedef struct
{
    FILE *fp;
    DSK_FAT fat;
    DSK_DirEntry dirs[DSK_MAX_DIR_ENTRIES];
    DSK_DRIVE_STATUS drv_status; // 0 - unmounted, 1 - mounted
} DSK_Drive;

//--------------------------------------
// library functions
//--------------------------------------
int dsk_seek_drive(DSK_Drive *drv, int track, int sector);
DSK_Drive *dsk_mount_drive(const char *filename);
int dsk_unload_drive(DSK_Drive *drv);
int dsk_dir(DSK_Drive *drv);
int dsk_granule_map(DSK_Drive *drv);
int dsk_free_bytes(DSK_Drive *drv);
int dsk_free_granules(DSK_Drive *drv);
int dsk_add_file(DSK_Drive *drv, const char *filename);
int dsk_extract_file(DSK_Drive *drv, const char *filename);
int dsk_new(const char *filename);
int dsk_format(DSK_Drive *drv);

#endif  // __DSK_H
