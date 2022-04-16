#!/bin/bash
sudo killall color_detect

set -e
pushd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make color_detect
popd
sudo ./build/color_detect ./http_server 1234
