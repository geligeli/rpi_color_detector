#pragma once

#include <string>
#include <memory>
#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

namespace cpp_classifier {

struct Classifier {
  std::unique_ptr<TfLiteInterpreter,  void(*)(TfLiteInterpreter*)> interpreter;
  std::unique_ptr<TfLiteInterpreterOptions,  void(*)(TfLiteInterpreterOptions*)> options;
  std::unique_ptr<TfLiteModel, void(*)(TfLiteModel*)> model;

  TfLiteTensor *inputTensor{};
  const TfLiteTensor *outputTensor{};


  // TfLiteInterpreterDelete(interpreter);
  // TfLiteInterpreterOptionsDelete(options);
  // TfLiteModelDelete(model);

  // TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate();
  // TfLiteInterpreterOptionsSetNumThreads(options, numThreads);
  // TfLiteInterpreter *interpreter = TfLiteInterpreterCreate(model, options);

  // TfLiteInterpreterAllocateTensors(interpreter);

  // TfLiteTensor *inputTensor = TfLiteInterpreterGetInputTensor(interpreter, 0);

  // std::cout << TfLiteTypeGetName(inputTensor->type) << std::endl;


  void LoadFromFile(const std::string& fn);

  float Classify(unsigned char const * data,int h,int w) const;
};

}  // namespace cpp_classifier