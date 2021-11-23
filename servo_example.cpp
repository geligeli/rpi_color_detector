#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

namespace fs = std::filesystem;

int main() {
  if (!fs::exists("/sys/class/pwm/pwmchip0/pwm0")) {
    std::ofstream("/sys/class/pwm/pwmchip0/export") << "0";
    if (!fs::exists("/sys/class/pwm/pwmchip0/pwm0")) {
      std::cerr << "was not able to export pwm0 device" << std::endl;
      std::terminate();
    }
  }
  std::ofstream("/sys/class/pwm/pwmchip0/pwm0/enable") << "1";
  std::ofstream("/sys/class/pwm/pwmchip0/pwm0/period") << "50000000";
  // 2ms max
  std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1700000";
  std::this_thread::sleep_for(std::chrono::seconds(1));
  // 2ms max
  std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1500000";
  // std::this_thread::sleep_for(std::chrono::seconds(1));
  // 1ms is min
  // std::ofstream("/sys/class/pwm/pwmchip0/pwm0/duty_cycle") << "1000000";
}