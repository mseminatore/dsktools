#include <stdio.h>
#include <assert.h>
#include "dsk.h"

//
int main(int argc, char *argv[])
{
    assert(sizeof(DSK_DirEntry) == 32);

    dsk_mount_drive(argv[1]);

    // dir();

    dsk_unload_drive();
}
