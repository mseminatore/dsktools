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
// count free granules on drive
//----------------------------------------
int dsk_free_granules(DSK_Drive *drv)
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
int dsk_free_bytes(DSK_Drive *drv)
{
    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    int grans = dsk_free_granules(drv);
    int count = grans * BYTES_PER_GRANULE;
    return count;
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

    puts("");

    for (int i = 0; i < DSK_MAX_DIR_ENTRIES; i++)
    {
        DSK_DirEntry *dirent = &drv->dirs[i];

        if (dirent->filename[0] != DSK_DIRENT_DELETED && DSK_DIRENT_FREE != (uint8_t)dirent->filename[0])
        {
            strncpy(file, dirent->filename, DSK_MAX_FILENAME);
            strncpy(ext, dirent->ext, DSK_MAX_EXT);

            int grans = count_granules(drv, dirent->first_granule, NULL);
#if 0
            printf("%8s %3s\t%d %c %d\n", file, ext, dirent->type, dirent->binary_ascii == 0? 'B' : 'A', grans);
#else
            printf("%8s %3s\t%d %c %d %d\n", file, ext, dirent->type, dirent->binary_ascii == 0? 'B' : 'A', grans, file_size(drv, dirent));
            granule_chain(drv, dirent);
#endif
        }
    }

    printf("\n%d bytes (%d granules) free.\n", dsk_free_bytes(drv), dsk_free_granules(drv));

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
printf("seeking to track %d, sector %d, offset is %lX\n", track, sector, offset);

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
    for (int i = 1; i <= TOTAL_GRANULES; i++)
    {
        printf("%02X ", drv->fat.granule_map[i-1]);
        if (0 == (i%24))
            puts("");
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

    drv->fp = fopen(filename, "r+b");
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

//------------------------------------
// add file to a mounted DSK file
//------------------------------------
int dsk_add_file(DSK_Drive *drv, const char *filename)
{
    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        puts("disk invalid");
        return E_FAIL;
    }

    return E_OK;
}

//
static char *file_ncopy(char *dst, char *src, int n)
{
    while (n--)
    {
        if (*src == ' ' || *src == 0)
            break;

        *dst++ = *src++;
    }
    
    *dst =0;

    return dst;
}

//
static DSK_DirEntry *find_file_dir(DSK_Drive *drv, const char *filename)
{
    char dirfile[DSK_MAX_FILENAME + DSK_MAX_EXT + 2];

    // find dir entry
    for (int i = 0; i < DSK_MAX_DIR_ENTRIES; i++)
    {
        DSK_DirEntry *dirent = &drv->dirs[i];

        if (dirent->filename[0] == DSK_DIRENT_DELETED || DSK_DIRENT_FREE == (uint8_t)dirent->filename[0])
            continue;

        file_ncopy(dirfile, dirent->filename, DSK_MAX_FILENAME);
        strcat(dirfile, ".");
        strncat(dirfile, dirent->ext, DSK_MAX_EXT);

        if (!strcasecmp(filename, dirfile))
            return dirent;
    }

    return NULL;
}

//------------------------------------
// unmount a DSK file
//------------------------------------
int dsk_extract_file(DSK_Drive *drv, const char *filename)
{
    char sector_data[256];

    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        puts("disk invalid.");
        return E_FAIL;
    }

    DSK_DirEntry *dirent = find_file_dir(drv, filename);
    if (!dirent)
    {
        puts("file not found.");
        return E_FAIL;
    }

    // open the output file
    FILE *fout = fopen(filename, "wb");
    if (!fout)
    {
        puts("file not found.");
        return E_FAIL;
    }

    // walk the granules
    int gran = dirent->first_granule;

    // write out full granules    
    while(gran < 0xC0)
    {
        int track = gran / GRANULES_PER_TRACK;
        int granule_half = gran % GRANULES_PER_TRACK;
        int sector = 1 + (gran % GRANULES_PER_TRACK) * SECTORS_PER_GRANULE;
printf("t: %d, s: %d\n", track, sector);
        dsk_seek_drive(drv, track, sector);

printf("extracting granule %2X\n", gran);
        for (int i = 0; i < 9; i++)
        {
            fread(sector_data, BYTES_DATA_PER_SECTOR, 1, drv->fp);
            fwrite(sector_data, BYTES_DATA_PER_SECTOR, 1, fout);
        }

        // get next granule
        gran = drv->fat.granule_map[gran];
    }

    // write out partial granule
    // write out partial sector

    fclose(fout);

    return E_OK;
}

//------------------------------------
// create a new DSK file
//------------------------------------
int dsk_new(const char *filename)
{
    return E_OK;
}

//------------------------------------
// write the FAT and DIR to the DSK
//------------------------------------
int dsk_flush(DSK_Drive *drv)
{
    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        puts("disk invalid.");
        return E_FAIL;
    }

    return E_OK;
}

//------------------------------------
// format a mounted drive
//------------------------------------
int dsk_format(DSK_Drive *drv)
{
    uint8_t val = DSK_GRANULE_FREE;

    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        puts("disk invalid.");
        return E_FAIL;
    }

    // clear FAT granule entries
    dsk_seek_drive(drv, DSK_DIR_TRACK, DSK_FAT_SECTOR);
    // TODO - clear FAT in drv and then write the FAT
    fwrite(&val, sizeof(val), TOTAL_GRANULES, drv->fp);

    // clear Directory entries
    dsk_seek_drive(drv, DSK_DIR_TRACK, DSK_DIRECTORY_SECTOR);
    // TODO - clear dir entries in drv and then write the DIR
//    fwrite(&val)

    return E_OK;
}
