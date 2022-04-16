#!/bin/bash
set -e
pushd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make color_detect
popd
sudo gdb --args ./build/color_detect ./http_server 1234
