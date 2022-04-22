#include <libcamera/libcamera.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "camera_loop/camera_loop.h"
#include "cpp_classifier/cpp_classifier.h"
#include "http_server/http_server.h"
#include "image_task/image_task.h"
#include "stepper_thread/stepper_thread.h"

int main(int argc, char *argv[]) {
  cpp_classifier::Classifier classifier(
      "/nfs/general/shared/tflite/fused_model.tflite");
  image_task::ImageTask imgTask(classifier);
  OnProvideImageJpeg = [&](std::ostream&os) {
    auto capture = imgTask.getCurrentCapture();
    os << capture->getJpeg();
  };
  OnProvideJson = [&](std::ostream&os) {
    auto capture = imgTask.getCurrentCapture();
    capture->dumpJson(os);
  };
  stepper_thread::StepperThread stepper_thread(imgTask);
  OnKeyPress = [&](const std::string &key) {
    if (key == "KeyD" || key == "KeyA") {
      const auto outDir = std::filesystem::path("/nfs/general/shared") / key;
      std::filesystem::create_directories(outDir);
      const auto msSinceEpoch =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();
      imgTask.getCurrentCapture()->dumpJpegFile((outDir / (std::to_string(msSinceEpoch) + ".jpg")));
    }
    if (key == "KeyD") {
      stepper_thread.KeyD();
    } else if (key == "KeyA") {
      stepper_thread.KeyA();
    } else if (key == "KeyE") {
      stepper_thread.KeyE();
    } else if (key == "KeyQ") {
      stepper_thread.KeyQ();
    } else if (key == "KeyF") {
      stepper_thread.Spill();
    } else if (key == "KeyG") {
      stepper_thread.Stop();
    } else if (key == "KeyC") {
      stepper_thread.AutoSort();
    } else if (key == "KeyT") {
      stepper_thread.RecordPositionTrainingData();
    }
  };
  try {
    CameraLoop loop(
        [&](uint8_t *data, const libcamera::StreamConfiguration &config) {
          imgTask.CaptureImage(data, config.size.height, config.size.width);
          // if (!m.try_lock()) {
          //   return;
          // }
          // img.data = data;
          // img.h = config.size.height;
          // img.w = config.size.width;
          // img.classification = classifier.Classify(img.data, img.h, img.w);
          // m.unlock();
        });
    if (argc > 2) {
      run_server(argv[1], std::atoi(argv[2]));
    }
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
