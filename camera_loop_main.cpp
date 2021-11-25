#include <exception>
#include <iostream>

#include <libcamera/libcamera.h>
#include <thread>
#include "camera_loop/camera_loop.h"

int main(int argc, char *argv[])
{
  try
  {
    CameraLoop loop([](uint8_t *data, const libcamera::StreamConfiguration &config)
                    { std::cout << config.toString() << '\n'; });
    std::this_thread::sleep_for(std::chrono::seconds(100));
  }
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}