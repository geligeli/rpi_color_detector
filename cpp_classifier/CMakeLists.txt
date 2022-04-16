include_directories(${CMAKE_SOURCE_DIR})
add_library(cpp_classifier cpp_classifier.cpp)
# target_link_libraries(cpp_classifier -lpthread -ltensorflowlite_c)