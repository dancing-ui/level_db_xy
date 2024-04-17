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

check_cxx_symbol_exists(fdatasync "unistd.h" HAVE_FDATASYNC)
check_cxx_symbol_exists(F_FULLFSYNC "fcntl.h" HAVE_FULLFSYNC)
check_cxx_symbol_exists(O_CLOEXEC "fcntl.h" HAVE_O_CLOEXEC)

if("${TEST_FILE_NAME}" STREQUAL "test_")
    
else()
    set(DEPEND_LIB -pthread libgtest.a libgmock.a)
    add_executable(${TEST_FILE_NAME} 
        ${TEST_FILE_NAME}.cpp
        ${SOURCE_LIST}
    )
    target_link_libraries(${TEST_FILE_NAME} ${DEPEND_LIB})
endif()