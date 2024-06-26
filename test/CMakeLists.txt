cmake_minimum_required(VERSION 3.10)
project(test)

# set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -fno-stack-protector -fno-var-tracking")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -fno-stack-protector -fno-var-tracking")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS "-fPIC -ggdb -std=c++17 -Wall -fno-inline -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

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
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/${DIR})
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../src/${DIR} SOURCE_LIST)
endforeach()

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(fdatasync "unistd.h" CMAKE_HAVE_FDATASYNC)
check_cxx_symbol_exists(F_FULLFSYNC "fcntl.h" CMAKE_HAVE_FULLFSYNC)
check_cxx_symbol_exists(O_CLOEXEC "fcntl.h" CMAKE_HAVE_O_CLOEXEC)
if (CMAKE_HAVE_FDATASYNC)
    message("[INFO] [TEST] Found fdatasync")
    add_definitions(-DHAVE_FDATASYNC)
endif()
if (CMAKE_HAVE_FULLFSYNC)
    message("[INFO] [TEST] Found F_FULLFSYNC")
    add_definitions(-DHAVE_FULLFSYNC)
endif()
if (CMAKE_HAVE_O_CLOEXEC)
    message("[INFO] [TEST] Found O_CLOEXEC")
    add_definitions(-DHAVE_O_CLOEXEC)
endif()

set(DEPEND_LIB "")

include(CheckLibraryExists)
check_library_exists(crc32c crc32c_value "" CMAKE_HAVE_CRC32C)
if (CMAKE_HAVE_CRC32C)
    message("[INFO] [TEST] Found crc32c")
    add_definitions(-DHAVE_CRC32C)
    list(APPEND DEPEND_LIB crc32c)
endif()

find_library(SNAPPY_LIBRARY NAMES snappy PATHS "/usr/local/lib")
if(SNAPPY_LIBRARY)
    message("[INFO] [TEST] Found snappy")
    add_definitions(-DHAVE_SNAPPY)
    list(APPEND DEPEND_LIB snappy)
endif()

find_library(ZSTD_LIBRARY NAMES zstd PATHS "/usr/local/lib")
if(ZSTD_LIBRARY)
    message("[INFO] [TEST] Found zstd")
    add_definitions(-DHAVE_ZSTD)
    list(APPEND DEPEND_LIB zstd)
endif()

if("${TEST_FILE_NAME}" STREQUAL "test_")
    
else()
    list(APPEND DEPEND_LIB pthread gtest gmock)
    add_executable(${TEST_FILE_NAME} 
        ${TEST_FILE_NAME}.cpp
        ${SOURCE_LIST}
    )
    target_link_libraries(${TEST_FILE_NAME} ${DEPEND_LIB})
endif()