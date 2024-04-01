#include <stdio.h>
#include <assert.h>
#include "dsk.h"

//
int main(int argc, char *argv[])
{
    DSK_Drive *drv;

    assert(sizeof(DSK_DirEntry) == 32);

    drv = dsk_mount_drive(argv[1]);

    // dsk_granule_map(drv);

    dsk_dir(drv);

    dsk_unload_drive(drv);
}
