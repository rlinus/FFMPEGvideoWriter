cmake_minimum_required (VERSION 3.8)

project (FFMPEGvideoWriter VERSION 0.1)

# The following dependencies were installed with vcpkg (vcpkg git commit 81b67c387848731aefacc19aea7f41364e5d5ba2)
find_package(OpenCV CONFIG REQUIRED)

find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h)
find_library(FFMPEG_AVCODEC_LIB avcodec)
find_library(FFMPEG_AVFORMAT_LIB avformat)
find_library(FFMPEG_UTIL_LIB avutil)
find_library(FFMPEG_SWSCALE_LIB swscale)
find_library(FFMPEG_SWRESAMPLE_LIB swresample)
set(FFMPEG_LIBS ${FFMPEG_AVCODEC_LIB} ${FFMPEG_AVFORMAT_LIB} ${FFMPEG_UTIL_LIB} ${FFMPEG_SWSCALE_LIB} ${FFMPEG_SWRESAMPLE_LIB})

##FFMPEGvideoWriterLib
add_library(FFMPEGvideoWriterLib "FFMPEGvideoWriter.cpp" FFMPEGvideoWriter.h)
target_link_libraries(FFMPEGvideoWriterLib PUBLIC opencv_core)

target_include_directories(FFMPEGvideoWriterLib PRIVATE ${FFMPEG_INCLUDE_DIR})
target_link_libraries(FFMPEGvideoWriterLib PRIVATE ${FFMPEG_LIBS})

##FFMPEGvideoWriteTest executable
add_executable(VideoWriterTest VideoWriterTest.cpp)
target_link_libraries (VideoWriterTest LINK_PUBLIC FFMPEGvideoWriterLib)
    




