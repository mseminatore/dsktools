#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
// #include <assert.h>
// #include <ctype.h>
#include "dsk.h"

//
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("usage: dsk_format filename");
        exit(E_FAIL);
    }

    DSK_Drive *drv = dsk_mount_drive(argv[1]);
    if (!drv)
    {
        printf("error: unable to mount DSK file %s\n", argv[1]);
        return E_FAIL;
    }

    if (dsk_format(drv))
        return E_FAIL;

    printf("%s formatted.\n", argv[1]);

    return dsk_unmount_drive(drv);
}
