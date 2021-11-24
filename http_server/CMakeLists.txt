include(GNUInstallDirs)

find_library(JPEG_LIBRARY jpeg REQUIRED)

add_library(http_server http_server.cpp)
target_link_libraries(http_server jpeg)