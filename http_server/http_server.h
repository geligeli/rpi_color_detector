#pragma once

#include <string>
#include <functional>

void run_server();

struct ImagePtr
{
  unsigned char *data;
  int h;
  int w;
};

extern std::function<const ImagePtr()> OnAquireImage;
extern std::function<void(const ImagePtr &)> OnReleaseImage;
