# dsktools

![GitHub License](https://img.shields.io/github/license/mseminatore/dsktools)
[![CMake](https://github.com/mseminatore/dsktools/actions/workflows/cmake.yml/badge.svg)](https://github.com/mseminatore/dsktools/actions/workflows/cmake.yml)

DskTools is a library (libdsk) and application (dsktools) for working with 
TRS-80 Color Computer (CoCo) virtual disk (.DSK) files in JVC format. No JVC header
is created or recognized.

> Note: By default the original 35 track (160K) format is supported. Starting with
> v0.4.0 the dsk_new() API has been updated to allow for arbitrary track/side
> configurations.

# Why create this library?

I have a particular fondness for the TRS-80 Color Computer. It was the
first computer that I owned and I learned a great deal about programming from
tinkering with that machine. Amazingly I still have a functioning device! 
Using the [CoCo SDC](https://retrorewind.ca/coco-sdc) from RetroRewind I am 
able to move files between my CoCo and other devices via SD Card. The Coco SDC
accesses files from within virtual disk drives on the SD Card. I needed a way to 
manipulate DSK files in order to move files to/from my device.

While there are a number of existing tools for working with DSK files,
I've never had the opportunity to work directly on a file system. As the
DSK format is clearly
[documented](http://cocosdc.blogspot.com/p/sd-card-socket-sd-card-socket-is-push.html#:~:text=DSK%20Images&text=Images%20in%20this%20format%20consist,to%20precede%20the%20sector%20array.)
it seemed that DSK files were a good project for learning about the the 
low-level aspects of file systems.

> The original physical floppy disc format is documented [here](https://colorcomputerarchive.com/repo/Documents/Manuals/Hardware/Color%20Computer%20Disk%20System%20(Tandy).pdf#page27).

Interestingly, Microsoft licensed [Basic-69](https://en.wikipedia.org/wiki/Microsoft_BASIC#BASIC-68_and_BASIC-69)
to Tandy for the Color Computer. This became the basis of the Color BASIC 
implementation. And the later Disk Extended Color Basic ("DECB") uses an early form
of the [8-bit FAT](https://en.wikipedia.org/wiki/File_Allocation_Table#FAT8) 
file system from  Microsoft BASIC-80. Though most of us know about FAT from PC-DOS 
and MS-DOS. So this project gave me an opportunity to explore the implementation 
of a FAT-based system.

# Library functions

The libdsk library provides a number of functions for working with DSK files.

Function | Description
-------- | -----------
dsk_seek_drive | seek to given track and sector
dsk_mount_drive | mount a DSK file
dsk_unmount_drive | unmount a DSK file
dsk_dir | display directory of mounted DSK file
dsk_free_bytes | return number of free bytes on DSK
dsk_free_granules | return number of free granules on DSK
dsk_add_file | add a new file to the DSK
dsk_extract_file | extract a file from the DSK
dsk_new | create a new (empty) DSK file
dsk_format | format a DSK file (erases contents)
dsk_flush | sync directory and FAT to DSK
dsk_del | delete a file from the DSK
dsk_set_output_function | replace the default output function
dsk_rename | rename a file on the DSK

# Code Examples

Working with libdsk is straightforward. Simply include dsk.h and link to libdsk and
you are ready to begin working with DSK files.

Below is an example of creating a new, empty DSK file.

```C
#include <stdio.h>
#include "dsk.h"

int main(int argc, char *argv[])
{
    // create (and format) a new blank DSK file
    return dsk_new(argv[1]);
}
```

And here is an example of mounting an existing DSK file and displaying the
directory.

```C
#include <stdio.h>
#include "dsk.h"

int main(int argc, char *argv[])
{
    // attempt to mount an existing DSK file
    DSK_Drive *drv = dsk_mount(argv[1]);

    // display the directory
    if (drv)
        dsk_dir(drv);

    return 0;
}
```

More complete examples of the library and its usage are provided by the `dsktools` 
application and related tools.
