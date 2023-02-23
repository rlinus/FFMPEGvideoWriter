#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/avassert.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

/**
 * FFMPEG video writer class.
 * The class mimics the interface of the OpenCV cv::VideoWriter class, with the added benefit that it lets you select arbitrary encoders and configure all supported options.
 * This is mostly a C++ wrapper of (this example)[https://ffmpeg.org/doxygen/trunk/muxing_8c-source.html] without the soundstream.
 * 
 * (c) Linus Ruettimann, 2020
 */
class FFMPEGvideoWriter {
public:
	FFMPEGvideoWriter();
	virtual ~FFMPEGvideoWriter();

	/**
	 * Initializes or reinitializes video writer.
	 * The method first calls FFMOEGvideoWriter::release to close the already opened file.
	 * The video writer uses the ffmpeg library to encode and muxe video frames into a file named filename.\n"
     * The output format/container is automatically guessed according to the file extension in the filename.
     * Raw images can also be output by using '%%d' in the filename.
	 *
	 * @param filename Name of the output video file.
	 * @param fps Framerate of the created video stream.
	 * @param frameSize Image size
	 * @param encoder Specifies the encoder to use (e.g. "libx264" or "h264_nvenc"). If empty the default encoder for the container is used. Run 'ffmpeg -encoders' to get a list of supported encoders.
	 * @param options Options for the encoder as a comma separated key=value list (in this form: "key1=value1, key2=value2"). Run e.g. 'ffmpeg -h encoder=h264_nvenc' to get a list of supported options for a specific encoder.
	 * @param isColor If true, the encoder will expect and encode color frames, otherwise it will work with grayscale frames.
	 * @param isBGR If true, the encoder will expect images in BGR pixel format (OpenCV standard format), if false images are expected in RGB format.
	 * @return true if video writer has been successfully initialized (otherwise exception is thrown)
	 * @throw std::runtime_exception if encoder or file cannot be opened, or something else goes wrong.
	 */
	bool open(const std::string& filename, double fps, cv::Size frameSize, const std::string& encoder = std::string(), const std::string& options = std::string(), bool isColor = true, bool isBGR = true);

	/**
	 * Closes the video writer.
	 *
	 * The method is automatically called by subsequent FFMPEGvideoWriter::open and by the FFMPEGvideoWriter::~FFMPEGvideoWriter destructor.
	 */
	void release();

	 /**
	  * Writes the next video frame.
	  *
	  * @param image the frame to be written. The cv::Mat::type type is expected to be CV_8UC3 (isColor = true) or CV_8UC1 (isColor = false).
	  * @throw std::runtime_exception if error occurs.
	  */
	void write(cv::Mat &image);


	/**
	  * @return Returns true if video writer has been successfully initialized.
	  */
	virtual bool isOpened() const { return isOpen; };

private:
	int write_frame(AVFormatContext* fmt_ctx, AVCodecContext* c, AVStream* st, AVFrame* frame);
	void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt);
	AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);

	bool isOpen = false;

	const AVCodec* codec = NULL;

	AVCodecContext* enc = NULL;
	AVFrame* frame = NULL;
	struct SwsContext* sws_context = NULL; //rgb24 to yuv420 conversion object

	AVStream* st = NULL;
	AVOutputFormat* fmt = NULL;
	AVFormatContext* oc = NULL;
	AVDictionary* opt = NULL;
	int64_t next_pts = 0;
};