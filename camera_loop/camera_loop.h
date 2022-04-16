#pragma once

#include <libcamera/libcamera.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

class CameraLoop {
 public:
  explicit CameraLoop(
      std::function<void(uint8_t *data, const libcamera::StreamConfiguration &)>
          callback);
  // int StartLoop();

  ~CameraLoop();

 private:
  void processRequest(libcamera::Request *request);

  std::unordered_map<libcamera::FrameBuffer *,
                     std::vector<libcamera::Span<uint8_t>>>
      mapped_buffers;
  std::shared_ptr<libcamera::Camera> camera;
  std::function<void(uint8_t *data, const libcamera::StreamConfiguration &)>
      OnFrameCallback;
  std::unique_ptr<libcamera::CameraManager> cm;
  std::unique_ptr<libcamera::CameraConfiguration> config;
  std::unique_ptr<libcamera::FrameBufferAllocator> allocator;
  libcamera::Stream *stream;
  std::vector<std::unique_ptr<libcamera::Request>> requests;
};
