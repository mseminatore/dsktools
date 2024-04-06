# dsktools

DskTools is a library (libdsk) and application (dsktools) for working with 
TRS-80 Color Computer virtual disc (.DSK) files in JVC format. No JVC header
is created or recognized.

> At this time only the original 35 track (160K) format is supported.

# Why create this library?

First, I have a particular fondness for the TRS-80 Color Computer. It was the
first computer that I owned and I learned a great deal about programming from
tinkering with that machine.

Second, I've never had the opportunity to work directly on the low-level
aspects of a file system.
The DSK format is clearly [documented](http://cocosdc.blogspot.com/p/sd-card-socket-sd-card-socket-is-push.html#:~:text=DSK%20Images&text=Images%20in%20this%20format%20consist,to%20precede%20the%20sector%20array.).
That seened to make DSK files a good candidate for learning.

> The original physical floppy disc format is documented [here](https://colorcomputerarchive.com/repo/Documents/Manuals/Hardware/Color%20Computer%20Disk%20System%20(Tandy).pdf#page27).

# Library functions

The libdsk library provides a number of functions for working with DSK files.

Function | Description
-------- | -----------
dsk_seek_drive | seek to given track and sector
dsk_mount_drive | mount a DSK file
dsk_unload_drive | unmount a DSK file
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
