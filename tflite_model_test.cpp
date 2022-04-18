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
  for (int i =0; i < 10; ++i) { 
    const auto start = std::chrono::system_clock::now();
    c.Classify(data.data(), 640, 480);
    // std::cerr << c.Classify(data.data(), 640, 480) << std::endl;
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()-start).count() << "ms\n";
  }

  return 0;
}
