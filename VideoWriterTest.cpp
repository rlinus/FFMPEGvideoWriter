#include "FFMPEGvideoWriter.h"

int main(int argc, char* argv[])
{
    const char filename[] = "testvid4.mkv";

    std::string encoder = "h264_nvenc"; //ffmpeg -encoders
    std::string options = "preset=medium, rc = constqp, qp=23";
    //std::string codecName = "libx264"; //ffmpeg -encoders

    int width = 352;
    int height = 288;

    int i, ret, x, y;
    

    FFMPEGvideoWriter videoWriter;

    videoWriter.open(filename, 25, cv::Size(width, height), encoder, options, true, false);
    

    AVFrame* frame;
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = AV_PIX_FMT_RGB24;
    frame->width = width;
    frame->height = height;
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Main: Could not allocate the video frame data\n");
        exit(1);
    }
    using namespace std;

    cv::Mat frameCV(height, width, CV_8UC3);
    //fprintf(stdout, "step: %d, %d\n", (int)frameCV.step[0], (int)frameCV.step[1]);

    //int imgBufferSizeRGB = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    //int imgBufferSizeYUV = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
    //fprintf(stdout, "RGB: %d, YUV: %d\n", imgBufferSizeRGB, imgBufferSizeYUV);
    //fprintf(stdout, "linesize: %d, %d , %d, %d\n", frame->linesize[0], frame->linesize[1], frame->linesize[2], frame->linesize[3]);
    

    /* encode 1 second of video */
    for (i = 0; i < 250; i++) {
        fflush(stdout);
        /* make sure the frame data is writable */
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            return 1;
        /* prepare a dummy image */
        /* Y */
        //for (y = 0; y < height; y++) {
        //    for (x = 0; x < width; x++) {
        //        frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
        //    }
        //}
        ///* Cb and Cr */
        //for (y = 0; y < height / 2; y++) {
        //    for (x = 0; x < width / 2; x++) {
        //        frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
        //        frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
        //    }
        //}
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                frame->data[0][y * frame->linesize[0] + x * 3 + 0] = x + y + i * 3;
                frameCV.data[y * frameCV.step[0] + x * frameCV.step[1] + 0] = x + y + i * 3;
                frameCV.data[y * frameCV.step[0] + x * frameCV.step[1] + 1] = 128 + y + i * 2;
                frameCV.data[y * frameCV.step[0] + x * frameCV.step[1] + 2] = 64 + x + i * 5;
            }
        }
        frame->pts = i;
        /* encode the image */
        videoWriter.write(frameCV);
    }
    

    videoWriter.release();
    av_frame_free(&frame);
    return 0;
}