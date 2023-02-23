# FFMPEGvideoWriter
FFMPEG video writer class. The class mimics the interface of the OpenCV cv::VideoWriter class, with the added benefit that it lets you select arbitrary encoders and configure all supported options.

This is mostly a C++ wrapper of (this example)[https://ffmpeg.org/doxygen/trunk/muxing_8c-source.html] without the soundstream.