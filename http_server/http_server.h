#pragma once

#include <filesystem>
#include <functional>
#include <string>

void run_server(std::filesystem::path root, unsigned short port = 8888);

struct ImagePtr {
  unsigned char *data;
  int h;
  int w;
  float classification;
};

extern std::function<const ImagePtr()> OnAquireImage;
extern std::function<void(const ImagePtr &)> OnReleaseImage;

// 
extern std::function<void(const std::string &)> OnImageCompressed;

// User presses a key on UI callback.
extern std::function<void(std::string)> OnKeyPress;
