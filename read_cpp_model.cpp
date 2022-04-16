#include <iostream>

#include "cpp_classifier/cpp_classifier.h"

int main(int argc, char** argv) {
    if (argc <= 1) {
        return -1;
    }

    cpp_classifier::Classifier c;
    c.LoadFromFile(argv[1]);  

    std::cout << c.intercept << '\n';
    for (const auto& c : c.coefs) {
        std::cout << c.x << " " << c.y << " " << c.c << " " << c.coef << '\n';

    }
    return 0;
}