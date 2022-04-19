#include "cpp_classifier/cpp_classifier.h"

#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

#include <iostream>
#include <vector>

namespace cpp_classifier {

Classifier::Classifier(const std::string& fn) {
  model = {TfLiteModelCreateFromFile(fn.c_str()),
           TfLiteModelDelete};  // "/nfs/general/shared/adder.tflite");
  options = {TfLiteInterpreterOptionsCreate(), TfLiteInterpreterOptionsDelete};
  TfLiteInterpreterOptionsSetNumThreads(options.get(), 4);
  interpreter = {TfLiteInterpreterCreate(model.get(), options.get()),
                 TfLiteInterpreterDelete};
  TfLiteInterpreterAllocateTensors(interpreter.get());
  
  
  const auto numInputTensors = TfLiteInterpreterGetInputTensorCount(interpreter.get());
  for (int i = 0; i < numInputTensors; ++i) {
    inputTensors.push_back(TfLiteInterpreterGetInputTensor(interpreter.get(), i));
  }

  const auto numOutputTensors = TfLiteInterpreterGetOutputTensorCount(interpreter.get());
  for (int i = 0; i < numOutputTensors; ++i) {
    outputTensors.push_back(TfLiteInterpreterGetOutputTensor(interpreter.get(), i));
  }


  // model = TfLiteModelCreateFromFile(fn.c_str());
  // options = TfLiteInterpreterOptionsCreate();
  // TfLiteInterpreterOptionsSetNumThreads(options, 2);
  // interpreter = TfLiteInterpreterCreate(model, options);

  // inputTensor = TfLiteInterpreterGetInputTensor(interpreter, 0);

  /*const auto* outputTensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);


  std::cerr << "Input tensor type=";
  std::cerr << TfLiteTypeGetName(inputTensor->type) << std::endl;
  for (int i = 0; i < inputTensor->dims->size; ++i) {
    std::cerr << "dim[" << i << "]=" << inputTensor->dims->data[i] << std::endl;
  }
  std::cerr << reinterpret_cast<uintptr_t>(inputTensor) << std::endl;
  std::cerr << "Output tensor type=";
  std::cerr << TfLiteTypeGetName(outputTensor->type) << std::endl;
  for (int i = 0; i < outputTensor->dims->size; ++i) {
    std::cerr << "dim[" << i << "]=" << outputTensor->dims->data[i] <<
  std::endl;
  }*/
}

void Classifier::PrintDebugInfo() const {
  int index{0};
  for (const auto* inputTensor : inputTensors) {
    std::cout << "Input tensor ["<< index++ << "] type=" << TfLiteTypeGetName(inputTensor->type) << '\n';
    for (int i = 0; i < inputTensor->dims->size; ++i) {
      std::cout << "dim[" << i << "]=" << inputTensor->dims->data[i] << '\n';
    }
  }
  for (const auto* outputTensor : outputTensors) {
    std::cout << "Output tensor ["<< index++ << "] type=" << TfLiteTypeGetName(outputTensor->type) << '\n';
    for (int i = 0; i < outputTensor->dims->size; ++i) {
      std::cout << "dim[" << i << "]=" << outputTensor->dims->data[i] << '\n';
    }
  }
}


float Classifier::Classify(unsigned char const* data, int h, int w) const {
  // TfLiteTensorCopyFromBuffer(inputTensor, data, h * w * 3);
  TfLiteInterpreterInvoke(interpreter.get());
  // float y[2];
  // TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));
  // std::cerr << y[0] << "," << y[1] << std::endl;
  return 0; // y[0];
}

}  // namespace cpp_classifier