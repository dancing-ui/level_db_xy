cmake_minimum_required(VERSION 3.10)
project(test)

# set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -fno-stack-protector -fno-var-tracking")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -fno-stack-protector -fno-var-tracking")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS "-rdynamic -fPIC -ggdb -std=c++17 -Wall -Wno-deprecated -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

set(SRC_DIR 
    data_structure
    memory
    log
    algorithm
    comparator
    db_format
    util
)


foreach(DIR ${SRC_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/${DIR})
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../src/${DIR} SOURCE_LIST)
endforeach()

if("${TEST_FILE_NAME}" STREQUAL "test_")
    
else()
    add_executable(${TEST_FILE_NAME} 
        ${TEST_FILE_NAME}.cpp
        ${SOURCE_LIST}
    )
    target_link_libraries(${TEST_FILE_NAME} libgtest.a -pthread)
endif()


