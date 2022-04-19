#include "image_task/image_task.h"

#include <jpeglib.h>

#include <cstring>

namespace image_task {

namespace {

std::string base64_encode(const std::string &in) {
  std::string out;

  int val = 0, valb = -6;
  for (uchar c : in) {
      val = (val << 8) + c;
      valb += 8;
      while (valb >= 0) {
          out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
          valb -= 6;
      }
  }
  if (valb>-6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
  while (out.size()%4) out.push_back('=');
  return out;
}

std::string base64_decode(const std::string &in) {

  std::string out;

  std::vector<int> T(256,-1);
  for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

  int val=0, valb=-8;
  for (uchar c : in) {
      if (T[c] == -1) break;
      val = (val << 6) + T[c];
      valb += 6;
      if (valb >= 0) {
          out.push_back(char((val>>valb)&0xFF));
          valb -= 8;
      }
  }
  return out;
}
}  // namespace

void ImageTask::CaptureImage(uint8_t const* data, int h, int w) {
  if (!m_accept_capture_requests || !m_mutex.try_lock()) {
    return;
  }
  m_data.resize(h * w * 3);
  m_height = h;
  m_width = w;
  m_jpeg_representation = {};
  m_classification = {};
  std::memcpy(m_data.data(), data, h * w * 3);
  m_mutex.unlock();
  m_cv.notify_all();
}

void ImageTask::WaitForNewCapture() {
  std::unique_lock<std::mutex> lk(m_mutex);
  m_cv.wait(lk, [] { return true; });
}

ImageTask::RAIIRenableWrapper::~RAIIRenableWrapper() {
  m_image_task_ptr->m_accept_capture_requests = true;
}

ImageTask::RAIIRenableWrapper::RAIIRenableWrapper(ImageTask* image_task_ptr)
    : m_image_task_ptr{image_task_ptr} {}

ImageTask::RAIIRenableWrapper ImageTask::suspendCapture() {
  m_accept_capture_requests = false;
  return RAIIRenableWrapper(this);
}


std::string ImageTask::AsDataUrl() {
  return "data:image/jpeg;base64," + base64_encode(getJpeg());
}

std::string ImageTask::getJpeg() {
  std::lock_guard<std::mutex> l(m_mutex);
  if (m_data.empty()) {
    return "";
  }
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

  cinfo.image_width = m_width;
  cinfo.image_height = m_height;
  cinfo.input_components = 3;      // Number of color components per pixel.
  cinfo.in_color_space = JCS_RGB;  // Colorspace of input image as RGB.

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);

  unsigned char* image_buffer = m_data.data();
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

void ImageTask::dumpJpegFile(const std::string& fn) {
  auto img = getJpeg();
  if (img.empty()) {
    return;
  }
  FILE* fp = std::fopen(fn.c_str(), "w");
  if (!fp) {
    return;
  }
  std::fwrite(&img[0], img.size(), 1, fp);
  std::fclose(fp);
}

float ImageTask::getClassification() {
  std::lock_guard<std::mutex> l(m_mutex);
  if (!m_classification) {
    m_classification = m_classifierFun(m_data.data(), m_height, m_width);
  }
  return *m_classification;
}

}  // namespace image_task