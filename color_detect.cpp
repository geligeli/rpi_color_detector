#include <libcamera/libcamera.h>
#include <pigpio.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

#include "camera_loop/camera_loop.h"
#include "cpp_classifier/cpp_classifier.h"
#include "http_server/http_server.h"
#include "stepper_thread/stepper_thread.h"

struct JpegBuffer {
 public:
  void store(const std::string &s) {
    std::lock_guard<std::mutex> lk(m);
    img = s;
  }
  bool save(const std::string &fn) {
    std::lock_guard<std::mutex> lk(m);
    if (img.empty()) {
      return false;
    }
    FILE *fp = std::fopen(fn.c_str(), "w");
    if (!fp) {
      return false;
    }
    if (std::fwrite(&img[0], img.size(), 1, fp) != 1) {
      std::fclose(fp);
      return false;
    }
    std::fclose(fp);
    return true;
  }

 private:
  std::string img;
  std::mutex m;
};

int main(int argc, char *argv[]) {
  cpp_classifier::Classifier classifier("/nfs/general/shared/adder.tflite");
  try {
    std::mutex m;
    ImagePtr img{nullptr, 0, 0};

    JpegBuffer buf;

    OnAquireImage = [&]() -> const ImagePtr {
      m.lock();
      return img;
    };

    stepper_thread::StepperThread stepper_thread([&]() {
      std::lock_guard<std::mutex> l(m);
      return img.classification;
    });

    OnReleaseImage = [&](const ImagePtr &) { m.unlock(); };

    OnImageCompressed = [&](const std::string &s) { buf.store(s); };

    OnKeyPress = [&](const std::string &key) {
      if (key == "KeyD" || key == "KeyA") {
        const auto outDir = std::filesystem::path("/nfs/general/shared") / key;
        std::filesystem::create_directories(outDir);
        const auto msSinceEpoch =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        buf.save((outDir / (std::to_string(msSinceEpoch) + ".jpg")));
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
      }
    };

    CameraLoop loop(
        [&](uint8_t *data, const libcamera::StreamConfiguration &config) {
          if (!m.try_lock()) {
            return;
          }
          img.data = data;
          img.h = config.size.height;
          img.w = config.size.width;
          img.classification = classifier.Classify(img.data, img.h, img.w);
          m.unlock();
        });
    if (argc > 2) {
      run_server(argv[1], std::atoi(argv[2]));
    }
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
