#include "cpp_classifier/cpp_classifier.h"

#include <vector>
#include <iostream>

#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

namespace cpp_classifier {

Classifier::Classifier(const std::string& fn) {
  // model = {TfLiteModelCreateFromFile(fn.c_str()), TfLiteModelDelete}; // "/nfs/general/shared/adder.tflite");
  // options = {TfLiteInterpreterOptionsCreate(), TfLiteInterpreterOptionsDelete};
  // TfLiteInterpreterOptionsSetNumThreads(options.get(), 4);
  // interpreter = {TfLiteInterpreterCreate(model.get(), options.get()), TfLiteInterpreterDelete};

  // auto* inputTensor = TfLiteInterpreterGetInputTensor(interpreter.get(), 0);
  // const auto* outputTensor = TfLiteInterpreterGetOutputTensor(interpreter.get(), 0);

  model = TfLiteModelCreateFromFile(fn.c_str());
  options = TfLiteInterpreterOptionsCreate();
  TfLiteInterpreterOptionsSetNumThreads(options, 2);
  interpreter = TfLiteInterpreterCreate(model, options);
  inputTensor = TfLiteInterpreterGetInputTensor(interpreter, 0);
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
    std::cerr << "dim[" << i << "]=" << outputTensor->dims->data[i] << std::endl;
  }*/
}

float Classifier::Classify(unsigned char const* data, int h, int w) const {
  // auto* interpreter = TfLiteInterpreterCreate(model, options);

  // auto* inputTensor = TfLiteInterpreterGetInputTensor(interpreter, 0);
  // const auto* outputTensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);


  // std::cerr << reinterpret_cast<uintptr_t>(inputTensor) << std::endl;
  std::cerr << "copy data" << std::endl;
  std::cerr << "Input tensor type=";
  std::cerr << TfLiteTypeGetName(inputTensor->type) << std::endl;
  for (int i = 0; i < inputTensor->dims->size; ++i) {
    std::cerr << "dim[" << i << "]=" << inputTensor->dims->data[i] << std::endl;
  }
  std::cerr << h << " " << w << " " << 3 << std::endl;

  std::vector<unsigned char> foo(h*w*3);
  TfLiteTensorCopyFromBuffer(inputTensor, foo.data(), h*w*3);

  // std::cerr << "call interpreter" << std::endl;
  // TfLiteInterpreterInvoke(interpreter.get());
  // float y[2];
  // std::cerr << "get result" << std::endl;
  // TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));

  // std::cerr << "done result" << std::endl;
  // return y[0];

  // float r = intercept;
  // for (const auto& c : coefs) {
  //   r += data[c.x*3*w + c.y*3 + c.c]*c.coef;
  // }
  // return r;
  return 0;
}

Classifier::~Classifier() {
  std::cerr << "booooooooooooooom" << std::endl;
}

}  // namespace cpp_classifier