cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(dsk VERSION 1.0 LANGUAGES C)

# testing
#enable_testing()
#add_test(NAME blas_test COMMAND blas_test)

# add the includes
include_directories(${PROJECT_SOURCE_DIR})

# add the library
add_library(dsk STATIC dsk.c)

#
# add the executables
#
add_executable(dsktools main.c)
target_link_libraries(dsktools dsk)

# install targets
#install(TARGETS dsk DESTINATION lib)
#install(FILES dsk.h DESTINATION include)

# cmake --build . --config Release
# cmake --install . --prefix c:\opt -v
