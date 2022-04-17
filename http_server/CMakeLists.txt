include_directories(${CMAKE_SOURCE_DIR})

add_library(http_server http_server.cpp)
target_link_libraries(http_server)