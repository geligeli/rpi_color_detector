#include <libcamera/libcamera.h>
#include <pigpio.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

#include "camera_loop/camera_loop.h"
#include "http_server/http_server.h"
#include "cpp_classifier/cpp_classifier.h"

namespace fs = std::filesystem;
using namespace std::literals::chrono_literals;

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

/**
 * 360 degrees = 1600 steps. Holes are 18 degrees apart.
 * 2 steps = 1.8 deg.
 */
void Step(int v, std::chrono::microseconds stepTime = std::chrono::microseconds{
                     20000}) {
  std::cerr << v << " " << stepTime.count() << std::endl;
  if (v < 0) {
    gpioWrite(24, 1);
  } else {
    gpioWrite(24, 0);
  }

  for (int i = 0; i < std::abs(v); ++i) {
    gpioWrite(23, 1);
    std::this_thread::sleep_for(stepTime / 2);
    gpioWrite(23, 0);
    std::this_thread::sleep_for(stepTime / 2);
  }
}

int main(int argc, char *argv[]) {
  cpp_classifier::Classifier classifier("/nfs/general/shared/adder.tflite")
  try {
    // Note this binds to port 8888!!
    if (gpioInitialise() < 0) {
      // pigpio initialisation failed.
      std::cerr << "pigpio initialisation failed.\n";
      return EXIT_FAILURE;
    }
    gpioSetMode(23, PI_OUTPUT);  // Set GPIO17 as input.
    gpioSetMode(24, PI_OUTPUT);  // Set GPIO18 as output.
    gpioWrite(23, 0);
    gpioWrite(24, 0);

    std::atomic<bool> spillThreadRunning{true};

    auto spill = [&spillThreadRunning]() {
      while (spillThreadRunning.load()) {
        Step(-1, 20ms);
        // std::this_thread::sleep_for(400ms);
        // Step(89, 2ms);
        // std::this_thread::sleep_for(400ms);
        // Step(200, 2ms);
        // Step(-200, 2ms);
      }
    };

    std::thread spillThread;

    std::mutex m;
    ImagePtr img{nullptr, 0, 0};

    JpegBuffer buf;

    OnAquireImage = [&]() -> const ImagePtr {
      m.lock();
      std::cout << classifier.Classify(img.data, img.h, img.w) << '\n';
      return img;
    };
    OnReleaseImage = [&](const ImagePtr &) { m.unlock(); };

    OnImageCompressed = [&](const std::string &s) { buf.store(s); };

    OnKeyPress = [&](const std::string &key) mutable {
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
        Step(-80, 4ms);
        Step(-10, 16ms);
        Step(20, 16ms);
        Step(-10, 16ms);
      } else if (key == "KeyA") {
        Step(80, 4ms);
        Step(10, 16ms);
        Step(-20, 16ms);
        Step(10, 16ms);
      } else if (key == "KeyE") {
        Step(-1);
      } else if (key == "KeyQ") {
        Step(1);
      } else if (key == "KeyF") {
        spillThreadRunning = true;
        if (!spillThread.joinable()) {
          spillThread = std::thread(spill);
        }
      } else if (key == "KeyG") {
        spillThreadRunning = false;
        if (spillThread.joinable()) {
          spillThread.join();
        }
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
