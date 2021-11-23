#include <exception>
#include <iostream>

#include "http_server/http_server.h"

int main(int argc, char *argv[]) {
  try {
    run_server();
  } catch (std::exception const &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}