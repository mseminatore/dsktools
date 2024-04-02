#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "dsk.h"

#define TRUE    1
#define FALSE   0
#define SMALL_BUFFER    256

typedef int (*cmd_func_t)(DSK_Drive *drv, void *params);

//
typedef struct
{
    char* cmd;
    cmd_func_t cmd_func;
    char *help_text;
    int hidden;
} Command;

//
int done = FALSE;
DSK_Drive *g_drv = NULL;
Command cmds[];

//---------------------------------
//
//---------------------------------
int quit_fn(DSK_Drive *drv, void *params)
{
    done = TRUE;
    return TRUE;
}

//---------------------------------
//
//---------------------------------
int dir_fn(DSK_Drive *drv, void *params)
{
    dsk_dir(drv);
    return TRUE;
}

//---------------------------------
//
//---------------------------------
int mount_fn(DSK_Drive *drv, void *params)
{
    char* filename = strtok(NULL, " \n");
    if (!filename)
    {
        puts("missing filename");
        return FALSE;
    }

    // unmount any already mounted drive
    if (g_drv)
        dsk_unload_drive(g_drv);

    g_drv = dsk_mount_drive(filename);
    if (!g_drv)
    {
        printf("unable to mount %s\n", filename);
        return FALSE;
    }

    return TRUE;
}

//---------------------------------
//
//---------------------------------
int unmount_fn(DSK_Drive *drv, void *params)
{
    dsk_unload_drive(drv);
    return TRUE;
}

//---------------------------------
//
//---------------------------------
int gran_map_fn(DSK_Drive *drv, void *params)
{
    dsk_granule_map(drv);
    return TRUE;
}

//---------------------------------
//
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
//
//---------------------------------
int free_fn(DSK_Drive *drv, void *params)
{
    printf("\n%d bytes (%d granules) free.\n", dsk_free_bytes(drv), dsk_free_granules(drv));

    return TRUE;
}

//---------------------------------
//
//---------------------------------
int add_fn(DSK_Drive *drv, void *params)
{
    char* filename = strtok(NULL, " \n");
    if (!filename)
    {
        puts("missing filename");
        return FALSE;
    }

    dsk_add_file(drv, filename);
    return TRUE;
}

//---------------------------------
//
//---------------------------------
int extract_fn(DSK_Drive *drv, void *params)
{
    char* filename = strtok(NULL, " \n");
    if (!filename)
    {
        puts("missing filename");
        return FALSE;
    }

    dsk_extract_file(drv, filename);
    return FALSE;
}

//---------------------------------
//
//---------------------------------
Command cmds[] =
{
    {"add", add_fn, "add file to DSK"},
    {"dir", dir_fn, "list directory contents", 0},
    {"extract", extract_fn, "extract file from DSK"},
    {"free", free_fn, "free space on drive", 0},
    {"grans", gran_map_fn, "show granule map", 0},
    {"help", help_fn, "list commands", 0},
    {"mount", mount_fn, "mount a DSK file", 0},
    {"unmount", unmount_fn, "unmount current DSK file", 0},
    {"q", quit_fn , "quit app", 1},
    {"quit", quit_fn , "quit app", 0},
 
    // {"r", print_regs_fn },
    // {"s", step_fn },
    // {"g", run_fn },
    // {"m", dump_mem_fn},
    // {"db", dump_byte_fn},
    // {"dw", dump_word_fn},
    // {"f", fill_mem_fn},
    // {"d", disasm_fn},

    // {"l", load_mem_fn},
    // {"w", write_mem_fn},

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
            return pCmd->cmd_func(drv, NULL);
        }
    }

    // command not found!
    return FALSE;
}

//
int main(int argc, char *argv[])
{
    char buf[256];

    assert(sizeof(DSK_DirEntry) == 32);

    if (argc > 1)
        g_drv = dsk_mount_drive(argv[1]);

    printf("\nDSKTools - Welcome to the CoCo DSK file tool!\n\n");

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
}
