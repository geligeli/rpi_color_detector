#include <exception>
#include <iostream>
#include <mutex>

#include <libcamera/libcamera.h>

#include "http_server/http_server.h"
#include "camera_loop/camera_loop.h"

int main(int argc, char *argv[])
{
  try
  {

    std::mutex m;
    ImagePtr img{nullptr, 0, 0};

    OnAquireImage = [&]() -> const ImagePtr
    {
      m.lock();
      return img;
    };
    OnReleaseImage = [&](const ImagePtr &)
    {
      m.unlock();
    };

    CameraLoop loop([&](uint8_t *data, const libcamera::StreamConfiguration &config)
                    {
                      if (!m.try_lock())
                      {
                        return;
                      }
                      img.data = data;
                      img.h = config.size.height;
                      img.w = config.size.width;
                      m.unlock();
                    });
    run_server();
  }
  catch (std::exception const &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
