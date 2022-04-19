#pragma once

#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

#include <memory>
#include <string>

namespace cpp_classifier {

struct Classifier {
  Classifier(const std::string &fn);
  float Classify(unsigned char const *data, int h, int w) const;
  void PrintDebugInfo() const;

 private:
  std::unique_ptr<TfLiteInterpreter, void (*)(TfLiteInterpreter *)> interpreter{
      nullptr, TfLiteInterpreterDelete};
  std::unique_ptr<TfLiteInterpreterOptions,
                  void (*)(TfLiteInterpreterOptions *)>
      options{nullptr, TfLiteInterpreterOptionsDelete};
  std::unique_ptr<TfLiteModel, void (*)(TfLiteModel *)> model{
      nullptr, TfLiteModelDelete};

  TfLiteTensor *inputTensor;
  const TfLiteTensor *outputTensor{};
};

}  // namespace cpp_classifier