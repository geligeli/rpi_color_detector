#include "image_task/image_task.h"

#include <jpeglib.h>

#include <cstring>
#include <fstream>

namespace image_task {

namespace {

std::string base64_encode(const std::string& in) {
  std::string out;

  int val = 0, valb = -6;
  for (unsigned char c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(
          "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
              [(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
            [((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4) out.push_back('=');
  return out;
}
/*
std::string base64_decode(const std::string& in) {
  std::string out;

  std::vector<int> T(256, -1);
  for (int i = 0; i < 64; i++)
    T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] =
        i;

  int val = 0, valb = -8;
  for (unsigned char c : in) {
    if (T[c] == -1) break;
    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}*/
}  // namespace

void ImageTask::CaptureImage(uint8_t const* data, int h, int w) {
  m_image_for_capture->m_data.resize(h * w * 3);
  m_image_for_capture->m_height = h;
  m_image_for_capture->m_width = w;
  std::memcpy(m_image_for_capture->m_data.data(), data, h * w * 3);
  {
    std::lock_guard lk(m_mutex);
    std::swap(m_image_for_capture, m_last_image_for_capture);
    m_current_capture = nullptr;
  }
  m_cv.notify_one();
}

std::shared_ptr<ImageTask::Capture> ImageTask::getCurrentCapture() {
  std::unique_lock<std::mutex> lk(m_mutex);
  if (m_current_capture == nullptr) {
    lk.unlock();
    return getNextCapture();
  }
  return m_current_capture;
}

std::shared_ptr<ImageTask::Capture> ImageTask::getNextCapture() {
  std::unique_lock<std::mutex> lk(m_mutex);
  m_cv.wait(lk, [&]() { return !m_last_image_for_capture->m_data.empty(); });
  m_current_capture = std::shared_ptr<Capture>(
      new ImageTask::Capture(std::move(*m_last_image_for_capture), this));
  return m_current_capture;
}

ImageTask::Capture::Capture(ImageTask::RawImage&& raw_image, ImageTask* parent)
    : m_raw_image{std::move(raw_image)}, m_parent{parent} {}

void ImageTask::Capture::dumpJson(std::ostream& os) {
  os << "{\"image\" : \"data:image/jpeg;base64," << base64_encode(getJpeg())
     << "\", \"classification\":";
  cpp_classifier::Classifier::Classification c = getClassification();
  os << "\"" << c.predictedClass() << "\"}";
}

const std::string& ImageTask::Capture::getJpeg() {
  std::lock_guard<std::mutex> lk(m_jpeg_mutex);
  if (m_jpeg_representation) {
    return *m_jpeg_representation;
  }

  m_jpeg_representation = std::string();
  std::string& buffer = *m_jpeg_representation;

  int quality = 50;
  struct jpeg_compress_struct cinfo;  // Basic info for JPEG properties.
  struct jpeg_error_mgr jerr;         // In case of error.
  JSAMPROW row_pointer[1];            // Pointer to JSAMPLE row[s].
  int row_stride;                     // Physical row width in image buffer.
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  struct Result {
    unsigned char* buf{};
    unsigned long size{};
  } result;

  jpeg_mem_dest(&cinfo, &result.buf, &result.size);

  cinfo.image_width = m_raw_image.m_width;
  cinfo.image_height = m_raw_image.m_height;
  cinfo.input_components = 3;      // Number of color components per pixel.
  cinfo.in_color_space = JCS_RGB;  // Colorspace of input image as RGB.

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);

  unsigned char* image_buffer = m_raw_image.m_data.data();
  jpeg_start_compress(&cinfo, TRUE);
  row_stride = cinfo.image_width * 3;  // JSAMPLEs per row in image_buffer

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
    (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  buffer.resize(result.size);
  std::copy(result.buf, result.buf + result.size, buffer.begin());
  free(result.buf);
  return buffer;
}

void ImageTask::Capture::dumpJpegFile(const std::string& fn) {
  FILE* fp = std::fopen(fn.c_str(), "w");
  if (!fp) {
    return;
  }
  const auto& img = getJpeg();
  std::fwrite(&img[0], img.size(), 1, fp);
  std::ofstream(fn + ".classification") << getClassification();
  std::fclose(fp);
}

cpp_classifier::Classifier::Classification
ImageTask::Capture::getClassification() {
  std::lock_guard<std::mutex> lk(m_classification_mutex);
  if (!m_classification) {
    m_classification = m_parent->m_classifier.Classify(
        m_raw_image.m_data.data(), m_raw_image.m_height, m_raw_image.m_width);
  }
  return *m_classification;
}

}  // namespace image_task