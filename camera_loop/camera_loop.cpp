#include "camera_loop/camera_loop.h"

#include <assert.h>
#include <libcamera/libcamera.h>
#include <sys/mman.h>

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

std::string cameraName(libcamera::Camera *camera) {
  using namespace libcamera;
  const ControlList &props = camera->properties();
  std::string name;
  switch (props.get(properties::Location)) {
    case properties::CameraLocationFront:
      name = "Internal front camera";
      break;
    case properties::CameraLocationBack:
      name = "Internal back camera";
      break;
    case properties::CameraLocationExternal:
      name = "External camera";
      if (props.contains(properties::Model))
        name += " '" + props.get(properties::Model) + "'";
      break;
  }

  name += " (" + camera->id() + ")";

  return name;
}

CameraLoop::CameraLoop(
    std::function<void(uint8_t *data, const libcamera::StreamConfiguration &)>
        callback)
    : OnFrameCallback(callback),
      cm{std::make_unique<libcamera::CameraManager>()} {
  using namespace libcamera;
  // std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
  cm->start();
  for (auto const &camera : cm->cameras())
    std::cout << " - " << cameraName(camera.get()) << std::endl;
  if (cm->cameras().empty()) {
    std::cout << "No cameras were identified on the system." << std::endl;
    cm->stop();
    std::terminate();
  }

  std::string cameraId = cm->cameras()[0]->id();
  camera = cm->get(cameraId);
  camera->acquire();
  /*std::unique_ptr<CameraConfiguration> */ config =
      camera->generateConfiguration({StreamRole::Viewfinder});
  StreamConfiguration &streamConfig = config->at(0);
  std::cout << "Default viewfinder configuration is: "
            << streamConfig.toString() << std::endl;

  streamConfig.size.width = 640;
  streamConfig.size.height = 480;
  streamConfig.pixelFormat = libcamera::formats::BGR888;
  config->validate();
  std::cout << "Validated viewfinder configuration is: "
            << streamConfig.toString() << std::endl;
  if (camera->configure(config.get())) {
    std::cout << "CONFIGURATION FAILED!" << std::endl;
    std::terminate();
  }
  // FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);
  allocator = std::make_unique<FrameBufferAllocator>(camera);

  for (StreamConfiguration &cfg : *config) {
    int ret = allocator->allocate(cfg.stream());
    if (ret < 0) {
      std::cerr << "Can't allocate buffers" << std::endl;
      std::terminate();
    }

    size_t allocated = allocator->buffers(cfg.stream()).size();
    std::cout << "Allocated " << allocated << " buffers for stream"
              << std::endl;

    for (const std::unique_ptr<FrameBuffer> &buffer :
         allocator->buffers(cfg.stream())) {
      size_t buffer_size = 0;
      for (unsigned i = 0; i < buffer->planes().size(); i++) {
        const FrameBuffer::Plane &plane = buffer->planes()[i];
        buffer_size += plane.length;
        if (i == buffer->planes().size() - 1 ||
            plane.fd.fd() != buffer->planes()[i + 1].fd.fd()) {
          void *memory = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE,
                              MAP_SHARED, plane.fd.fd(), 0);
          mapped_buffers[buffer.get()].push_back(libcamera::Span<uint8_t>(
              static_cast<uint8_t *>(memory), buffer_size));
          buffer_size = 0;
        }
      }
    }
  }
  stream = streamConfig.stream();
  const std::vector<std::unique_ptr<FrameBuffer>> &buffers =
      allocator->buffers(stream);

  for (unsigned int i = 0; i < buffers.size(); ++i) {
    std::unique_ptr<Request> request = camera->createRequest();
    if (!request) {
      std::cerr << "Can't create request" << std::endl;
      std::terminate();
    }

    const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
    int ret = request->addBuffer(stream, buffer.get());
    if (ret < 0) {
      std::cerr << "Can't set buffer for request" << std::endl;
      std::terminate();
    }

    /*
     * Controls can be added to a request on a per frame basis.
     */
    ControlList &controls = request->controls();
    // controls.set(controls::Brightness, 0.5);
    int64_t frame_time = 1000000 / 60;  // in us
    controls.set(controls::FrameDurationLimits, {frame_time, frame_time});
    controls.set(controls::AeEnable, false);
    controls.set(controls::AnalogueGain, 10);

    requests.push_back(std::move(request));
  }
  // auto call = [&](libcamera::Request *request)
  // { processRequest(request); };
  camera->requestCompleted.connect(this, &CameraLoop::processRequest);
  camera->start();
  for (std::unique_ptr<Request> &request : requests)
    camera->queueRequest(request.get());
}

CameraLoop::~CameraLoop() {
  camera->stop();
  allocator->free(stream);
  camera->release();
  camera.reset();
  cm->stop();
}

void CameraLoop::processRequest(libcamera::Request *request) {
  using namespace libcamera;
  if (request->status() == Request::RequestCancelled) {
    std::cout << "aborted" << std::endl;
    return;
  }

  const Request::BufferMap &buffers = request->buffers();

  for (auto bufferPair : buffers) {
    FrameBuffer *buffer = bufferPair.second;

    auto it = mapped_buffers.find(buffer);
    if (it != mapped_buffers.end()) {
      uint8_t *buf = it->second[0].data();

      if (OnFrameCallback) {
        OnFrameCallback(buf, bufferPair.first->configuration());
      }
    };
  }

  /* Re-queue the Request to the camera. */
  request->reuse(Request::ReuseBuffers);
  camera->queueRequest(request);
}
