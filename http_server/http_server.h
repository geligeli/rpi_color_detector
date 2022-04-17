#pragma once

#include <filesystem>
#include <functional>
#include <string>

void run_server(std::filesystem::path root, unsigned short port = 8888);

extern std::function<std::string()> OnProvideImageJpeg;

// User presses a key on UI callback.
extern std::function<void(std::string)> OnKeyPress;
