#!/bin/bash

rm -r build
mkdir build
cd build

cmake -DTEST_FILE_NAME=test_${1} .. 
make -j20

if [ -n "${1}" ]; then
    test/test_${1}
fi
