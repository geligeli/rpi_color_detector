#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "cpp_classifier/cpp_classifier.h"


namespace image_task {

class ImageTask {
 public:
  ImageTask(const cpp_classifier::Classifier& classifier)
      : m_classifier(classifierFun) {}
  void CaptureImage(uint8_t const* img_data, int h, int w);
  std::string getJpeg();
  cpp_classifier::Classifier::Classification getClassification();
  void dumpJpegFile(const std::string& fn);
  std::string AsDataUrl();
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
  const cpp_classifier::Classifier& m_classifier;
  std::atomic<bool> m_accept_capture_requests{true};
  std::mutex m_mutex{};
  std::condition_variable m_cv{};
  std::vector<uint8_t> m_data{};
  int m_height{};
  int m_width{};
  std::optional<std::string> m_jpeg_representation{};
  std::optional<cpp_classifier::Classifier::Classification> m_classification{};
};

}  // namespace image_task