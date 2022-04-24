#!/bin/bash
sudo killall color_detect
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR
set -e
pushd build
cmake ..
make color_detect
popd
sudo COLOR=${COLOR} ./build/color_detect ./http_server 1234
