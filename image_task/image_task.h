#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "cpp_classifier/cpp_classifier.h"

namespace image_task {

class ImageTask {
 public:
  ImageTask(const cpp_classifier::Classifier& classifier)
      : m_classifier(classifier) {}
  void CaptureImage(uint8_t const* img_data, int h, int w);

  struct RawImage {
    std::vector<uint8_t> m_data{};
    int m_height{};
    int m_width{};
  };

  class Capture {
   public:
    Capture() = delete;
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;
    Capture(Capture&&) = delete;
    Capture& operator=(Capture&&) = delete;

    const std::string& getJpeg();
    void dumpJson(std::ostream& os);
    cpp_classifier::Classifier::Classification getClassification();
    void dumpJpegFile(const std::string& fn);

   private:
    Capture(ImageTask::RawImage&& underlying_raw_image, ImageTask* parent);
    RawImage m_raw_image;
    ImageTask* m_parent;
    std::mutex m_jpeg_mutex{};
    std::mutex m_classification_mutex{};
    std::optional<std::string> m_jpeg_representation{};
    std::optional<cpp_classifier::Classifier::Classification>
        m_classification{};
    friend class ImageTask;
  };

  std::shared_ptr<Capture> getCurrentCapture();
  std::shared_ptr<Capture> getNextCapture();

 private:
  const cpp_classifier::Classifier& m_classifier;
  std::mutex m_mutex{};
  std::condition_variable m_cv;
  std::array<RawImage, 2> m_double_buffered_raw_image;
  RawImage* m_image_for_capture{&m_double_buffered_raw_image[0]};
  RawImage* m_last_image_for_capture{&m_double_buffered_raw_image[1]};
  std::shared_ptr<Capture> m_current_capture;

  friend class Capture;
};

}  // namespace image_task