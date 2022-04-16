#pragma once

#include <filesystem>
#include <functional>
#include <string>

void run_server(std::filesystem::path root, unsigned short port = 8888);

struct ImagePtr {
  unsigned char *data;
  int h;
  int w;
};

extern std::function<const ImagePtr()> OnAquireImage;
extern std::function<void(const ImagePtr &)> OnReleaseImage;
extern std::function<void(const std::string &)> OnImageCompressed;
extern std::function<void(std::string)> OnKeyPress;
