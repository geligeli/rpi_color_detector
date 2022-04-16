#pragma once

#include <string>
#include <memory>
#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

namespace cpp_classifier {

struct Classifier {
  Classifier(const std::string& fn);
  float Classify(unsigned char const * data,int h,int w) const;
  ~Classifier();
  private:
  // std::unique_ptr<TfLiteInterpreter,  void(*)(TfLiteInterpreter*)> interpreter {nullptr, TfLiteInterpreterDelete};
  // std::unique_ptr<TfLiteInterpreterOptions,  void(*)(TfLiteInterpreterOptions*)> options{nullptr, TfLiteInterpreterOptionsDelete};
  // std::unique_ptr<TfLiteModel, void(*)(TfLiteModel*)> model{nullptr, TfLiteModelDelete};

  TfLiteInterpreter* interpreter;
  TfLiteInterpreterOptions* options;
  TfLiteModel* model;

  // TfLiteTensor *inputTensor{};
  // const TfLiteTensor *outputTensor{};
};

}  // namespace cpp_classifier