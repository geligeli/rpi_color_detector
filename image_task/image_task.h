#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace image_task {

class ImageTask {
 public:
  ImageTask(
      std::function<float(uint8_t const* img_data, int h, int w)> classifierFun)
      : m_classifierFun(classifierFun) {}
  void CaptureImage(uint8_t const* img_data, int h, int w);
  std::string getJpeg();
  float getClassification();

 private:
  std::mutex m_mutex{};
  std::function<float(uint8_t const* img_data, int h, int w)> m_classifierFun{};
  std::vector<uint8_t> m_data{};
  int m_height{};
  int m_width{};
  std::optional<std::string> m_jpeg_representation{};
  std::optional<float> m_classification{};
};

}  // namespace image_task