#include <exception>
#include <iostream>

#include <libcamera/libcamera.h>

#include "http_server/http_server.h"
#include "camera_loop/camera_loop.h"

int main(int argc, char *argv[])
{
  try
  {
    CameraLoop loop([](uint8_t *data, const libcamera::StreamConfiguration &config)
                    { std::cout << config.toString() << '\n'; });
    run_server();
  }
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
