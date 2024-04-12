#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "dsk.h"

//
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        puts("usage: dsk_extract filename dskfile");
        exit(E_FAIL);
    }

    DSK_Drive *drv = dsk_mount_drive(argv[2]);
    if (!drv)
    {
        printf("error: unable to mount DSK file %s\n", argv[1]);
        return E_FAIL;
    }

    if (dsk_extract_file(drv, argv[1]))
        return E_FAIL;

    printf("%s extracted.\n", argv[1]);

    return dsk_unload_drive(drv);
}
