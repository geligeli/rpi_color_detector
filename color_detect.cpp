#include <libcamera/libcamera.h>
#include <pigpio.h>

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

namespace fs = std::filesystem;

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


void Step(int v) {
  if (v < 0) {
    gpioWrite(24, 1);
  } else {
    gpioWrite(24, 0);
  }

  using namespace std::literals::chrono_literals;

  for (int i = 0; i < std::abs(v); ++i)
  {
    gpioWrite(23, 1);
    std::this_thread::sleep_for(10ms);
    gpioWrite(23, 0);
    std::this_thread::sleep_for(10ms);
  }
}



int main(int argc, char *argv[]) {
  try {
    if (gpioInitialise() < 0)
    {
      // pigpio initialisation failed.
      std::cerr << "pigpio initialisation failed.\n";
      return EXIT_FAILURE;
    }
    gpioSetMode(23, PI_OUTPUT); // Set GPIO17 as input.
    gpioSetMode(24, PI_OUTPUT); // Set GPIO18 as output.
    gpioWrite(23, 0);
    gpioWrite(24, 0);

    std::mutex m;
    ImagePtr img{nullptr, 0, 0};

    JpegBuffer buf;

    OnAquireImage = [&]() -> const ImagePtr {
      m.lock();
      return img;
    };
    OnReleaseImage = [&](const ImagePtr &) { m.unlock(); };

    OnImageCompressed = [&](const std::string &s) { buf.store(s); };

    OnKeyPress = [&, counter = 0](const std::string &key) mutable {
      std::cerr<<key;
      if (key == "KeyD" || key == "KeyA") {
        // std::filesystem::create_directories(key);
        // if (buf.save(key + "/" + std::to_string(counter) + ".jpg")) {
        //   ++counter;
        // }
      }
      if (key == "KeyD") {
        Step(-1);

        // std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1700000";
        // std::this_thread::sleep_for(std::chrono::milliseconds(800));
        // std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1500000";
      }
      if (key == "KeyA") {
        Step(1);

        // std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1300000";
        // std::this_thread::sleep_for(std::chrono::milliseconds(800));
        // std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1500000";
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
