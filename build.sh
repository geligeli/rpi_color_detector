#!/bin/bash
sudo killall color_detect
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR
git pull
set -e
cd build
cmake ..
make color_detect
