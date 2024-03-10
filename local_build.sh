#!/bin/bash

rm -r build
mkdir build
cd build
cmake .. 
make -j20

test/${1}