find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBCAMERA libcamera)
message(STATUS "libcamera library found:")
message(STATUS "    version: ${LIBCAMERA_VERSION}")
message(STATUS "    libraries: ${LIBCAMERA_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBCAMERA_INCLUDE_DIRS}")

if (LIBCAMERA_FOUND)
  include_directories(${CMAKE_SOURCE_DIR} ${LIBCAMERA_INCLUDE_DIRS})

  add_library(camera_loop camera_loop.cpp)
  target_link_libraries(camera_loop ${LIBCAMERA_LINK_LIBRARIES})
endif()

