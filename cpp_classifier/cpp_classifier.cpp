#include "cpp_classifier/cpp_classifier.h"

#include <array>
#include <iostream>

#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

namespace cpp_classifier {

void Classifier::LoadFromFile(const std::string& fn) {
  model = {TfLiteModelCreateFromFile(fn.c_str()), TfLiteModelDelete}; // "/nfs/general/shared/adder.tflite");
  options = {TfLiteInterpreterOptionsCreate(), TfLiteInterpreterOptionsDelete};
  TfLiteInterpreterOptionsSetNumThreads(options.get(), 4);
  interpreter = {TfLiteInterpreterCreate(model.get(), options.get()), TfLiteInterpreterDelete};

  // TfLiteInterpreterAllocateTensors(interpreter);

  inputTensor = TfLiteInterpreterGetInputTensor(interpreter.get(), 0);
  outputTensor = TfLiteInterpreterGetOutputTensor(interpreter.get(), 0);

  std::cout << "Input tensor type="
  std::cout << TfLiteTypeGetName(inputTensor->type) << std::endl;
  for (int i = 0; i < inputTensor->dims->size; ++i) {
    std::cout << "dim[" << i << "]=" << inputTensor->dims->data[i] << std::endl;
  }

  // const TfLiteTensor *outputTensor =
  // TfLiteInterpreterGetOutputTensor(interpreter, 0);

  std::cout << "Output tensor type="
  std::cout << TfLiteTypeGetName(outputTensor->type) << std::endl;
  for (int i = 0; i < outputTensor->dims->size; ++i) {
    std::cout << "dim[" << i << "]=" << outputTensor->dims->data[i] << std::endl;
  }

    // TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));


  // for (int j = 0; j < 5; ++j) {
  //   const auto start = std::chrono::system_clock::now();
  //   float x[] = {1.0f*j};
  //   TfLiteTensorCopyFromBuffer(inputTensor, x, sizeof(x));
  //   TfLiteInterpreterInvoke(interpreter);
  //   // float y[1];
  //   // const TfLiteTensor *outputTensor =
  //   // TfLiteInterpreterGetOutputTensor(interpreter, 0);
  //   // TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));
    
  //   const auto stop =  std::chrono::system_clock::now();
  //   auto numMs=std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count();
  //   printf("%lldms\n",  numMs);
  // }
  


  // TfLiteInterpreterDelete(interpreter);
  // TfLiteInterpreterOptionsDelete(options);
  // TfLiteModelDelete(model);


}

float Classifier::Classify(unsigned char const* data, int h, int w) const {
  TfLiteTensorCopyFromBuffer(inputTensor, data, h*w*3);
  TfLiteInterpreterInvoke(interpreter.get());
  float y[2];
  TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));
  return y[0];

  // float r = intercept;
  // for (const auto& c : coefs) {
  //   r += data[c.x*3*w + c.y*3 + c.c]*c.coef;
  // }
  // return r;
}

}  // namespace cpp_classifier