#!/bin/bash
set -e
pushd build
make color_detect
popd
sudo  ./build/color_detect ./http_server 1234
