#include "FFMPEGvideoWriter.h"
#include <stdexcept>


#define _CRT_SECURE_NO_WARNINGS //disable warning C4996

void FFMPEGvideoWriter::log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt)
{
    AVRational* time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    char buf1[AV_TS_MAX_STRING_SIZE], buf2[AV_TS_MAX_STRING_SIZE], buf3[AV_TS_MAX_STRING_SIZE], buf4[AV_TS_MAX_STRING_SIZE], buf5[AV_TS_MAX_STRING_SIZE], buf6[AV_TS_MAX_STRING_SIZE];
    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
        av_ts_make_string(buf1, pkt->pts), av_ts_make_time_string(buf2, pkt->pts, time_base),
        av_ts_make_string(buf3, pkt->pts), av_ts_make_time_string(buf4, pkt->pts, time_base),
        av_ts_make_string(buf5, pkt->pts), av_ts_make_time_string(buf6, pkt->pts, time_base),
        pkt->stream_index);
}

int FFMPEGvideoWriter::write_frame(AVFormatContext* fmt_ctx, AVCodecContext* c, AVStream* st, AVFrame* frame)
{
    int ret;
    char buf[AV_ERROR_MAX_STRING_SIZE];
    char buf2[AV_ERROR_MAX_STRING_SIZE*2];

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        sprintf(buf2, "Error sending a frame to the encoder: %s\n", av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret));
        throw std::runtime_error(buf2);
    }

    while (ret >= 0) {
        AVPacket pkt = { 0 };

        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            sprintf(buf2, "Error encoding a frame: %s\n", av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret));
            throw std::runtime_error(buf2);
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
        pkt.stream_index = st->index;

        /* Write the compressed frame to the media file. */
        if(av_log_get_level() >= AV_LOG_VERBOSE) log_packet(fmt_ctx, &pkt);
        ret = av_interleaved_write_frame(fmt_ctx, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            sprintf(buf2, "Error while writing output packet: %s\n", av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret));
            throw std::runtime_error(buf2);
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}

AVFrame* FFMPEGvideoWriter::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame* picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if (ret < 0) {
        throw std::runtime_error("Could not allocate frame data.");
    }

    return picture;
}


FFMPEGvideoWriter::FFMPEGvideoWriter()
{
    av_log_set_level(AV_LOG_WARNING); //prevent libx264 from logging to std::error, also disables dump output
}

FFMPEGvideoWriter::~FFMPEGvideoWriter()
{
    if (isOpen) release();
}

bool FFMPEGvideoWriter::open(const std::string& filename, double fps, cv::Size frameSize, const std::string& encoder, const std::string& options, bool isColor, bool isBGR)
{   
    if (this->isOpen) release();
    next_pts = 1;

    int& width = frameSize.width;
    int& height = frameSize.height;
    int ret;
    AVPixelFormat inputPixelFormat; 
    constexpr auto cbuf_size = 200;
    char cbuf[cbuf_size];

    if (~isColor)
    {
        inputPixelFormat = AV_PIX_FMT_GRAY8;
    }
    else
    {
        if (isBGR)
            inputPixelFormat = AV_PIX_FMT_BGR24;
        else //RGB
            inputPixelFormat = AV_PIX_FMT_RGB24;
    }

    
    av_dict_free(&opt);
    ret = av_dict_parse_string(&opt, options.c_str(), "=", ",", 0);
    if (ret < 0) {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        sprintf(cbuf, "Could not parse options string: %s\n", av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret));
        throw std::runtime_error(cbuf);
    }


    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename.c_str());
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename.c_str());
    }
    if (!oc) throw std::runtime_error("Could not deduce output format from file extension");

    fmt = oc->oformat;


    //// add_stream ////

    /* Add the video stream using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec == AV_CODEC_ID_NONE) throw std::runtime_error("Output format does not support video streams");

    if (encoder.empty())
    {
        codec = avcodec_find_encoder(fmt->video_codec);
        if (!codec) {
            sprintf(cbuf, "Could not find encoder for '%s'\n", avcodec_get_name(fmt->video_codec));
            throw std::runtime_error(cbuf);
        }
    }
    else
    {
        codec = avcodec_find_encoder_by_name(encoder.c_str());
        if (!codec) {
            sprintf(cbuf, "Could not find encoder for '%s'\n", encoder.c_str());
            throw std::runtime_error(cbuf);
        }
    }

    AVCodecContext* c;
    st = avformat_new_stream(oc, NULL);
    if (!st) {
        sprintf(cbuf, "Could not allocate stream\n");
        throw std::runtime_error(cbuf);
    }
    st->id = oc->nb_streams - 1;
    c = avcodec_alloc_context3(codec);
    if (!c) {
        sprintf(cbuf, "Could not alloc an encoding context\n");
        throw std::runtime_error(cbuf);
    }
    enc = c;

    if (codec->type != AVMEDIA_TYPE_VIDEO) throw std::runtime_error("Codec is not a video codec");

    c->codec_id = codec->id;

    /* Resolution must be a multiple of two. */
    c->width = width;
    c->height = height;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    AVRational rfps = av_d2q(fps, INT32_MAX);
    st->time_base = av_div_q({ 1, 1 }, rfps);
    c->time_base = st->time_base;
    st->avg_frame_rate = rfps;
    c->framerate = rfps;

    //AVPixelFormat pixelFormat = AV_PIX_FMT_YUV420P;

    if(!codec->pix_fmts) throw std::runtime_error("Codec does not have a supported pixel format.");

    AVPixelFormat pixelFormat = codec->pix_fmts[0]; //get defualt pixel format for selected codec
    c->pix_fmt = pixelFormat;


    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    //// end add_stream /////


    //// open_video ////

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    if (ret < 0) {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        sprintf(cbuf, "Could not open video codec: %s\n", av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret));
        throw std::runtime_error(cbuf);
    }

    /* allocate and init a re-usable frame */
    frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!frame) {
        throw std::runtime_error("Could not allocate video frame");
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(st->codecpar, c);
    if (ret < 0) {
        throw std::runtime_error("Could not copy the stream parameters");
    }
    //// end open_video ////
    av_dump_format(oc, 0, filename.c_str(), 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            char buf[AV_ERROR_MAX_STRING_SIZE];
            sprintf(cbuf, "Could not open '%s': %s\n", filename.c_str(), av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret));
            throw std::runtime_error(cbuf);
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        char buf[AV_ERROR_MAX_STRING_SIZE];
        fprintf(stderr, "Error occurred when opening output file: %s\n", av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, ret));
        throw std::runtime_error(cbuf);
    }

    /* opt should be empty now */
    if (av_dict_count(opt))
    {
        char* pbuf = NULL;
        av_dict_get_string(opt, &pbuf, '=', ',');
        std::cerr << "the following key/value options were not found: " << pbuf << std::endl;
        av_freep(&pbuf);
    }

    /* init pixel format converter */
    sws_context = sws_getCachedContext(sws_context, width, height, inputPixelFormat, width, height, pixelFormat, 0, 0, 0, 0);
    if (!sws_context) {
        throw std::runtime_error("Could not initialize the conversion context");
    }

    this->isOpen = true;
    return true;
}

void FFMPEGvideoWriter::release()
{
    if (!this->isOpen) return;

    /* flush the encoder */
    write_frame(oc, enc, st, NULL);

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);


    //// close stream ////
    avcodec_free_context(&enc);
    av_frame_free(&frame);
    sws_freeContext(sws_context);
    //// end close stream ////
    av_dict_free(&opt);

    this->isOpen = false;
    return;
}

void FFMPEGvideoWriter::write(cv::Mat& image)
{
    if (!isOpen) return;
    
    int linesize[] = { int(image.step[0])};
    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(this->frame) < 0) throw std::runtime_error("Could not make frame writable");

    sws_scale(this->sws_context, &image.data, linesize, 0, this->frame->height, this->frame->data, this->frame->linesize); //pixel format conversion
    this->frame->pts = next_pts++;
    write_frame(oc, enc, st, this->frame);
    return;
}
