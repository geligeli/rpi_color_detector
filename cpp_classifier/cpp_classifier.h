#pragma once

#include <math.h>
#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

namespace cpp_classifier {

struct Classifier {
  Classifier(const std::string &fn);

  class Classification {
   public:
    std::array<float, 2> classification;
    std::array<float, 2> orientation;

    double prob() const {
      return classification[0];
    }
    double angle() const {
      return atan2(orientation[1], orientation[0]) * 180 / 3.14159265;
    }
    friend std::ostream &operator<<(std::ostream &os, const Classification &c) {
      os << c.classification[0] << '\t' << c.classification[1] << '\t'
         << c.orientation[0] << '\t' << c.orientation[1] << '\n';
      return os;
    }
  };
  Classification Classify(unsigned char const *data, int h, int w) const;
  void PrintDebugInfo() const;

 private:
  mutable std::mutex m_mutex{};
  std::unique_ptr<TfLiteInterpreter, void (*)(TfLiteInterpreter *)> interpreter{
      nullptr, TfLiteInterpreterDelete};
  std::unique_ptr<TfLiteInterpreterOptions,
                  void (*)(TfLiteInterpreterOptions *)>
      options{nullptr, TfLiteInterpreterOptionsDelete};
  std::unique_ptr<TfLiteModel, void (*)(TfLiteModel *)> model{
      nullptr, TfLiteModelDelete};

  std::vector<TfLiteTensor *> inputTensors{};
  std::vector<TfLiteTensor const *> outputTensors{};
};

}  // namespace cpp_classifier