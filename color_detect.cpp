#include <assert.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <libcamera/libcamera.h>
#include <sys/mman.h>

#include <atomic>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

class EventLoop {
public:
  EventLoop() {
    assert(!instance_);
    evthread_use_pthreads();
    event_ = event_base_new();
    instance_ = this;
  }

  ~EventLoop() {
    instance_ = nullptr;
    event_base_free(event_);
    libevent_global_shutdown();
  }

  void exit(int code = 0) {
    exitCode_ = code;
    exit_.store(true, std::memory_order_release);
    interrupt();
  }
  int exec() {
    exitCode_ = -1;
    exit_.store(false, std::memory_order_release);
    while (!exit_.load(std::memory_order_acquire)) {
      dispatchCalls();
      event_base_loop(event_, EVLOOP_NO_EXIT_ON_EMPTY);
    }
    return exitCode_;
  }

  void timeout(unsigned int sec) {
    struct event *ev;
    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = 0;
    ev = evtimer_new(event_, &timeoutTriggered, this);
    evtimer_add(ev, &tv);
  }

  template <typename F> void callLater(F &&func) {
    {
      std::unique_lock<std::mutex> locker(lock_);
      calls_.emplace_back(std::forward<F>(func));
    }

    interrupt();
  }

private:
  static EventLoop *instance_;

  static void timeoutTriggered(int fd, short event, void *arg) {
    EventLoop *self = static_cast<EventLoop *>(arg);
    self->exit();
  }

  struct event_base *event_;
  std::atomic<bool> exit_;
  int exitCode_;

  std::list<std::function<void()>> calls_;
  std::mutex lock_;

  void interrupt() { event_base_loopbreak(event_); }

  void dispatchCalls() {
    std::unique_lock<std::mutex> locker(lock_);

    for (auto iter = calls_.begin(); iter != calls_.end();) {
      std::function<void()> call = std::move(*iter);

      iter = calls_.erase(iter);

      locker.unlock();
      call();
      locker.lock();
    }
  }
};

EventLoop *EventLoop::instance_ = nullptr;
static constexpr int TIMEOUT_SEC = 3;
using namespace libcamera;
static std::shared_ptr<Camera> camera;
static EventLoop loop;
std::unordered_map<libcamera::FrameBuffer *,
                   std::vector<libcamera::Span<uint8_t>>>
    mapped_buffers;

std::chrono::high_resolution_clock::time_point start =
    std::chrono::high_resolution_clock::now();

static void processRequest(Request *request) {
  const Request::BufferMap &buffers = request->buffers();

  std::chrono::high_resolution_clock::time_point request_start =
      std::chrono::high_resolution_clock::now();
  for (auto bufferPair : buffers) {
    // (Unused) Stream *stream = bufferPair.first;
    FrameBuffer *buffer = bufferPair.second;

    // const FrameMetadata &metadata = buffer->metadata();

    /* Print some information about the buffer which has completed. */
    // std::cout << " seq: " << std::setw(6) << std::setfill('0')
    //           << metadata.sequence << " bytesused: ";

    // unsigned int nplane = 0;
    // for (const FrameMetadata::Plane &plane : metadata.planes()) {
    //   // plane.mem()
    //   std::cout << plane.bytesused;
    //   if (++nplane < metadata.planes().size())
    //     std::cout << "/";
    // }

    // if (metadata.sequence == 20) {
    auto it = mapped_buffers.find(buffer);
    if (it != mapped_buffers.end()) {
      //   std::cout << " " << it->second[0].size_bytes() << std::endl;
      //   std::ofstream ofs("/home/pi/foo.ppm", std::ios::out);
      //   ofs << "P6\n640 480\n255\n";
      uint8_t *buf = it->second[0].data();

      auto R = [&](auto x, auto y) -> uint8_t & {
        return buf[x * 640 * 3 + y * 3];
      };
      //   auto G = [&](auto x, auto y) -> uint8_t & {
      //     return buf[x * 640 * 3 + y * 3 + 1];
      //   };
      //   auto B = [&](auto x, auto y) -> uint8_t & {
      //     return buf[x * 640 * 3 + y * 3 + 2];
      //   };

      int numBrightPixels = 0;
      for (int x = 200; x < 280; ++x) {
        for (int y = 280; y < 360; ++y) {
          if (R(x, y) > 100) {
            ++numBrightPixels;
          }
        }
      }
      std::cout << numBrightPixels;

      //   ofs.write((char const *)buf, it->second[0].size_bytes());
      //   ofs.close();
      //   std::terminate();
    };
    // }

    /*
     * Image data can be accessed here, but the FrameBuffer
     * must be mapped by the application
     */
  }

  auto now = std::chrono::high_resolution_clock::now();

  std::cout << '\t'
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   now - request_start)
                   .count()
            << "usec "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   now - std::exchange(start, now))
                   .count()
            << "usec\n";

  /* Re-queue the Request to the camera. */
  request->reuse(Request::ReuseBuffers);
  camera->queueRequest(request);
}

static void requestComplete(Request *request) {
  if (request->status() == Request::RequestCancelled)
    return;

  loop.callLater([request]() { processRequest(request); });
}

/*
 * ----------------------------------------------------------------------------
 * Camera Naming.
 *
 * Applications are responsible for deciding how to name cameras, and present
 * that information to the users. Every camera has a unique identifier, though
 * this string is not designed to be friendly for a human reader.
 *
 * To support human consumable names, libcamera provides camera properties
 * that allow an application to determine a naming scheme based on its needs.
 *
 * In this example, we focus on the location property, but also detail the
 * model string for external cameras, as this is more likely to be visible
 * information to the user of an externally connected device.
 *
 * The unique camera ID is appended for informative purposes.
 */
std::string cameraName(Camera *camera) {
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

int main() {
  std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
  cm->start();

  /*
   * Just as a test, generate names of the Cameras registered in the
   * system, and list them.
   */
  for (auto const &camera : cm->cameras())
    std::cout << " - " << cameraName(camera.get()) << std::endl;

  /*
   * --------------------------------------------------------------------
   * Camera
   *
   * Camera are entities created by pipeline handlers, inspecting the
   * entities registered in the system and reported to applications
   * by the CameraManager.
   *
   * In general terms, a Camera corresponds to a single image source
   * available in the system, such as an image sensor.
   *
   * Application lock usage of Camera by 'acquiring' them.
   * Once done with it, application shall similarly 'release' the Camera.
   *
   * As an example, use the first available camera in the system after
   * making sure that at least one camera is available.
   *
   * Cameras can be obtained by their ID or their index, to demonstrate
   * this, the following code gets the ID of the first camera; then gets
   * the camera associated with that ID (which is of course the same as
   * cm->cameras()[0]).
   */
  if (cm->cameras().empty()) {
    std::cout << "No cameras were identified on the system." << std::endl;
    cm->stop();
    return EXIT_FAILURE;
  }

  std::string cameraId = cm->cameras()[0]->id();
  camera = cm->get(cameraId);
  camera->acquire();

  /*
   * Stream
   *
   * Each Camera supports a variable number of Stream. A Stream is
   * produced by processing data produced by an image source, usually
   * by an ISP.
   *
   *   +-------------------------------------------------------+
   *   | Camera                                                |
   *   |                +-----------+                          |
   *   | +--------+     |           |------> [  Main output  ] |
   *   | | Image  |     |           |                          |
   *   | |        |---->|    ISP    |------> [   Viewfinder  ] |
   *   | | Source |     |           |                          |
   *   | +--------+     |           |------> [ Still Capture ] |
   *   |                +-----------+                          |
   *   +-------------------------------------------------------+
   *
   * The number and capabilities of the Stream in a Camera are
   * a platform dependent property, and it's the pipeline handler
   * implementation that has the responsibility of correctly
   * report them.
   */

  /*
   * --------------------------------------------------------------------
   * Camera Configuration.
   *
   * Camera configuration is tricky! It boils down to assign resources
   * of the system (such as DMA engines, scalers, format converters) to
   * the different image streams an application has requested.
   *
   * Depending on the system characteristics, some combinations of
   * sizes, formats and stream usages might or might not be possible.
   *
   * A Camera produces a CameraConfigration based on a set of intended
   * roles for each Stream the application requires.
   */
  std::unique_ptr<CameraConfiguration> config =
      camera->generateConfiguration({StreamRole::Viewfinder});

  /*
   * The CameraConfiguration contains a StreamConfiguration instance
   * for each StreamRole requested by the application, provided
   * the Camera can support all of them.
   *
   * Each StreamConfiguration has default size and format, assigned
   * by the Camera depending on the Role the application has requested.
   */
  StreamConfiguration &streamConfig = config->at(0);
  std::cout << "Default viewfinder configuration is: "
            << streamConfig.toString() << std::endl;

  /*
   * Each StreamConfiguration parameter which is part of a
   * CameraConfiguration can be independently modified by the
   * application.
   *
   * In order to validate the modified parameter, the CameraConfiguration
   * should be validated -before- the CameraConfiguration gets applied
   * to the Camera.
   *
   * The CameraConfiguration validation process adjusts each
   * StreamConfiguration to a valid value.
   */

  /*
   * The Camera configuration procedure fails with invalid parameters.
   */
  streamConfig.size.width = 640;  // 4096
  streamConfig.size.height = 480; // 2560
  streamConfig.pixelFormat = libcamera::formats::BGR888;

  /*
   * Validating a CameraConfiguration -before- applying it will adjust it
   * to a valid configuration which is as close as possible to the one
   * requested.
   */
  config->validate();
  std::cout << "Validated viewfinder configuration is: "
            << streamConfig.toString() << std::endl;

  /*
   * Once we have a validated configuration, we can apply it to the
   * Camera.
   */
  if (camera->configure(config.get())) {
    std::cout << "CONFIGURATION FAILED!" << std::endl;
    return EXIT_FAILURE;
  }
  // camera->configure(config.get());

  /*
   * --------------------------------------------------------------------
   * Buffer Allocation
   *
   * Now that a camera has been configured, it knows all about its
   * Streams sizes and formats. The captured images need to be stored in
   * framebuffers which can either be provided by the application to the
   * library, or allocated in the Camera and exposed to the application
   * by libcamera.
   *
   * An application may decide to allocate framebuffers from elsewhere,
   * for example in memory allocated by the display driver that will
   * render the captured frames. The application will provide them to
   * libcamera by constructing FrameBuffer instances to capture images
   * directly into.
   *
   * Alternatively libcamera can help the application by exporting
   * buffers allocated in the Camera using a FrameBufferAllocator
   * instance and referencing a configured Camera to determine the
   * appropriate buffer size and types to create.
   */
  FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);

  for (StreamConfiguration &cfg : *config) {
    int ret = allocator->allocate(cfg.stream());
    if (ret < 0) {
      std::cerr << "Can't allocate buffers" << std::endl;
      return EXIT_FAILURE;
    }

    size_t allocated = allocator->buffers(cfg.stream()).size();
    std::cout << "Allocated " << allocated << " buffers for stream"
              << std::endl;

    for (const std::unique_ptr<FrameBuffer> &buffer :
         allocator->buffers(cfg.stream())) {
      // "Single plane" buffers appear as multi-plane here, but we can spot them
      // because then planes all share the same fd. We accumulate them so as to
      // mmap the buffer only once.
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
          std::cout << reinterpret_cast<uintptr_t>(memory) << " " << buffer_size
                    << std::endl;
          buffer_size = 0;
        }
      }
    }
  }

  /*
   * --------------------------------------------------------------------
   * Frame Capture
   *
   * libcamera frames capture model is based on the 'Request' concept.
   * For each frame a Request has to be queued to the Camera.
   *
   * A Request refers to (at least one) Stream for which a Buffer that
   * will be filled with image data shall be added to the Request.
   *
   * A Request is associated with a list of Controls, which are tunable
   * parameters (similar to v4l2_controls) that have to be applied to
   * the image.
   *
   * Once a request completes, all its buffers will contain image data
   * that applications can access and for each of them a list of metadata
   * properties that reports the capture parameters applied to the image.
   */
  Stream *stream = streamConfig.stream();
  const std::vector<std::unique_ptr<FrameBuffer>> &buffers =
      allocator->buffers(stream);
  std::vector<std::unique_ptr<Request>> requests;
  for (unsigned int i = 0; i < buffers.size(); ++i) {
    std::unique_ptr<Request> request = camera->createRequest();
    if (!request) {
      std::cerr << "Can't create request" << std::endl;
      return EXIT_FAILURE;
    }

    const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
    int ret = request->addBuffer(stream, buffer.get());
    if (ret < 0) {
      std::cerr << "Can't set buffer for request" << std::endl;
      return EXIT_FAILURE;
    }

    /*
     * Controls can be added to a request on a per frame basis.
     */
    ControlList &controls = request->controls();
    // controls.set(controls::Brightness, 0.5);
    int64_t frame_time = 1000000 / 60; // in us
    controls.set(controls::FrameDurationLimits, {frame_time, frame_time});

    requests.push_back(std::move(request));
  }

  /*
   * --------------------------------------------------------------------
   * Signal&Slots
   *
   * libcamera uses a Signal&Slot based system to connect events to
   * callback operations meant to handle them, inspired by the QT graphic
   * toolkit.
   *
   * Signals are events 'emitted' by a class instance.
   * Slots are callbacks that can be 'connected' to a Signal.
   *
   * A Camera exposes Signals, to report the completion of a Request and
   * the completion of a Buffer part of a Request to support partial
   * Request completions.
   *
   * In order to receive the notification for request completions,
   * applications shall connecte a Slot to the Camera 'requestCompleted'
   * Signal before the camera is started.
   */
  camera->requestCompleted.connect(requestComplete);

  /*
   * --------------------------------------------------------------------
   * Start Capture
   *
   * In order to capture frames the Camera has to be started and
   * Request queued to it. Enough Request to fill the Camera pipeline
   * depth have to be queued before the Camera start delivering frames.
   *
   * For each delivered frame, the Slot connected to the
   * Camera::requestCompleted Signal is called.
   */
  camera->start();
  for (std::unique_ptr<Request> &request : requests)
    camera->queueRequest(request.get());

  /*
   * --------------------------------------------------------------------
   * Run an EventLoop
   *
   * In order to dispatch events received from the video devices, such
   * as buffer completions, an event loop has to be run.
   */
  // loop.timeout(TIMEOUT_SEC);
  int ret = loop.exec();
  std::cout << "Capture ran for " << TIMEOUT_SEC << " seconds and "
            << "stopped with exit status: " << ret << std::endl;

  /*
   * --------------------------------------------------------------------
   * Clean Up
   *
   * Stop the Camera, release resources and stop the CameraManager.
   * libcamera has now released all resources it owned.
   */
  camera->stop();
  allocator->free(stream);
  delete allocator;
  camera->release();
  camera.reset();
  cm->stop();

  return EXIT_SUCCESS;
}
