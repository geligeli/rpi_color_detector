#!/bin/bash
pushd build
make color_detect
popd
sudo -e trace=network ./build/color_detect ./http_server 1234
