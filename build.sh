#!/bin/bash
sudo killall color_detect
set -e
cd build
cmake ..
make color_detect
