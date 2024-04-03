#ifndef __DSK_H
#define __DSK_H

#include <stdint.h>

//------------------------------------
// DSK Drive properties
//------------------------------------
#define DSK_NUM_TRACKS              35
#define DSK_SECTORS_PER_TRACK       18
#define DSK_BYTES_DATA_PER_SECTOR   256
#define DSK_BYTES_DATA_PER_TRACK    (DSK_SECTORS_PER_TRACK * DSK_BYTES_DATA_PER_SECTOR)
#define DSK_DIR_TRACK               17
#define DSK_FAT_SECTOR              2
#define DSK_DIRECTORY_SECTOR        3
#define DSK_GRANULES_PER_TRACK      2
#define DSK_SECTORS_PER_GRANULE     (DSK_SECTORS_PER_TRACK / DSK_GRANULES_PER_TRACK)
#define DSK_BYTES_PER_GRANULE       (DSK_SECTORS_PER_GRANULE * DSK_BYTES_DATA_PER_SECTOR)
#define DSK_TOTAL_GRANULES          ((DSK_NUM_TRACKS - 1) * DSK_GRANULES_PER_TRACK)
#define DSK_MAX_FILENAME            8
#define DSK_MAX_EXT                 3
#define DSK_MAX_DIR_ENTRIES         72
#define DSK_DIRENT_FREE             0xFF
#define DSK_DIRENT_DELETED          0
#define DSK_GRANULE_FREE            0xFF
#define DSK_TOTAL_SIZE              (DSK_NUM_TRACKS * DSK_SECTORS_PER_TRACK * DSK_BYTES_DATA_PER_SECTOR)
#define DSK_PRINTF_BUF_SIZE         256
#define DSK_IS_LAST_GRANULE(g)      (!((g) < 0xC0))
#define DSK_SECTOR_COUNT_MASK       31

// error return codes
#ifndef E_OK
#   define E_OK 0
#endif

#ifndef E_FAIL
#   define E_FAIL -1
#endif

// offset in DSK to start of track t
#define DSK_TRACK_OFFSET(t) ((t) * DSK_BYTES_DATA_PER_TRACK)

// offset in track to start of sector s
#define DSK_SECTOR_OFFSET(s) (((s) - 1) * DSK_BYTES_DATA_PER_SECTOR)

// offset in DSK to start of track/sector
#define DSK_OFFSET(track, sector) (DSK_TRACK_OFFSET(track) + DSK_SECTOR_OFFSET(sector))

typedef enum {DSK_UNMOUNTED, DSK_MOUNTED} DSK_DRIVE_STATUS;

typedef struct
{
    char data[DSK_BYTES_DATA_PER_SECTOR];
} DSK_Sector;

typedef struct
{
    DSK_Sector sectors[DSK_SECTORS_PER_TRACK];
} DSK_Track;

//--------------------------------------
// individual directory entry, 32 bytes
//--------------------------------------
typedef struct
{
    char filename[DSK_MAX_FILENAME];
    char ext[DSK_MAX_EXT];
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
    uint8_t granule_map[DSK_TOTAL_GRANULES];    // 68 = 34 data tracks * 2 granules / track
    char reserved[256 - DSK_TOTAL_GRANULES];    // 256 - 68 = 188 reserved
} DSK_FAT;

//--------------------------------------
// represents a mounted disk drive
//--------------------------------------
typedef struct
{
    char filename[FILENAME_MAX];
    FILE *fp;
    DSK_FAT fat;
    DSK_DirEntry dirs[DSK_MAX_DIR_ENTRIES];
    DSK_DRIVE_STATUS drv_status; // 0 - unmounted, 1 - mounted
    int dirty_flag;
} DSK_Drive;

typedef void (*DSK_Print)(const char *s);

#ifdef DSK_DEBUG
#   define DSK_TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#   define DSK_TRACE(...)
// #   define MT_TRACE __noop
#endif

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
DSK_Drive *dsk_new(const char *filename);
int dsk_format(DSK_Drive *drv);
int dsk_flush(DSK_Drive *drv);
int dsk_del(DSK_Drive *drv, const char *filename);
void dsk_set_output_function(DSK_Print f);

#endif  // __DSK_H
