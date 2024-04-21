cmake_minimum_required(VERSION 3.10)

# set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -fno-stack-protector -fno-var-tracking")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -fno-stack-protector -fno-var-tracking")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "-fPIC -ggdb -std=c++17 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

set(SRC_DIR 
    algorithm
    block_builder
    cache
    comparator
    data_structure
    db_format
    env
    filter_policy
    iterator
    log
    log_reader
    log_writer
    memory
    options
    util
    write_batch
)

foreach(DIR ${SRC_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${DIR})
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/${DIR} SOURCE_LIST)
endforeach()

set(BUILD_PLATFORM "x86_64")

include(CheckCXXSymbolExists)
# Using check_cxx_symbol_exists() instead of check_c_symbol_exists() because
# we're including the header from C++, and feature detection should use the same
# compiler language that the project will use later. Principles aside, some
# versions of do not expose fdatasync() in <unistd.h> in standard C mode
# (-std=c11), but do expose the function in standard C++ mode (-std=c++11).
check_cxx_symbol_exists(fdatasync "unistd.h" CMAKE_HAVE_FDATASYNC)
check_cxx_symbol_exists(F_FULLFSYNC "fcntl.h" CMAKE_HAVE_FULLFSYNC)
check_cxx_symbol_exists(O_CLOEXEC "fcntl.h" CMAKE_HAVE_O_CLOEXEC)
# 判断fdatasync是否存在，若存在，则变量CMAKE_HAVE_FDATASYNC值为1；若不存在，则变量CMAKE_HAVE_FDATASYNC不存在
if (CMAKE_HAVE_FDATASYNC)
    # 由于C++检测不到CMAKE_HAVE_FDATASYNC的存在，所以需要添加新的变量定义，C++就可以根据新的变量进行预处理
    message("[INFO] [SRC] Found fdatasync")
    add_definitions(-DHAVE_FDATASYNC)
endif()
if (CMAKE_HAVE_FULLFSYNC)
    message("[INFO] [SRC] Found F_FULLFSYNC")
    add_definitions(-DHAVE_FULLFSYNC)
endif()
if (CMAKE_HAVE_O_CLOEXEC)
    message("[INFO] [SRC] Found O_CLOEXEC")
    add_definitions(-DHAVE_O_CLOEXEC)
endif()

set(DEPEND_LIB "")

include(CheckLibraryExists)
check_library_exists(crc32c crc32c_value "" CMAKE_HAVE_CRC32C)
if (CMAKE_HAVE_CRC32C)
    message("[INFO] [SRC] Found crc32c")
    add_definitions(-DHAVE_CRC32C)
    list(APPEND DEPEND_LIB crc32c)
endif()

find_library(SNAPPY_LIBRARY NAMES snappy PATHS "/usr/local/lib")
if(SNAPPY_LIBRARY)
    message("[INFO] [SRC] Found snappy")
    add_definitions(-DHAVE_SNAPPY)
    list(APPEND DEPEND_LIB snappy)
endif()

find_library(ZSTD_LIBRARY NAMES zstd PATHS "/usr/local/lib")
if(ZSTD_LIBRARY)
    message("[INFO] [SRC] Found zstd")
    add_definitions(-DHAVE_ZSTD)
    list(APPEND DEPEND_LIB zstd)
endif()


add_library(level_db_xy SHARED ${SOURCE_LIST})
target_link_libraries(level_db_xy ${DEPEND_LIB})

add_executable(level_db_xy_app 
    app/main.cpp
    ${SOURCE_LIST}
)
target_link_libraries(level_db_xy_app ${DEPEND_LIB})