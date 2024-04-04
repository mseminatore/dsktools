#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "dsk.h"

#define VERSION_STRING "0.1.0"

#define TRUE    1
#define FALSE   0
#define SMALL_BUFFER    256

typedef int (*cmd_func_t)(DSK_Drive *drv, void *params);

typedef enum {CMD_SHOW, CMD_HIDDEN} CommandVisibility;
//
typedef struct
{
    char* cmd;
    cmd_func_t cmd_func;
    char *help_text;
    CommandVisibility hidden;
} Command;

//
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
        dsk_unload_drive(g_drv);

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
    dsk_unload_drive(drv);
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

    char *pmode = strtok(NULL, " \n");
    OpenMode mode = MODE_BINARY;
    if (pmode && toupper(pmode[0]) == 'A')
        mode = MODE_ASCII;

    dsk_add_file(drv, filename, mode);
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
        dsk_unload_drive(drv);
    
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
    {"add", add_fn, "add file to DSK", CMD_SHOW },
    {"del", del_fn, "delete file from DSK", CMD_HIDDEN },
    {"dir", dir_fn, "list directory contents", CMD_SHOW },
    {"dskini", format_fn, "format DSK", CMD_HIDDEN },
    {"extract", extract_fn, "extract file from DSK", CMD_SHOW },
    {"format", format_fn, "format DSK", CMD_SHOW },
    {"free", free_fn, "report free space on drive", CMD_SHOW },
    {"grans", gran_map_fn, "show granule map", CMD_SHOW },
    {"help", help_fn, "list commands", CMD_SHOW },
    {"kill", del_fn, "delete file from DSK", CMD_SHOW},
    {"ls", dir_fn, "list directory contents", CMD_HIDDEN },
    {"mount", mount_fn, "mount a DSK file", CMD_SHOW },
    {"new", new_fn, "create new DSK", CMD_SHOW },
    {"unload", unmount_fn, "unmount current DSK file", CMD_SHOW },
    {"unmount", unmount_fn, "unmount current DSK file", CMD_HIDDEN },
    {"q", quit_fn , "quit app", CMD_HIDDEN },
    {"quit", quit_fn , "quit app", CMD_SHOW },
    {"rename", rename_fn, "rename file", CMD_HIDDEN},

    { NULL, NULL , NULL}
};

//-------------------
// execute dbg cmd
//-------------------
int exec_cmd(DSK_Drive *drv, char *cmd)
{
    Command *pCmd = cmds;

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

//-------------------
// main program start
//-------------------
int main(int argc, char *argv[])
{
    char buf[SMALL_BUFFER];

    assert(sizeof(DSK_DirEntry) == 32);

    if (argc > 1)
        g_drv = dsk_mount_drive(argv[1]);

    printf("\nDSKTools v%s - Welcome to the CoCo DSK file tool!\n", VERSION_STRING);
    printf("\t(type help for list of commands)\n");

    while (!done)
    {
        printf("\ndsk>");
        fgets(buf, SMALL_BUFFER - 1, stdin);

        char* pCmd = strtok(buf, " \n");
        if (!exec_cmd(g_drv, pCmd))
            printf("Command '%s' failed.\n", pCmd);
        else
            puts("OK");
    }

    return 0;
}
