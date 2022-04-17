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

  inputTensor = TfLiteInterpreterGetInputTensor(interpreter.get(), 0);
  outputTensor = TfLiteInterpreterGetOutputTensor(interpreter.get(), 0);

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

float Classifier::Classify(unsigned char const* data, int h, int w) const {
  TfLiteTensorCopyFromBuffer(inputTensor, data, h * w * 3);
  TfLiteInterpreterInvoke(interpreter.get());
  float y[2];
  TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));
  // std::cerr << y[0] << "," << y[1] << std::endl;
  return y[0];
}

}  // namespace cpp_classifier