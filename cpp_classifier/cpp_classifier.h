#pragma once

#include <vector>
#include <string>
namespace cpp_classifier {

struct Classifier {
  float intercept;
  int height;
  int width;
  int num_channels;

  struct Coef {
    int x;
    int y;
    int c;
    float coef;
  };
  std::vector<Coef> coefs;

  void LoadFromFile(const std::string& fn);

  float Classify(unsigned char const * data,int h,int w) const;
};

}  // namespace cpp_classifier