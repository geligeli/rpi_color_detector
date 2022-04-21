#pragma once

#include <filesystem>
#include <functional>
#include <ostream>
#include <string>

void run_server(std::filesystem::path root, unsigned short port = 8888);

extern std::function<void(std::ostream&)> OnProvideImageJpeg;
extern std::function<void(std::ostream&)> OnProvideJson;

// User presses a key on UI callback.
extern std::function<void(std::string)> OnKeyPress;
