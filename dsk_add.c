#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "dsk.h"

//
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        puts("usage: dsk_add filename dskfile [ASCII|BINARY] [BASIC|ML|TEXT|DATA]");
        exit(E_FAIL);
    }

    DSK_Drive *drv = dsk_mount_drive(argv[2]);
    if (!drv)
    {
        printf("error: unable to mount DSK file %s\n", argv[1]);
        return E_FAIL;
    }

    // look for optional file mode and type
    char *pmode = argv[3];
    DSK_OPEN_MODE mode = DSK_MODE_BINARY;
    if (pmode && toupper(pmode[0]) == 'A')
        mode = DSK_MODE_ASCII;

    char *ptype = argv[4];
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

    if (dsk_add_file(drv, argv[1], mode, type))
        return E_FAIL;

    printf("%s added.\n", argv[1]);

    return dsk_unmount_drive(drv);
}
