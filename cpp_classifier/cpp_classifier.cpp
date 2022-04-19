#include "cpp_classifier/cpp_classifier.h"

#include <math.h>
#include <iostream>

namespace cpp_classifier {

Classifier::Classifier(const std::string& fn) {
  model = {TfLiteModelCreateFromFile(fn.c_str()), TfLiteModelDelete};
  options = {TfLiteInterpreterOptionsCreate(), TfLiteInterpreterOptionsDelete};
  TfLiteInterpreterOptionsSetNumThreads(options.get(), 4);
  interpreter = {TfLiteInterpreterCreate(model.get(), options.get()),
                 TfLiteInterpreterDelete};
  TfLiteInterpreterAllocateTensors(interpreter.get());

  const auto numInputTensors =
      TfLiteInterpreterGetInputTensorCount(interpreter.get());
  for (int i = 0; i < numInputTensors; ++i) {
    inputTensors.push_back(
        TfLiteInterpreterGetInputTensor(interpreter.get(), i));
  }

  const auto numOutputTensors =
      TfLiteInterpreterGetOutputTensorCount(interpreter.get());
  for (int i = 0; i < numOutputTensors; ++i) {
    outputTensors.push_back(
        TfLiteInterpreterGetOutputTensor(interpreter.get(), i));
  }
}

void Classifier::PrintDebugInfo() const {
  int index{0};
  for (const auto* inputTensor : inputTensors) {
    std::cout << "Input tensor [" << index++
              << "] type=" << TfLiteTypeGetName(inputTensor->type) << '\n';
    for (int i = 0; i < inputTensor->dims->size; ++i) {
      std::cout << "dim[" << i << "]=" << inputTensor->dims->data[i] << '\n';
    }
  }
  for (const auto* outputTensor : outputTensors) {
    std::cout << "Output tensor [" << index++
              << "] type=" << TfLiteTypeGetName(outputTensor->type) << '\n';
    for (int i = 0; i < outputTensor->dims->size; ++i) {
      std::cout << "dim[" << i << "]=" << outputTensor->dims->data[i] << '\n';
    }
  }
}

float Classifier::Classify(unsigned char const* data, int h, int w) const {
  TfLiteTensorCopyFromBuffer(inputTensors[0], data, h * w * 3);
  TfLiteInterpreterInvoke(interpreter.get());
  float classification[2];
  float orientation[2];
  TfLiteTensorCopyToBuffer(outputTensors[0], classification, sizeof(classification));
  TfLiteTensorCopyToBuffer(outputTensors[1], orientation, sizeof(orientation));
  double probability = 1.0 / (1.0 + exp(classification[1]-classification[0]));
  std::cerr << classification[0] << '\t' << classification[1] << '\t' << probability << '\t' << atan2(orientation[1],orientation[0]) * 180 /  3.14159265 << '\n';
  return classification[0];
}

}  // namespace cpp_classifier