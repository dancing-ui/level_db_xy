#!/bin/bash

if [[ -d "build" ]]; then
    rm -r build
fi
mkdir build
cd build

# cmake -DTEST_FILE_NAME=test_${1} -DCMAKE_C_COMPILER="/usr/bin/clang" -DCMAKE_CXX_COMPILER="/usr/bin/clang++" .. 
# --debug-trycompile可以用于查看check_cxx_symbol_exists的查找情况
# cmake -DTEST_FILE_NAME=test_${1} --debug-trycompile .. 
cmake -DTEST_FILE_NAME=test_${1} .. 
make -j20

coredump_gen_dir=$(cat /proc/sys/kernel/core_pattern)
coredump_gen_dir=$(dirname $coredump_gen_dir)
build_file="test_$1"
build_path="test"
debug_coredump_dir="../coredump"
rm -rf $debug_coredump_dir/*

if [[ -f "$build_path/$build_file" ]]; then
    $build_path/$build_file
    if [[ $? -ne 0 ]]; then
        coredump_file=$(ls -l $coredump_gen_dir | grep $build_file | tail -n1 | awk '{print $9}')
        echo $coredump_file
        cp $coredump_gen_dir/$coredump_file $debug_coredump_dir
        cp $build_path/$build_file $debug_coredump_dir
        # gdb $debug_coredump_dir/$build_file $debug_coredump_dir/$coredump_file
        echo -e "[FAIL] 程序发生严重错误，请检查coredump目录 "
    fi
fi
