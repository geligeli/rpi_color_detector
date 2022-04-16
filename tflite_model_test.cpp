
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

#include "cpp_classifier/cpp_classifier.h"

int main() {
  cpp_classifier::Classifier c("/nfs/general/shared/adder.tflite");
  std::vector<uint8_t> data(640*480*3);
  std::cerr << c.Classify(data.data(), 640, 480) << std::endl;
  
  /*int numThreads = 4;

  TfLiteModel *model = TfLiteModelCreateFromFile("/nfs/general/shared/adder.tflite"); // "/nfs/general/shared/adder.tflite");

  TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate();
  TfLiteInterpreterOptionsSetNumThreads(options, numThreads);
  TfLiteInterpreter *interpreter = TfLiteInterpreterCreate(model, options);

  TfLiteInterpreterAllocateTensors(interpreter);

  TfLiteTensor *inputTensor = nullptr;
  std::thread t([&](){
    inputTensor = TfLiteInterpreterGetInputTensor(interpreter, 0);
  });

  t.join();

  std::cout << TfLiteTypeGetName(inputTensor->type) << std::endl;
  std::cout << inputTensor->name << std::endl;
  std::cout << inputTensor->dims->size << std::endl;

  for (int i = 0; i < inputTensor->dims->size; ++i) {
    std::cout << inputTensor->dims->data[i] << std::endl;
  }

  const TfLiteTensor *outputTensor =
  TfLiteInterpreterGetOutputTensor(interpreter, 0);

  std::cout << TfLiteTypeGetName(outputTensor->type) << std::endl;
  std::cout << outputTensor->name << std::endl;
  std::cout << outputTensor->dims->size << std::endl;

    // TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));


  for (int j = 0; j < 5; ++j) {
    const auto start = std::chrono::system_clock::now();
    std::vector<uint8_t> data(640*480*3);
    TfLiteTensorCopyFromBuffer(inputTensor, data.data(), data.size());
    TfLiteInterpreterInvoke(interpreter);
    // float y[1];
    // const TfLiteTensor *outputTensor =
    // TfLiteInterpreterGetOutputTensor(interpreter, 0);
    // TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));
    
    const auto stop =  std::chrono::system_clock::now();
    auto numMs=std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count();
    printf("%lldms\n",  numMs);
  }
  


  TfLiteInterpreterDelete(interpreter);
  TfLiteInterpreterOptionsDelete(options);
  TfLiteModelDelete(model);
*/
  return 0;
}
