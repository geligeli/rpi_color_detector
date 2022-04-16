#include <pigpio.h>
// #include <filesystem>
// #include <fstream>
#include <chrono>
#include <iostream>
#include <thread>
// #include <thread>

// namespace fs = std::filesystem;

int main(int argc, char **argv) {
  using namespace std::literals::chrono_literals;

  if (gpioInitialise() < 0) {
    // pigpio initialisation failed.
    std::cerr << "asdf";
    std::terminate();
  }

  gpioSetMode(14, PI_OUTPUT);  // Set GPIO17 as input.
  gpioSetMode(15, PI_OUTPUT);  // Set GPIO18 as output.
  gpioSetMode(18, PI_OUTPUT);  // Set GPIO18 as output.

  gpioWrite(14, 0);
  gpioWrite(15, 0);
  gpioWrite(18, 0);

  for (int i = 0; i < 1000; ++i) {
    gpioWrite(14, 1);
    std::this_thread::sleep_for(10ms);
    gpioWrite(14, 0);
    std::this_thread::sleep_for(10ms);
  }

  gpioTerminate();
}