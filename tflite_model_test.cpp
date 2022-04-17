#include <chrono>
#include <iostream>
#include <vector>

#include "cpp_classifier/cpp_classifier.h"

int main() {
  cpp_classifier::Classifier c("/nfs/general/shared/adder.tflite");
  std::vector<uint8_t> data(640 * 480 * 3);
  std::cerr << c.Classify(data.data(), 640, 480) << std::endl;

  return 0;
}
