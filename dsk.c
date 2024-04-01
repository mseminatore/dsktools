#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "dsk.h"

DSK_DirEntry dirs[72];
DSK_FAT fat;

FILE *fp = NULL;

//------------------------------------
//
//------------------------------------
void dir()
{
    DSK_Sector s;

    for (int track = 0; track < 35; track++)
    {
        for (int sector = 1; sector < 19; sector++)
        {
            fread(&s, sizeof(DSK_Sector), 1, fp);
            // printf("Track: %d, Sector %d (t: %02X, s: %02X)\n", track, sector, s.sector_header[12], s.sector_header[14]);
        }
    }
}

//------------------------------------
// seek to track/sector in DSK file
//------------------------------------
int dsk_seek_drive(int track, int sector)
{
    assert(fp);
    assert(track >= 0 && track < NUM_TRACKS);
    assert(sector >= 1 && sector <= SECTORS_PER_TRACK);

    long offset = DSK_OFFSET(track, sector);    // DSK_TRACK_OFFSET(track) + DSK_SECTOR_OFFSET(sector);
// printf("seeking to track %d, sector %d, offset is %lX\n", track, sector, offset);

    fseek(fp, offset, SEEK_SET);

    return E_OK;
}

//------------------------------------
// mount a DSK file
//------------------------------------
int dsk_mount_drive(const char *filename)
{
    if (fp)
        fclose(fp);

    fp = fopen(filename, "rb");
    if (!fp)
        return E_FAIL;

    // read FAT
    dsk_seek_drive(17, 2);
    fread(&fat, sizeof(DSK_FAT), 1, fp);

    // print FAT
    for (int i = 0; i < TOTAL_GRANULES; i++)
    {
        printf("%02X ", fat.granule_map[i]);
    }

    // read dirs
    dsk_seek_drive(17, 3);
    fread(&dirs, sizeof(DSK_DirEntry), 8, fp);

    for (int i = 0; i < 72; i++)
    {
       if (dirs[i].filename[0] != 0 && 0xFF != (uint8_t)dirs[i].filename[0])
            printf("%8s.%3s\n", dirs[i].filename, dirs[i].ext);
    }

    return E_OK;
}

//------------------------------------
// unmount a DSK file
//------------------------------------
int dsk_unload_drive()
{
    fclose(fp);
    fp = NULL;

    return E_OK;
}
