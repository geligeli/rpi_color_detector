#include "cpp_classifier/cpp_classifier.h"

#include <array>
#include <iostream>
namespace cpp_classifier {

void Classifier::LoadFromFile(const std::string& fn) {
  FILE* fp = fopen(fn.c_str(), "rb");

  std::array<int64_t, 4> buf;
  fread(&buf, sizeof(buf), 1, fp);

  auto [height, width, num_channels, n] = buf;

  auto readInt = [&]() -> int {
    int64_t v{};
    fread(&v, sizeof(v), 1, fp);
    return v;
  };

  auto readFloat = [&]() -> float {
    float v{};
    fread(&v, sizeof(v), 1, fp);
    return v;
  };

  coefs.resize(n);
  for (int i = 0; i < n; ++i) {
    coefs[i].x = readInt();
    if (coefs[i].x < 0 || coefs[i].x >= height) {
      throw std::runtime_error("x out of bounds");
    }
  }
  for (int i = 0; i < n; ++i) {
    coefs[i].y = readInt();
    if (coefs[i].y < 0 || coefs[i].y >= width) {
      throw std::runtime_error("y out of bounds");
    }
  }
  for (int i = 0; i < n; ++i) {
    coefs[i].c = readInt();
    if (coefs[i].c < 0 || coefs[i].c >= num_channels) {
      throw std::runtime_error("c out of bounds");
    }
  }
  intercept = readFloat();
  for (int i = 0; i < n; ++i) {
    coefs[i].coef = readFloat();
  }

  fclose(fp);
}

float Classifier::Classify(unsigned char const* data, int h, int w) const {
  float r = intercept;
  for (const auto& c : coefs) {
    r += data[c.x*3*w + c.y*3 + c.c]*c.coef;
  }
  return r;
}

}  // namespace cpp_classifier