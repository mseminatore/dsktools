cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(dsk VERSION 1.0 LANGUAGES C)

if ( WIN32 )
	set(COPY_CMD "copy")
	set(DEL_CMD "del")
	set(DIFF_CMD "fc")
else()
	set(COPY_CMD "cp")
	set(DEL_CMD "rm")
	set(DIFF_CMD "diff")
endif()

# testing
enable_testing()

#set(test_file ${CMAKE_CURRENT_BINARY_DIR}/a.txt)
set(test_file a.txt)
add_test(NAME create_file COMMAND ${CMAKE_COMMAND} -E copy CMakeCache.txt ${test_file})
add_test(NAME dsk_new COMMAND dsk_new foo.dsk)
add_test(NAME dsk_format COMMAND dsk_format FOO.DSK)
add_test(NAME dsk_add COMMAND dsk_add ${test_file} FOO.DSK ascii text)
add_test(NAME dsk_rename COMMAND dsk_rename FOO.DSK ${test_file} b.txt)
add_test(NAME dsk_extract COMMAND dsk_extract b.txt FOO.DSK)
add_test(NAME dsk_del COMMAND dsk_del b.txt FOO.DSK)
add_test(NAME compare COMMAND ${CMAKE_COMMAND} -E compare_files ${test_file} b.txt)
add_test(NAME cleanup_disk COMMAND ${CMAKE_COMMAND} -E rm FOO.DSK)
add_test(NAME cleanup_txt COMMAND ${CMAKE_COMMAND} -E rm ${test_file} b.txt)

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

add_executable(dsk_del dsk_del.c)
target_link_libraries(dsk_del dsk)

add_executable(dsk_rename dsk_rename.c)
target_link_libraries(dsk_rename dsk)

# install targets
#install(TARGETS dsk DESTINATION lib)
#install(FILES dsk.h DESTINATION include)

# cmake --build . --config Release
# cmake --install . --prefix c:\opt -v
