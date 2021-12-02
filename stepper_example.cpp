#include <pigpio.h>
// #include <filesystem>
// #include <fstream>
#include <iostream>
// #include <thread>

// namespace fs = std::filesystem;

int main()
{

  if (gpioInitialise() < 0)
  {
    // pigpio initialisation failed.
    std::cerr << "asdf";
  }
  else
  {
    // pigpio initialised okay.
  }
}