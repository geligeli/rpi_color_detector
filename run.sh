#!/bin/bash
sudo killall color_detect

set -e
pushd build
cmake ..
make color_detect
popd
sudo ./build/color_detect ./http_server 1234
