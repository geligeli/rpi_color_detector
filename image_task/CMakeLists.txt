include(GNUInstallDirs)

find_library(JPEG_LIBRARY jpeg REQUIRED)

pkg_check_modules(JPEG_LIBRARY libjpeg)
message(STATUS "libjpeg library found:")
message(STATUS "    version: ${JPEG_LIBRARY_VERSION}")
message(STATUS "    libraries: ${JPEG_LIBRARY_LINK_LIBRARIES}")
message(STATUS "    include path: ${JPEG_LIBRARY_INCLUDE_DIRS}")
include_directories(${CMAKE_SOURCE_DIR} ${JPEG_LIBRARY_INCLUDE_DIRS})

add_library(image_task image_task.cpp)
target_link_libraries(image_task ${JPEG_LIBRARY_LINK_LIBRARIES})