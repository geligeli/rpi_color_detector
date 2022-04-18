#include <chrono>
#include <iostream>
#include <vector>

#include "cpp_classifier/cpp_classifier.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "usage: tflite_model_test modelfile.tflite\n";
    return 1;
  }
  cpp_classifier::Classifier c(argv[1]);
  
  std::vector<uint8_t> data(640 * 480 * 3);
  std::cerr << c.Classify(data.data(), 640, 480) << std::endl;

  return 0;
}
