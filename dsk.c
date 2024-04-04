#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "dsk.h"

#ifdef _WIN32
#   define DIR_SEPARATOR '\\'
#else
#   define DIR_SEPARATOR '/'
#endif

static void dsk_default_output(const char *s);
DSK_Print dsk_puts = dsk_default_output;

//----------------------------------------
// return pointer to the base filename without path
//----------------------------------------
static const char *dsk_basename(const char *s)
{
	if (!strchr(s, DIR_SEPARATOR))
		return s;

	const char *p = s + strlen(s);

	for (; p != s; p--)
		if (*p == DIR_SEPARATOR)
		{
			p++;
			break;
		}
	return p;
}

//----------------------------------------
// default output function
//----------------------------------------
static void dsk_default_output(const char *s)
{
	fputs(s, stdout);
}

//----------------------------------------
// DSK formatted output function
//----------------------------------------
static void dsk_printf(char *format, ...)
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
        dsk_printf("disk invalid.\n");
        return E_FAIL;
    }

    // get granule count and count of sectors in last granule
    int sectors = 0;
    int grans = count_granules(drv, dirent->first_granule, &sectors);

    // add in size of full granules
    int size = (grans - 1) * DSK_BYTES_PER_GRANULE;

    // add in size of full sectors
    if (sectors)
    {
        if (dirent->bytes_in_last_sector)
            size += (sectors - 1) * DSK_BYTES_DATA_PER_SECTOR;
        else
            size += (sectors) * DSK_BYTES_DATA_PER_SECTOR;
    }

    // add in bytes in last sector
    size += ntohs(dirent->bytes_in_last_sector);

    DSK_TRACE("%d total grans, %d sectors in last gran, %d bytes in last sector\n", grans, sectors, ntohs(dirent->bytes_in_last_sector));

    return size;
}

//------------------------------------
//
//------------------------------------
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

//------------------------------------
//
//------------------------------------
static DSK_DirEntry *find_file_in_dir(DSK_Drive *drv, const char *filename)
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

//----------------------------------------
// override the default output function
//----------------------------------------
void dsk_set_output_function(DSK_Print f)
{
    dsk_puts = f;
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
// calc free bytes on DSK
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
//
//------------------------------------
int dsk_seek_to_granule(DSK_Drive *drv, int granule)
{
    assert(drv && drv->fp);
    assert(granule >= 0 && granule <= DSK_LAST_GRANULE);

    int track = granule / DSK_GRANULES_PER_TRACK;
    int sector = 1 + (granule % DSK_GRANULES_PER_TRACK) * DSK_SECTORS_PER_GRANULE;

    return dsk_seek_drive(drv, track, sector);
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
        if (0 == (i%23))
            dsk_printf("\n");
    }

    dsk_printf("\n");

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

    // save the DSK filename
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
// find and return a free dir entry
//------------------------------------
static DSK_DirEntry *find_free_dir_entry(DSK_Drive *drv)
{
    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid\n");
        return NULL;
    }

    // find dir entry
    for (int i = 0; i < DSK_MAX_DIR_ENTRIES; i++)
    {
        DSK_DirEntry *dirent = &drv->dirs[i];

        if (dirent->filename[0] == DSK_DIRENT_DELETED || DSK_DIRENT_FREE == (uint8_t)dirent->filename[0])
            return dirent;
    }

    return NULL;
}

//------------------------------------
// uppercase the string
//------------------------------------
static char *string_upper(char *s)
{
    char *pstr = s;

    for (; *s; s++)
    {
        *s = toupper(*s);
    }

    return pstr;
}

//------------------------------------
// return the first free granule
//------------------------------------
static int find_first_free_granule(DSK_Drive *drv)
{
    for (int i = 0; i < DSK_TOTAL_GRANULES; i++)
    {
        if (drv->fat.granule_map[i] == DSK_GRANULE_FREE)
        {
            dsk_printf("returning free granule: %d\n", i);
            return i;
        }
    }

    return -1;
}

//------------------------------------
// add file to a mounted DSK file
//------------------------------------
int dsk_add_file(DSK_Drive *drv, const char *filename, DSK_OPEN_MODE mode, DSK_FILE_TYPE type)
{
    char *pmode = "rb";
    char sector_data[DSK_BYTES_DATA_PER_SECTOR];
    char dest_filename[DSK_MAX_FILENAME + DSK_MAX_EXT + 2];

    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid.\n");
        return E_FAIL;
    }

    // get dest filename and ensure upper case
    strcpy(dest_filename, dsk_basename(filename));
    string_upper(dest_filename);
    DSK_TRACE("adding file '%s'\n", dest_filename);

    // check filename.ext length
    if (strlen(filename) > DSK_MAX_FILENAME + DSK_MAX_EXT + 1)
    {
        dsk_printf("filename '%s' is too long.\n", dest_filename);
        return E_FAIL;
    }

    // open the input file
    if (mode == DSK_MODE_ASCII)
        pmode = "rt";

    FILE *fin = fopen(filename, pmode);
    if (!fin)
    {
        dsk_printf("file not found.\n");
        return E_FAIL;
    }

    // get the file size
    fseek(fin, 0, SEEK_END);
    long fin_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    // check that disk has space for file
    if (fin_size > dsk_free_bytes(drv))
    {
        dsk_printf("out of space.\n");
        fclose(fin);
        return E_FAIL;
    }

    // see if file already exists on DSK
    DSK_DirEntry *dirent = find_file_in_dir(drv, filename);
    if (dirent)
    {
        dsk_printf("file already exists.\n");
        fclose(fin);
        return E_FAIL;
    }

    // find first free directory entry
    dirent = find_free_dir_entry(drv);
    if (!dirent)
    {
        dsk_printf("drive is full.\n");
        fclose(fin);
        return E_FAIL;
    }

    // update directory entry
    char *basefile = strtok(dest_filename, ".");
    char *ext = strtok(NULL, "");

    // copy in the filename, left justified, padded with spaces
    for (int i = 0; i < DSK_MAX_FILENAME; i++)
    {
        if (i < strlen(basefile))
            dirent->filename[i]= basefile[i];
        else
            dirent->filename[i] = ' ';
    }

    // copy in the extension, left justified, padded with spaces
    for (int i = 0; i < DSK_MAX_EXT; i++)
    {
        if (i < strlen(ext))
            dirent->ext[i] = ext[i];
        else
            dirent->ext[i] = ' ';
    }

    DSK_TRACE("%s -> file: %s, ext: %s\n", dest_filename, basefile, ext);
    
    dirent->binary_ascii = (mode == DSK_MODE_ASCII) ? 0xFF : 0;
    dirent->type = type;

    // find first free granule
    int next_gran, gran = find_first_free_granule(drv);
    dirent->first_granule = gran;

    // copy full granule data
    for (; fin_size >= DSK_BYTES_PER_GRANULE; fin_size -= DSK_BYTES_PER_GRANULE)
    {
        dsk_seek_to_granule(drv, gran);
        for (int sector = 0; sector < DSK_SECTORS_PER_GRANULE; sector++)
        {
            fread(sector_data, sizeof(sector_data), 1, fin);
            fwrite(sector_data, sizeof(sector_data), 1, drv->fp);
        }

        // temporarily mark this granule as used find chooses next avail
        drv->fat.granule_map[gran] = 0xC0;

        // find next available granule
        next_gran = find_first_free_granule(drv);

        // patch current granule point to next granule
        drv->fat.granule_map[gran] = next_gran;
        gran = next_gran;
    }

    // find number of sectors used
    int tail_sectors = fin_size / DSK_BYTES_DATA_PER_SECTOR;
    int extra_bytes = fin_size % DSK_BYTES_DATA_PER_SECTOR;
    DSK_TRACE("fin_size: %d, tail sectors: %d, extra bytes: %d\n", fin_size, tail_sectors, extra_bytes);

    dsk_seek_to_granule(drv, gran);
    for (int sector = 0; sector < tail_sectors; sector++)
    {
        fread(sector_data, sizeof(sector_data), 1, fin);
        fwrite(sector_data, sizeof(sector_data), 1, drv->fp);
    }

    fread(sector_data, extra_bytes, 1, fin);
    fwrite(sector_data, extra_bytes, 1, drv->fp);
    
    // mark last granule
    drv->fat.granule_map[gran] = 0xC0 + tail_sectors;

    // update bytes in last sector, respecting endianess
    dirent->bytes_in_last_sector = htons(extra_bytes);

    fclose(fin);

    // update DSK image
    drv->dirty_flag = 1;
    dsk_flush(drv);

    return E_OK;
}

//------------------------------------
// extract a file from the DSK
//------------------------------------
int dsk_extract_file(DSK_Drive *drv, const char *filename)
{
    char sector_data[DSK_BYTES_DATA_PER_SECTOR];

    assert(drv && drv->fp);
    if (!drv || !drv->fp)
    {
        dsk_printf("disk invalid.\n");
        return E_FAIL;
    }

    DSK_DirEntry *dirent = find_file_in_dir(drv, filename);
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
    while(!DSK_IS_LAST_GRANULE(drv->fat.granule_map[gran]))
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
    int next_gran = drv->fat.granule_map[gran];
    int tail_sectors = (next_gran & DSK_SECTOR_COUNT_MASK);
    int bytes_in_last_sector = ntohs(dirent->bytes_in_last_sector);

    dsk_printf("tail sectors in granule %02X: %d\n", gran, tail_sectors);

    // extract full sectors
    if (bytes_in_last_sector)
        tail_sectors--;

    dsk_seek_to_granule(drv, gran);
    for (int i = 0; i < tail_sectors; i++)
    {
        fread(sector_data, DSK_BYTES_DATA_PER_SECTOR, 1, drv->fp);
        fwrite(sector_data, DSK_BYTES_DATA_PER_SECTOR, 1, fout);
    }

    // extract partial sector
    dsk_printf("bytes in last sector: %d\n", bytes_in_last_sector);

    if (bytes_in_last_sector)
    {
        fread(sector_data, bytes_in_last_sector, 1, drv->fp);
        fwrite(sector_data, bytes_in_last_sector, 1, fout);
    }

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

    DSK_DirEntry *dirent = find_file_in_dir(drv, filename);
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

    // flush changes to DSK file
    drv->dirty_flag = 1;
    dsk_flush(drv);

    return E_OK;
}

//------------------------------------
// rename file1 to file2
//------------------------------------
int dsk_rename(DSK_Drive *drv, char *file1, char *file2)
{
    string_upper(file1);
    string_upper(file2);

    if (strlen(file2) > DSK_MAX_FILENAME + DSK_MAX_EXT + 1)
    {
        dsk_printf("filename too long.\n");
    }

    DSK_DirEntry *dirent = find_file_in_dir(drv, file1);
    if (!dirent)
    {
        dsk_printf("file not found.\n");
        return E_FAIL;
    }

    // TODO - copy fhe filename
    // TODO - copy the extension
    // for (int i = 0; i < )

    // flush changes to DSK file
    drv->dirty_flag = 1;
    dsk_flush(drv);

    return E_OK;
}
