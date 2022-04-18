#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR
git pull
cd build
cmake ..
make tflite_model_test
./tflite_model_test /nfs/general/shared/color_classifier.tflite


