#pragma once

#include <atomic>
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
  void dumpJpegFile(const std::string& fn);
  void WaitForNewCapture();

  class RAIIRenableWrapper {
   public:
    RAIIRenableWrapper(RAIIRenableWrapper&&) = default;
    RAIIRenableWrapper(const RAIIRenableWrapper&) = delete;
    RAIIRenableWrapper& operator=(RAIIRenableWrapper&&) = default;
    RAIIRenableWrapper& operator=(const RAIIRenableWrapper&) = delete;
    ~RAIIRenableWrapper();

   private:
    RAIIRenableWrapper(ImageTask* image_task_ptr);
    ImageTask* m_image_task_ptr;
    friend class ImageTask;
  };
  RAIIRenableWrapper suspendCapture();

 private:
  std::atomic<bool> m_accept_capture_requests{true};
  std::mutex m_mutex{};
  std::condition_variable m_cv{};
  std::function<float(uint8_t const* img_data, int h, int w)> m_classifierFun{};
  std::vector<uint8_t> m_data{};
  int m_height{};
  int m_width{};
  std::optional<std::string> m_jpeg_representation{};
  std::optional<float> m_classification{};
};

}  // namespace image_task