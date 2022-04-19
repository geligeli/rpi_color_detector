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

Classification Classifier::Classify(unsigned char const* data, int h, int w) const {
  TfLiteTensorCopyFromBuffer(inputTensors[0], data, h * w * 3);
  TfLiteInterpreterInvoke(interpreter.get());
  Classification result;
  TfLiteTensorCopyToBuffer(outputTensors[0], result.classification, sizeof(result.classification));
  TfLiteTensorCopyToBuffer(outputTensors[1], result.orientation, sizeof(result.orientation));
  return result;
}

}  // namespace cpp_classifier