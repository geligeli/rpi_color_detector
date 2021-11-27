#include <libcamera/libcamera.h>

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

int main(int argc, char *argv[]) {
  try {
    if (!fs::exists("/sys/class/pwm/pwmchip0/pwm0")) {
      std::ofstream("/sys/class/pwm/pwmchip0/export") << "0";
      if (!fs::exists("/sys/class/pwm/pwmchip0/pwm0")) {
        std::cerr << "was not able to export pwm0 device" << std::endl;
        std::terminate();
      }
    }
    std::ofstream("/sys/class/pwm/pwmchip0/pwm0/enable") << "1";
    std::ofstream("/sys/class/pwm/pwmchip0/pwm0/period") << "50000000";
    std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1500000";

    int i{0};
    do {
      std::ifstream("/sys/class/pwm/pwmchip0/pwm0/enable") >> i;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } while (i==0);

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
        std::filesystem::create_directories(key);
        if (buf.save(key + "/" + std::to_string(counter) + ".jpg")) {
          ++counter;
        }
      }
      if (key == "KeyD") {
        std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1700000";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1500000";
      }
      if (key == "KeyA") {
        std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1300000";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1500000";
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
    run_server();
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
