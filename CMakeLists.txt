cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(dsk VERSION 1.0 LANGUAGES C)

# testing
enable_testing()
add_test(NAME setup COMMAND cp Makefile a.txt)
add_test(NAME dsk_new COMMAND dsk_new foo.dsk)
add_test(NAME dsk_format COMMAND dsk_format FOO.DSK)
add_test(NAME dsk_add COMMAND dsk_add a.txt FOO.DSK)
add_test(NAME rename COMMAND mv a.txt b.txt)
add_test(NAME dsk_extract COMMAND dsk_extract a.txt FOO.DSK)
add_test(NAME compare COMMAND diff a.txt b.txt)
add_test(NAME cleanup_disk COMMAND rm FOO.DSK)
add_test(NAME cleanup_txt COMMAND rm a.txt)

# add the includes
include_directories(${PROJECT_SOURCE_DIR})

# add the library
add_library(dsk STATIC dsk.c)

#
# add the executables
#
add_executable(dsktools main.c)
target_link_libraries(dsktools dsk)

add_executable(dsk_new dsk_new.c)
target_link_libraries(dsk_new dsk)

add_executable(dsk_format dsk_format.c)
target_link_libraries(dsk_format dsk)

add_executable(dsk_add dsk_add.c)
target_link_libraries(dsk_add dsk)

add_executable(dsk_extract dsk_extract.c)
target_link_libraries(dsk_extract dsk)

# install targets
#install(TARGETS dsk DESTINATION lib)
#install(FILES dsk.h DESTINATION include)

# cmake --build . --config Release
# cmake --install . --prefix c:\opt -v
