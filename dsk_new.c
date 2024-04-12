#include <stdio.h>
#include <stdlib.h>
#include "dsk.h"

//
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        puts("usage: dsk_new filename");
        exit(E_FAIL);
    }

    DSK_Drive *drv = dsk_new(argv[1]);
    if (!drv)
    {
        printf("error: unable to create DSK file %s\n", argv[1]);
        return E_FAIL;
    }

    printf("%s created.\n", argv[1]);
    
    return dsk_unload_drive(drv);
}
