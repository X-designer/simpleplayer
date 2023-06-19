cmake_minimum_required(VERSION 3.24)
project(ffmpeg_demos VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FFMPEG_ROOT "D:/ffmpeg_dev")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#find_package(FFMPEG COMPONENTS AVCODEC AVFORMAT AVUTIL AVDEVICE REQUIRED)
find_package(FFMPEG COMPONENTS avcodec avformat avutil avdevice REQUIRED)

list(APPEND CMAKE_PREFIX_PATH "D:/CGlib/SDL2-2.26.5/x86_64-w64-mingw32/lib/cmake")
find_package(SDL2 CONFIG)



add_subdirectory(apps)

