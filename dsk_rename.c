#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "dsk.h"

//
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        puts("usage: dsk_ren dskfile file1 file2");
        exit(E_FAIL);
    }

    DSK_Drive *drv = dsk_mount_drive(argv[1]);
    if (!drv)
    {
        printf("error: unable to mount DSK file %s\n", argv[1]);
        return E_FAIL;
    }

    if (dsk_rename(drv, argv[2], argv[3]))
        return E_FAIL;

    printf("renamed '%s' to '%s'.\n", argv[2], argv[3]);

    return dsk_unmount_drive(drv);
}
