#include <stdio.h>
#include <stdint.h>
#include <assert.h>

//
// Drive properties
//
#define NUM_TRACKS 35
#define NUM_SECTORS 18
#define BYTES_PER_TRACK 6250
#define BYTES_PER_SECTOR 338
#define DIR_TRACK 17

// error returns
#define E_OK 0
#define E_FAIL -1

// offset in DSK to start of track t
#define TRACK_OFFSET(t) ((t) * BYTES_PER_TRACK)

// offset in track to start of sector s
#define SECTOR_OFFSET(s) (((s) - 1) * BYTES_PER_SECTOR)

// offset in DSK to start of track/sector
#define OFFSET(track, sector)

typedef struct
{
    char sector_header[56];
    char data[256];
    char sector_tail[26];
} Sector;

typedef struct
{
    Sector sectors[18];
} Track;

//------------------------------------
// individual directory entry, 32 bytes
//------------------------------------
typedef struct
{
    char filename[8];
    char ext[3];
    uint8_t type;
    uint8_t binary_ascii;
    uint8_t first_granule;
    uint16_t bytes_in_last_sector;
    char reserved[16];
} DirEntry;

typedef struct
{
    char granule_map[68];   // 34 data tracks * 2 granules / track
    char reserved[188];
} FAT;

DirEntry dirs[72];
FAT fat;

FILE *fp = NULL;

int sector_map[] = {1, 6, 11, 16, 3, 8, 13, 18, 5, 10, 15, 2, 7, 12, 17, 4, 9, 14};

//------------------------------------
//
//------------------------------------
void dir()
{
    Sector s;

    for (int track = 0; track < 35; track++)
    {
        for (int sector = 1; sector < 19; sector++)
        {
            fread(&s, sizeof(Sector), 1, fp);
            printf("Track: %d, Sector %d (t: %02X, s: %02X)\n", track, sector, s.sector_header[12], s.sector_header[14]);
        }
    }
}

//------------------------------------
// seek to track/sector in DSK file
//------------------------------------
int seek_drive(int track, int sector)
{
    assert(fp);
    assert(track >= 0 && track < NUM_TRACKS);
    assert(sector >= 1 && sector <= NUM_SECTORS);

    long offset = TRACK_OFFSET(track) + SECTOR_OFFSET(sector_map[sector - 1]);
printf("seeking to track %d, sector %d, offset is %lX\n", track, sector, offset);

    fseek(fp, offset, SEEK_SET);

    return E_OK;
}

//------------------------------------
// mount a DSK file
//------------------------------------
int mount_drive(const char *filename)
{
    if (fp)
        fclose(fp);

    fp = fopen(filename, "rb");
    if (!fp)
        return E_FAIL;

    // read FAT
    seek_drive(17, 2);
    fseek(fp, 56, SEEK_CUR);

    fread(&fat, sizeof(FAT), 1, fp);
    for (int i = 0; i < 68; i++)
    {
        printf("%02X ", fat.granule_map[i]);
    }

    // read dirs
    seek_drive(17, 3);
    fseek(fp, 56, SEEK_CUR);

    fread(&dirs, sizeof(DirEntry), 8, fp);

    for (int i = 0; i < 72; i++)
    {
//        if (dirs[i].filename[0] != 0 && dirs[i].filename[0] != 0xFF)
            printf("%8s.%3s\n", dirs[i].filename, dirs[i].ext);
    }

    return E_OK;
}

//------------------------------------
// unmount a DSK file
//------------------------------------
int unload_drive()
{
    fclose(fp);
    fp = NULL;

    return E_OK;
}

//
int main(int argc, char *argv[])
{
    assert(sizeof(DirEntry) == 32);

    mount_drive(argv[1]);

    // dir();

    unload_drive();
}
