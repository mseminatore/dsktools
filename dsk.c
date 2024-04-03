#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "dsk.h"

static void dsk_default_output(const char *s);
DSK_Print dsk_puts = dsk_default_output;

//----------------------------------------
// default output function
//----------------------------------------
static void dsk_default_output(const char *s)
{
	fputs(s, stdout);
}

//----------------------------------------
// override the default output function
//----------------------------------------
void dsk_set_output_function(DSK_Print f)
{
    dsk_puts = f;
}

//----------------------------------------
// DSK formatted output function
//----------------------------------------
void dsk_printf(char *format, ...)
{
	char buf[DSK_PRINTF_BUF_SIZE];
	va_list valist;

	va_start(valist, format);
	vsprintf(buf, format, valist);
	va_end(valist);

	dsk_puts(buf);
}

//----------------------------------------
// count granules used by file
//----------------------------------------
static int count_granules(DSK_Drive *drv, int gran, int *tail_sectors)
{
    int count = 0;

    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid\n");
        return E_FAIL;
    }

    while (!DSK_IS_LAST_GRANULE(gran))
    {
        count++;
        gran = drv->fat.granule_map[gran];
    }

    // return number of sectors in last granule
    if (tail_sectors)
        *tail_sectors = gran & DSK_SECTOR_COUNT_MASK;
    
    return count;
}

//---------------------------------------------
// walk and print the granule chain in the FAT
//---------------------------------------------
static int granule_chain(DSK_Drive *drv, DSK_DirEntry *dirent)
{
    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid\n");
        return E_FAIL;
    }

    int gran = dirent->first_granule;

    while (!DSK_IS_LAST_GRANULE(gran))
    {
        dsk_printf("%02X->", gran);
        gran = drv->fat.granule_map[gran];
    }
    
    dsk_printf("%02X\n", gran);

    return E_OK;
}

//----------------------------------------
// return size of file
//----------------------------------------
static int file_size(DSK_Drive *drv, DSK_DirEntry *dirent)
{
    assert(drv && drv->fp);

    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid\n");
        return E_FAIL;
    }

    // get granule count and count of sectors in last granule
    int sectors = 0;
    int grans = count_granules(drv, dirent->first_granule, &sectors);

    // add in bytes in last sector
    int size = (grans-1) * DSK_BYTES_PER_GRANULE + (sectors-1) * DSK_BYTES_DATA_PER_SECTOR + dirent->bytes_in_last_sector;
printf("%d grans, %d sectors, %d bytes\n", grans, sectors, dirent->bytes_in_last_sector);

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
        dsk_printf("disk invalid\n");
        return E_FAIL;
    }

    for (int i = 0; i < DSK_TOTAL_GRANULES; i++)
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
        dsk_printf("disk invalid\n");
        return E_FAIL;
    }

    int grans = dsk_free_granules(drv);
    int count = grans * DSK_BYTES_PER_GRANULE;
    return count;
}

//----------------------------------------
// print a directory of the mounted drive
//----------------------------------------
int dsk_dir(DSK_Drive *drv)
{
    char file[DSK_MAX_FILENAME + 1], ext[DSK_MAX_EXT + 1];

    if (!drv || !drv->fp)
    {
        dsk_printf("no disk mounted.\n");
        return E_FAIL;
    }

    memset(file, 0, sizeof(file));
    memset(ext, 0, sizeof(ext));

    dsk_printf("Directory of '%s'\n\n", drv->filename);

    for (int i = 0; i < DSK_MAX_DIR_ENTRIES; i++)
    {
        DSK_DirEntry *dirent = &drv->dirs[i];

        if (dirent->filename[0] != DSK_DIRENT_DELETED && DSK_DIRENT_FREE != (uint8_t)dirent->filename[0])
        {
            strncpy(file, dirent->filename, DSK_MAX_FILENAME);
            strncpy(ext, dirent->ext, DSK_MAX_EXT);

            int grans = count_granules(drv, dirent->first_granule, NULL);
#if 1
            dsk_printf("%8s %3s\t%d %c %d\n", file, ext, dirent->type, dirent->binary_ascii == 0? 'B' : 'A', grans);
#else
            dsk_printf("%8s %3s\t%d %c %d (%d bytes)\n", file, ext, dirent->type, dirent->binary_ascii == 0? 'B' : 'A', grans, file_size(drv, dirent));
            granule_chain(drv, dirent);
#endif
        }
    }

    dsk_printf("\n%d bytes (%d granules) free.\n", dsk_free_bytes(drv), dsk_free_granules(drv));

    return E_OK;
}

//------------------------------------
// seek to track/sector in DSK file
//------------------------------------
int dsk_seek_drive(DSK_Drive *drv, int track, int sector)
{
    assert(drv && drv->fp);
    assert(track >= 0 && track < DSK_NUM_TRACKS);
    assert(sector >= 1 && sector <= DSK_SECTORS_PER_TRACK);

    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid\n");
        return E_FAIL;
    }

    long offset = DSK_OFFSET(track, sector);
    
    DSK_TRACE("seeking to track %d, sector %d, offset is %lX\n", track, sector, offset);

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
        dsk_printf("disk invalid\n");
        return E_FAIL;
    }

    // print FAT
    for (int i = 1; i <= DSK_TOTAL_GRANULES; i++)
    {
        dsk_printf("%02X ", drv->fat.granule_map[i-1]);
        if (0 == (i%24))
            dsk_printf("\n");
    }

    dsk_printf("");

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

    memset(drv, 0, sizeof(DSK_Drive));

    drv->fp = fopen(filename, "r+b");
    if (!drv->fp)
    {
        dsk_printf("Disk (%s) not found.\n", filename);
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
    // TODO - maybe we don't fix it here, instead fix it on each use
    for (int i = 0; i < DSK_MAX_DIR_ENTRIES; i++)
        drv->dirs[i].bytes_in_last_sector = ntohs(drv->dirs[i].bytes_in_last_sector);

    strcpy(drv->filename, filename);
    
    drv->drv_status = DSK_MOUNTED;

    return drv;
}

//------------------------------------
// unmount a DSK file
//------------------------------------
int dsk_unload_drive(DSK_Drive *drv)
{
    // assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        dsk_printf("no disk mounted.\n");
        return E_FAIL;
    }

    // ensure any changes are written!
    dsk_flush(drv);

    // fflush(drv->fp);
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
        dsk_printf("disk invalid\n");
        return E_FAIL;
    }

    drv->dirty_flag = 1;

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
        dsk_printf("disk invalid.\n");
        return E_FAIL;
    }

    DSK_DirEntry *dirent = find_file_dir(drv, filename);
    if (!dirent)
    {
        dsk_printf("file not found.\n");
        return E_FAIL;
    }

    // open the output file
    FILE *fout = fopen(filename, "wb");
    if (!fout)
    {
        dsk_printf("file not found.\n");
        return E_FAIL;
    }

    // walk the granules
    int gran = dirent->first_granule;

    // write out full granules    
    while(!DSK_IS_LAST_GRANULE(gran))
    {
        int track = gran / DSK_GRANULES_PER_TRACK;
        int sector = 1 + (gran % DSK_GRANULES_PER_TRACK) * DSK_SECTORS_PER_GRANULE;
dsk_printf("t: %d, s: %d\n", track, sector);
        dsk_seek_drive(drv, track, sector);

dsk_printf("extracting granule %2X\n", gran);
        for (int i = 0; i < DSK_SECTORS_PER_GRANULE; i++)
        {
            fread(sector_data, DSK_BYTES_DATA_PER_SECTOR, 1, drv->fp);
            fwrite(sector_data, DSK_BYTES_DATA_PER_SECTOR, 1, fout);
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
// delete file from mounted DSK
//------------------------------------
int dsk_del(DSK_Drive *drv, const char *filename)
{
    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid.\n");
        return E_FAIL;
    }

    DSK_DirEntry *dirent = find_file_dir(drv, filename);
    if (!dirent)
    {
        dsk_printf("file not found.\n");
        return E_FAIL;
    }

    // mark all file granules as free
    int gran = dirent->first_granule;
    while (!DSK_IS_LAST_GRANULE(gran))
    {
        int next_gran = drv->fat.granule_map[gran];
        DSK_TRACE("marking granule %2X as free.\n", gran);
        drv->fat.granule_map[gran] = DSK_GRANULE_FREE;
        gran = next_gran;
    }

    // mark the dir entry as freed
    dirent->filename[0] = DSK_DIRENT_DELETED;

    drv->dirty_flag = 1;

    dsk_flush(drv);

    return E_OK;
}

//------------------------------------
// create a new DSK file
//------------------------------------
DSK_Drive *dsk_new(const char *filename)
{
    char sector_data[DSK_BYTES_DATA_PER_SECTOR];

    assert(filename);

    FILE *fout = fopen(filename, "wb");
    if (!fout)
    {
        dsk_printf("file not found.\n");
        return NULL;
    }

    // write out empty DSK
    memset(sector_data, 0, DSK_BYTES_DATA_PER_SECTOR);

    for (int track = 0; track < DSK_NUM_TRACKS; track++)
    {
        for (int sector = 0; sector < DSK_SECTORS_PER_TRACK; sector++)
            fwrite(sector_data, sizeof(sector_data), 1, fout);
    }

    fclose(fout);

    // mount it
    DSK_Drive *drv = dsk_mount_drive(filename);
    if (!drv)
    {
        dsk_printf("disk not found.\n");
        return NULL;
    }

    // format it
    dsk_format(drv);

    return drv;
}

//------------------------------------
// write the FAT and DIR to the DSK
//------------------------------------
int dsk_flush(DSK_Drive *drv)
{
    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid.\n");
        return E_FAIL;
    }

    // don't flush if nothing has changed
    if (!drv->dirty_flag)
    {
        DSK_TRACE("flush called with no changes.\n");
        return E_OK;
    } else
    {
        DSK_TRACE("flushing dirty file.\n");
    }

    // write out the FAT
    dsk_seek_drive(drv, DSK_DIR_TRACK, DSK_FAT_SECTOR);
    fwrite(drv->fat.granule_map, 1, DSK_TOTAL_GRANULES, drv->fp);

    // write out the Directory
    dsk_seek_drive(drv, DSK_DIR_TRACK, DSK_DIRECTORY_SECTOR);
    fwrite(drv->dirs, sizeof(DSK_DirEntry), DSK_MAX_DIR_ENTRIES, drv->fp);

    // fflush(drv->fp);

    drv->dirty_flag = 0;

    return E_OK;
}

//------------------------------------
// format a mounted drive
//------------------------------------
int dsk_format(DSK_Drive *drv)
{
    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid.\n");
        return E_FAIL;
    }

    // clear FAT granule entries
    for (int i = 0; i < DSK_TOTAL_GRANULES; i++)
        drv->fat.granule_map[i] = DSK_GRANULE_FREE;

    // clear Directory entries
    for (int i = 0; i < DSK_MAX_DIR_ENTRIES; i++)
        drv->dirs[i].filename[0] = DSK_DIRENT_FREE;

    drv->dirty_flag = 1;

    // flush changes to DSK file
    dsk_flush(drv);

    return E_OK;
}
