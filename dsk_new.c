#include <stdio.h>
#include <stdlib.h>
#include "dsk.h"

//
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("usage: dsk_new filename [tracks [sides]]");
        exit(E_FAIL);
    }

    int tracks = argc > 2 ? atoi(argv[2]) : 35;
    int sides = argc > 3 ? atoi(argv[3]) : 1;

    DSK_Drive *drv = dsk_new(argv[1], tracks, sides);
    if (!drv)
    {
        printf("error: unable to create DSK file %s\n", argv[1]);
        return E_FAIL;
    }

    printf("%s created.\n", argv[1]);
    
    return dsk_unmount_drive(drv);
}
