#pragma once

#include <math.h>
#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace cpp_classifier {

struct Classifier {
  Classifier(const std::string &fn);

  class Classification {
   public:
    std::array<float, 7> classification;
    // std::array<float, 2> orientation;

    static constexpr std::array names = {"black",  "blue",  "green", "pink",
                                         "salmon", "white", "yellow"};

    char const *predictedClass() const {
      return names[std::max_element(std::begin(classification),
                                    std::end(classification)) -
                   std::begin(classification)];
      // return classification[0];
    }
    // double angle() const {
    //   return atan2(orientation[1], orientation[0]) * 180 / 3.14159265;
    // }
    friend std::ostream &operator<<(std::ostream &os, const Classification &c) {
      for (size_t i{}; i < c.classification.size(); ++i) {
        os << c.classification[i];
        if (i + 1 != c.classification.size()) {
          os << '\t';
        }
      }
      os << '\n';
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