cmake_minimum_required(VERSION 3.6)

project(color-detect)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 17)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.66.0)
IF(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
ENDIF(Boost_FOUND)

add_compile_options(-O3 -Wall -Wextra -pedantic -Wno-unused-parameter -faligned-new -Werror -Wfatal-errors)
add_definitions(-D_FILE_OFFSET_BITS=64)

if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wno-psabi)
endif()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

IF (NOT ENABLE_COMPILE_FLAGS_FOR_TARGET)
    # On a Pi this will give us armhf or arm64.
    execute_process(COMMAND dpkg-architecture -qDEB_HOST_ARCH
        OUTPUT_VARIABLE ENABLE_COMPILE_FLAGS_FOR_TARGET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
message(STATUS "Platform: ${ENABLE_COMPILE_FLAGS_FOR_TARGET}")

add_definitions(-ftree-vectorize)

find_package(PkgConfig REQUIRED)

add_subdirectory(http_server)
add_subdirectory(camera_loop)
add_executable(servo_example servo_example.cpp)

add_executable(http_server_main http_server_main.cpp)
target_link_libraries(http_server_main LINK_PUBLIC http_server -lpthread)

add_executable(camera_loop_main camera_loop_main.cpp)
target_link_libraries(camera_loop_main camera_loop)

pkg_check_modules(LIBCAMERA libcamera REQUIRED)
message(STATUS "libcamera library found:")
message(STATUS "    version: ${LIBCAMERA_VERSION}")
message(STATUS "    libraries: ${LIBCAMERA_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBCAMERA_INCLUDE_DIRS}")

include_directories(${CMAKE_SOURCE_DIR} ${LIBCAMERA_INCLUDE_DIRS})
add_executable(color_detect color_detect.cpp)
target_link_libraries(color_detect http_server camera_loop ${LIBCAMERA_LINK_LIBRARIES})

add_executable(stepper_example stepper_example.cpp)
target_link_libraries(stepper_example -lpthread -lpigpio -lrt)
