#!/bin/bash
set -e
pushd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make color_detect
popd
sudo LD_PRELOAD=/usr/lib/gcc/arm-linux-gnueabihf/10/libasan.so ./build/color_detect ./http_server 1234
