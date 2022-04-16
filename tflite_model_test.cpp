
#include <iostream>
#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

int main() {
  int numThreads = 4;

  TfLiteModel *model = TfLiteModelCreateFromFile("/nfs/general/shared/adder.tflite");

  TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate();
  TfLiteInterpreterOptionsSetNumThreads(options, numThreads);
  TfLiteInterpreter *interpreter = TfLiteInterpreterCreate(model, options);

  TfLiteInterpreterAllocateTensors(interpreter);

  TfLiteTensor *inputTensor = TfLiteInterpreterGetInputTensor(interpreter, 0);

  std::cout << TfLiteTypeGetName(inputTensor->type) << std::endl;
  std::cout << inputTensor->name << std::endl;
  std::cout << inputTensor->dims->size << std::endl;
  float x[] = {15.0f};
  TfLiteTensorCopyFromBuffer(inputTensor, x, sizeof(x));

  TfLiteInterpreterInvoke(interpreter);

  float y[1];

  const TfLiteTensor *outputTensor =
      TfLiteInterpreterGetOutputTensor(interpreter, 0);
  TfLiteTensorCopyToBuffer(outputTensor, y, sizeof(y));

  printf("%.4f\n", y[0]);

  TfLiteInterpreterDelete(interpreter);
  TfLiteInterpreterOptionsDelete(options);
  TfLiteModelDelete(model);

  return 0;
}
