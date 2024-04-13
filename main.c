#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "dsk.h"

#define VERSION_STRING "0.2.0"

#define TRUE    1
#define FALSE   0
#define SMALL_BUFFER    256

typedef int (*cmd_func_t)(DSK_Drive *drv, void *params);

typedef enum {CMD_SHOW, CMD_HIDDEN} CommandVisibility;

//---------------------------------
// command table entry
//---------------------------------
typedef struct
{
    char* cmd;
    cmd_func_t cmd_func;
    char *help_text;
    CommandVisibility hidden;
} Command;

//---------------------------------
// global vars
//---------------------------------
int done = FALSE;
DSK_Drive *g_drv = NULL;
Command cmds[];

//---------------------------------
// quit the program
//---------------------------------
int quit_fn(DSK_Drive *drv, void *params)
{
    done = TRUE;
    return TRUE;
}

//---------------------------------
// display DSK directory
//---------------------------------
int dir_fn(DSK_Drive *drv, void *params)
{
    dsk_dir(drv);
    return TRUE;
}

//---------------------------------
// mount a DSK file
//---------------------------------
int mount_fn(DSK_Drive *drv, void *params)
{
    char* filename = strtok(NULL, " \n");
    if (!filename)
    {
        puts("missing filename.");
        return FALSE;
    }

    // unmount any already mounted drive
    if (g_drv)
        dsk_unmount_drive(g_drv);

    g_drv = dsk_mount_drive(filename);
    if (!g_drv)
    {
        // printf("unable to mount (%s)\n", filename);
        return FALSE;
    }

    return TRUE;
}

//---------------------------------
// unmount the current DSK file
//---------------------------------
int unmount_fn(DSK_Drive *drv, void *params)
{
    dsk_unmount_drive(drv);
    return TRUE;
}

//---------------------------------
// display the granule map
//---------------------------------
int gran_map_fn(DSK_Drive *drv, void *params)
{
    dsk_granule_map(drv);
    return TRUE;
}

//---------------------------------
// display program help
//---------------------------------
int help_fn(DSK_Drive *drv, void *params)
{
    Command *pcmd = cmds;

    for(; pcmd->cmd; pcmd++)
    {
        if (!pcmd->hidden)
            printf("%s\t- %s\n", pcmd->cmd, pcmd->help_text);
    }

    return TRUE;
}

//---------------------------------
// display free space on DSK
//---------------------------------
int free_fn(DSK_Drive *drv, void *params)
{
    printf("\n%d bytes (%d granules) free.\n", dsk_free_bytes(drv), dsk_free_granules(drv));

    return TRUE;
}

//---------------------------------
// add a file to the DSK
//---------------------------------
int add_fn(DSK_Drive *drv, void *params)
{
    char* filename = strtok(NULL, " \n");
    if (!filename)
    {
        puts("missing filename.");
        return FALSE;
    }

    // look for optional file mode and type
    char *pmode = strtok(NULL, " \n");
    DSK_OPEN_MODE mode = DSK_MODE_BINARY;
    if (pmode && toupper(pmode[0]) == 'A')
        mode = DSK_MODE_ASCII;

    char *ptype = strtok(NULL, " \n");
    DSK_FILE_TYPE type = DSK_TYPE_ML;
    if (ptype)
    {
        if (toupper(ptype[0]) == 'B')
            type = DSK_TYPE_BASIC;
        else if (toupper(ptype[0]) == 'D')
            type = DSK_TYPE_DATA;
        else if (toupper(ptype[0]) == 'T')
            type = DSK_TYPE_TEXT;
    }

    dsk_add_file(drv, filename, mode, type);
    return TRUE;
}

//---------------------------------
// extract a file from the DSK
//---------------------------------
int extract_fn(DSK_Drive *drv, void *params)
{
    char* filename = strtok(NULL, " \n");
    if (!filename)
    {
        puts("missing filename.");
        return FALSE;
    }

    dsk_extract_file(drv, filename);
    return TRUE;
}

//---------------------------------
// create a new DSK file
//---------------------------------
int new_fn(DSK_Drive *drv, void *params)
{
    char* filename = strtok(NULL, " \n");
    if (!filename)
    {
        puts("missing filename.");
        return FALSE;
    }

    // unmount current DSK
    if (drv)
        dsk_unmount_drive(drv);
    
    g_drv = dsk_new(filename);

    return TRUE;
}

//---------------------------------
// format the mounted DSK file
//---------------------------------
int format_fn(DSK_Drive *drv, void *params)
{
    dsk_format(drv);

    return TRUE;
}

//---------------------------------
// delete file from DSK file
//---------------------------------
int del_fn(DSK_Drive *drv, void *params)
{
    char* filename = strtok(NULL, " \n");
    if (!filename)
    {
        puts("missing filename.");
        return FALSE;
    }

    dsk_del(drv, filename);

    return TRUE;
}

//---------------------------------
// rename file1 to file2
//---------------------------------
int rename_fn(DSK_Drive *drv, void *params)
{
    char *file1 = strtok(NULL, " \n");
    char *file2 = strtok(NULL, " \n");
    if (!file1 || !file2)
    {
        puts("missing filename.");
        return FALSE;
    }

    dsk_rename(drv, file1, file2);

    return TRUE;
}

//---------------------------------
// command table
//---------------------------------
Command cmds[] =
{
    {"add", add_fn, "add filename \t\t(adds file to mounted DSK)", CMD_SHOW },
    {"del", del_fn, "del filename \t(delete file from mounted DSK)", CMD_HIDDEN },
    {"dir", dir_fn, "dir \t\t\t(list directory of mounted DSK)", CMD_SHOW },
    {"dskini", format_fn, "dskini \t(format mounted DSK)", CMD_HIDDEN },
    {"extract", extract_fn, "extract filename \t(extracts file from mounted DSK)", CMD_SHOW },
    {"format", format_fn, "format \t\t(format currently mounted DSK)", CMD_SHOW },
    {"free", free_fn, "free \t\t\t(report free space on mounted DSK", CMD_SHOW },
    {"grans", gran_map_fn, "grans \t\t(show granule map)", CMD_SHOW },
    {"help", help_fn, "help \t\t\t(list commands and usage)", CMD_SHOW },
    {"kill", del_fn, "kill filename \t(delete file from mounted DSK)", CMD_SHOW},
    {"ls", dir_fn, "ls \t(list directory of mounted DSK)", CMD_HIDDEN },
    {"mount", mount_fn, "mount filename \t(mount a DSK file)", CMD_SHOW },
    {"new", new_fn, "new \t\t\t(create new DSK)", CMD_SHOW },
    {"unload", unmount_fn, "unload \t\t(unmount current DSK file)", CMD_HIDDEN },
    {"unmount", unmount_fn, "unmount \t\t(unmount current DSK file)", CMD_SHOW },
    {"q", quit_fn , "q \t\t\t(quit dsktools)", CMD_HIDDEN },
    {"quit", quit_fn , "quit \t\t\t(quit dsktools)", CMD_SHOW },
    {"rename", rename_fn, "rename file1 file2 \t(rename file1 to file2 on mounted DSK)", CMD_SHOW},
    {"ren", rename_fn, "rename file1 file2 \t(rename file1 to file2 on mounted DSK)", CMD_HIDDEN},
    {"rm", del_fn, "rm \t(delete file from mounted DSK)", CMD_HIDDEN},

    { NULL, NULL , NULL}
};

//-------------------
// execute dbg cmd
//-------------------
int exec_cmd(DSK_Drive *drv, char *cmd)
{
    Command *pCmd = cmds;

    if (!cmd)
        return TRUE;

    for (; pCmd->cmd != NULL; pCmd++)
    {
        if (!strcmp(cmd, pCmd->cmd))
        {
            // TODO - parse extra params and pass along instead of NULL?
            return pCmd->cmd_func(drv, NULL);
        }
    }

    // command not found!
    return FALSE;
}

//
// from https://www.askapache.com/online-tools/figlet-ascii/
//
void banner()
{
    puts("     _     _    _              _     ");
    puts("    | |   | |  | |            | |    ");
    puts("  __| |___| | _| |_ ___   ___ | |___ ");
    puts(" / _` / __| |/ / __/ _ \\ / _ \\| / __|");
    puts("| (_| \\__ \\   <| || (_) | (_) | \\__ \\");
    puts(" \\__,_|___/_|\\_\\\\__\\___/ \\___/|_|___/\n");
}

//-------------------
// main program start
//-------------------
int main(int argc, char *argv[])
{
    char buf[SMALL_BUFFER];

    assert(sizeof(DSK_DirEntry) == 32);

    if (argc > 1)
        g_drv = dsk_mount_drive(argv[1]);

    banner();

    printf("\nDSKTools v%s - Welcome to the CoCo DSK file tool!\n", VERSION_STRING);
    printf("\t(type help for list of commands)\n");

    while (!done)
    {
        printf("\ndsktools>");
        fgets(buf, SMALL_BUFFER - 1, stdin);

        char* pCmd = strtok(buf, " \n");
        if (!exec_cmd(g_drv, pCmd))
            printf("Command '%s' failed.\n", pCmd);
        else
            puts("OK");
    }

    return 0;
}
