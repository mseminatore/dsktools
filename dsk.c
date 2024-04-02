#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "dsk.h"

//----------------------------------------
// count granules used by file
//----------------------------------------
static int count_granules(DSK_Drive *drv, int gran, int *tail_sectors)
{
    int count = 1;

    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    while (drv->fat.granule_map[gran] < 0xC0)
    {
        count++;
        gran = drv->fat.granule_map[gran];
    }

    if (tail_sectors)
        *tail_sectors = drv->fat.granule_map[gran] & 31;
    
    return count;
}

//----------------------------------------
// count free granules on drive
//----------------------------------------
static int free_granules(DSK_Drive *drv)
{
    int count = 0;

    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    for (int i = 0; i < TOTAL_GRANULES; i++)
    {
        if (drv->fat.granule_map[i] == DSK_GRANULE_FREE)
            count++;
    }

    return count;
}

//----------------------------------------
// calc free bytes on drive
//----------------------------------------
static int free_bytes(DSK_Drive *drv)
{
    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    int grans = free_granules(drv);
    int count = grans * BYTES_PER_GRANULE;
    return count;
}

//----------------------------------------
// walk and print the granule chain in the FAT
//----------------------------------------
static int granule_chain(DSK_Drive *drv, DSK_DirEntry *dirent)
{
    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    int gran = dirent->first_granule;

    while(drv->fat.granule_map[gran] < 0xC0)
    {
        printf("%02X->%02X ", gran, drv->fat.granule_map[gran]);
        gran = drv->fat.granule_map[gran];
    }
    
    printf("%02X->%02X\n", gran, drv->fat.granule_map[gran]);

    return E_OK;
}

//----------------------------------------
//
//----------------------------------------
static int file_size(DSK_Drive *drv, DSK_DirEntry *dirent)
{
    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    // get granule count and count of sectors in last granule
    int sectors = 0;
    int grans = count_granules(drv, dirent->first_granule, &sectors);

    // add in bytes in last sector
    int size = (grans-1) * BYTES_PER_GRANULE + (sectors-1) * BYTES_DATA_PER_SECTOR + dirent->bytes_in_last_sector;
// printf("%d grans, %d sectors, %d bytes\n", grans, sectors, dirent->bytes_in_last_sector);

    return size;
}

//----------------------------------------
// print a directory of the mounted drive
//----------------------------------------
int dsk_dir(DSK_Drive *drv)
{
    char file[DSK_MAX_FILENAME + 1], ext[DSK_MAX_EXT + 1];

    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    memset(file, 0, sizeof(file));
    memset(ext, 0, sizeof(ext));

    for (int i = 0; i < DSK_MAX_DIR_ENTRIES; i++)
    {
        DSK_DirEntry *dirent = &drv->dirs[i];

        if (dirent->filename[0] != DSK_DIRENT_DELETED && DSK_DIRENT_FREE != (uint8_t)dirent->filename[0])
        {
            strncpy(file, dirent->filename, DSK_MAX_FILENAME);
            strncpy(ext, dirent->ext, DSK_MAX_EXT);

            int grans = count_granules(drv, dirent->first_granule, NULL);
#if 1
            printf("%8s %3s\t%d %c %d\n", file, ext, dirent->type, dirent->binary_ascii == 0? 'B' : 'A', grans);
#else
            printf("%8s %3s\t%d %c %d %d\n", file, ext, dirent->type, dirent->binary_ascii == 0? 'B' : 'A', grans, file_size(drv, dirent));
            granule_chain(drv, dirent);
#endif
        }
    }

    printf("%d bytes (%d granules) free.\n", free_bytes(drv), free_granules(drv));

    return E_OK;
}

//------------------------------------
// seek to track/sector in DSK file
//------------------------------------
int dsk_seek_drive(DSK_Drive *drv, int track, int sector)
{
    assert(drv && drv->fp);
    assert(track >= 0 && track < NUM_TRACKS);
    assert(sector >= 1 && sector <= SECTORS_PER_TRACK);

    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    long offset = DSK_OFFSET(track, sector);
// printf("seeking to track %d, sector %d, offset is %lX\n", track, sector, offset);

    fseek(drv->fp, offset, SEEK_SET);

    return E_OK;
}

//------------------------------------
// print granule map for mounted drive
//------------------------------------
int dsk_granule_map(DSK_Drive *drv)
{
    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    // print FAT
    for (int i = 0; i < TOTAL_GRANULES; i++)
    {
        printf("%02X ", drv->fat.granule_map[i]);
    }

    puts("");

    return E_OK;
}

//------------------------------------
// mount a DSK file
//------------------------------------
DSK_Drive *dsk_mount_drive(const char *filename)
{
    DSK_Drive *drv;

    drv = malloc(sizeof(DSK_Drive));

    if (!drv)
        return NULL;

    drv->fp = fopen(filename, "rb");
    if (!drv->fp)
    {
        printf("Disk (%s) not found.\n", filename);
        free(drv);
        return NULL;
    }

    // read in the FAT
    dsk_seek_drive(drv, DSK_DIR_TRACK, DSK_FAT_SECTOR);
    fread(&drv->fat, sizeof(DSK_FAT), 1, drv->fp);

    // read in the directory
    dsk_seek_drive(drv, DSK_DIR_TRACK, DSK_DIRECTORY_SECTOR);
    fread(&drv->dirs, sizeof(DSK_DirEntry), DSK_MAX_DIR_ENTRIES, drv->fp);

    // fixup endianess
    for (int i = 0; i < DSK_MAX_DIR_ENTRIES; i++)
        drv->dirs[i].bytes_in_last_sector = ntohs(drv->dirs[i].bytes_in_last_sector);

    drv->drv_status = DSK_MOUNTED;

    return drv;
}

//------------------------------------
// unmount a DSK file
//------------------------------------
int dsk_unload_drive(DSK_Drive *drv)
{
    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    fclose(drv->fp);
    drv->fp = NULL;
    drv->drv_status = DSK_UNMOUNTED;
    free(drv);

    return E_OK;
}
