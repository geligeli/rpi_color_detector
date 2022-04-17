#include "image_task/image_task.h"

#include <jpeglib.h>

namespace image_task {

void ImageTask::CaptureImage(uint8_t const* data, int h, int w) {
  std::lock_guard<std::mutex> l(m);
  m_data.resize(h * w * 3);
  m_height = h;
  m_width = w;
  m_jpeg_representation = {};
  m_classification = {};
  std::memcpy(m_data.data(), data, h * w * 3);
}

std::string ImageTask::getJpeg() {
  std::lock_guard<std::mutex> l(m);
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

  cinfo.image_width = m_height;
  cinfo.image_height = m_width;
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

float ImageTask::getClassification() {
  std::lock_guard<std::mutex> l(m);
  if (!m_classification) {
    m_classification = m_classifierFun(data.data(), m_height, m_width);
  }
  return *m_classification;
}

}  // namespace image_task