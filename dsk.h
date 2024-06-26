#ifndef __DSK_H
#define __DSK_H

#include <stdint.h>

#ifdef __GNUC__
#   include <arpa/inet.h>
#endif

#define DSK_VERSION_STRING "0.4.0"

//------------------------------------
// DSK Drive properties
//------------------------------------
#define DSK_NUM_TRACKS              (drv->num_tracks)  // 35
#define DSK_MIN_TRACKS              18  // enough room for DIR/FAT track
#define DSK_MAX_TRACKS              80
#define DSK_MAX_SIDES               2
#define DSK_SECTORS_PER_TRACK       18
#define DSK_BYTES_DATA_PER_SECTOR   256
#define DSK_BYTES_DATA_PER_TRACK    (DSK_SECTORS_PER_TRACK * DSK_BYTES_DATA_PER_SECTOR)
#define DSK_DIR_TRACK               17
#define DSK_FAT_SECTOR              2
#define DSK_DIRECTORY_SECTOR        3
#define DSK_GRANULES_PER_TRACK      2
#define DSK_DIR_START_GRANULE       (DSK_DIR_TRACK * DSK_GRANULES_PER_TRACK)
#define DSK_SECTORS_PER_GRANULE     (DSK_SECTORS_PER_TRACK / DSK_GRANULES_PER_TRACK)
#define DSK_BYTES_PER_GRANULE       (DSK_SECTORS_PER_GRANULE * DSK_BYTES_DATA_PER_SECTOR)
#define DSK_TOTAL_GRANULES          ((drv->num_tracks - 1) * DSK_GRANULES_PER_TRACK)
#define DSK_MAX_FILENAME            8
#define DSK_MAX_EXT                 3
#define DSK_MAX_DIR_ENTRIES         72
#define DSK_DIRENT_FREE             0xFF
#define DSK_DIRENT_DELETED          0
#define DSK_GRANULE_FREE            0xFF
#define DSK_TOTAL_SIZE              (DSK_NUM_TRACKS * DSK_SECTORS_PER_TRACK * DSK_BYTES_DATA_PER_SECTOR)
#define DSK_PRINTF_BUF_SIZE         256
#define DSK_IS_LAST_GRANULE(g)      (((g) >= 0xC0) && ((g) <= 0xC9))
#define DSK_SECTOR_COUNT_MASK       31
#define DSK_LAST_GRANULE            0x43
#define DSK_ENCODING_ASCII          0xFF
#define DSK_ENCODING_BINARY         0

// error return codes
#ifndef E_OK
#   define E_OK 0
#endif

#ifndef E_FAIL
#   define E_FAIL -1
#endif

#ifndef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#endif

#ifdef _WIN32
#   define BYTE_SWAP(data) (((data) >> 8) & 0x00FF) | (((data) << 8) & 0xFF00)
#ifndef ntohs
#   define ntohs(s) BYTE_SWAP(s)
#endif
#ifndef htons
#   define htons(s) BYTE_SWAP(s)
#endif
#   define strcasecmp _stricmp
#endif

// offset in DSK to start of track t
#define DSK_TRACK_OFFSET(t) ((t) * DSK_BYTES_DATA_PER_TRACK)

// offset in track to start of sector s
#define DSK_SECTOR_OFFSET(s) (((s) - 1) * DSK_BYTES_DATA_PER_SECTOR)

// offset in DSK to start of track/sector
#define DSK_OFFSET(track, sector) (DSK_TRACK_OFFSET(track) + DSK_SECTOR_OFFSET(sector))

// drive status enum
typedef enum
{
    DSK_UNMOUNTED, 
    DSK_MOUNTED
} DSK_DRIVE_STATUS;

// file modes
typedef enum
{
    DSK_MODE_BINARY, 
    DSK_MODE_ASCII
} DSK_OPEN_MODE;

// DSK file types
typedef enum
{
    DSK_TYPE_BASIC, 
    DSK_TYPE_DATA, 
    DSK_TYPE_ML, 
    DSK_TYPE_TEXT
} DSK_FILE_TYPE;

// represents a DSK sector
typedef struct
{
    char data[DSK_BYTES_DATA_PER_SECTOR];
} DSK_Sector;

// represents a DSK track
typedef struct
{
    DSK_Sector sectors[DSK_SECTORS_PER_TRACK];
} DSK_Track;

//--------------------------------------
// individual directory entry, 32 bytes
// 16 bytes reserved
//--------------------------------------
typedef struct
{
    char filename[DSK_MAX_FILENAME];
    char ext[DSK_MAX_EXT];
    uint8_t type;
    uint8_t binary_ascii;
    uint8_t first_granule;
    uint16_t bytes_in_last_sector;  // NB: in big endian order!
    char reserved[16];
} DSK_DirEntry;

//--------------------------------------
// FAT - file allocation table
//--------------------------------------
typedef struct
{
    // 68 = 34 data tracks * 2 granules / track
    // give the FAT the entire sector
    uint8_t granule_map[DSK_BYTES_DATA_PER_SECTOR];
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
    DSK_DRIVE_STATUS drv_status;    // 0 - unmounted, 1 - mounted
    int dirty_flag;                 // true if FAT/DIR have changed since last flush/write

    int num_tracks;
    int num_sides;
} DSK_Drive;

//--------------------------------------
// represents a JVC header
//--------------------------------------
typedef struct
{
    uint8_t sectors_per_track;
    uint8_t side_count;
    uint8_t sector_size_code;
    uint8_t first_sector_id;
    uint8_t sector_attribute_flag;
} DSK_HEADER;

typedef void (*DSK_Print)(const char *s);

#ifdef DSK_DEBUG
#   define DSK_TRACE(...) fprintf(stderr, __VA_ARGS__)
#else
#   define DSK_TRACE(...)
#endif

//--------------------------------------
// library functions
//--------------------------------------
int dsk_seek_drive(DSK_Drive *drv, int track, int sector);
DSK_Drive *dsk_mount_drive(const char *filename);
int dsk_unmount_drive(DSK_Drive *drv);
int dsk_dir(DSK_Drive *drv);
int dsk_granule_map(DSK_Drive *drv);
int dsk_free_bytes(DSK_Drive *drv);
int dsk_free_granules(DSK_Drive *drv);
int dsk_add_file(DSK_Drive *drv, const char *filename, DSK_OPEN_MODE mode, DSK_FILE_TYPE type);
int dsk_extract_file(DSK_Drive *drv, const char *filename);
DSK_Drive *dsk_new(char *filename, int tracks, int sides);
int dsk_format(DSK_Drive *drv);
int dsk_flush(DSK_Drive *drv);
int dsk_del(DSK_Drive *drv, const char *filename);
void dsk_set_output_function(DSK_Print f);
int dsk_rename(DSK_Drive *drv, char *file1, char *file2);

// future API ideas
// int dsk_open();
// int dsk_write();
// int dsk_read();
// int dsk_close();

#endif  // __DSK_H
