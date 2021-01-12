#include "VideoPlayer.h"

VideoPlayer::VideoPlayer() {
    av_format_ctx = avformat_alloc_context();
}

bool VideoPlayer::open_file(const char* filename) {

    reset_variables();

    // Open file
	if( avformat_open_input( &av_format_ctx, filename, NULL, NULL ) != 0 )
	{
        printf("Couldn't open video file\n");
        return false;
    }

    // Reset indexes
    video_stream_index = -1;
    audio_stream_index = -1;

    // Search stream indexes
    for (unsigned int i = 0; i < av_format_ctx->nb_streams; ++i) {
        switch (av_format_ctx->streams[i]->codecpar->codec_type){
            case AVMEDIA_TYPE_VIDEO:
                video_stream_index = i;
                video_av_codec_params = av_format_ctx->streams[i]->codecpar;
                video_av_codec = avcodec_find_decoder(video_av_codec_params->codec_id);
                break;
            case AVMEDIA_TYPE_AUDIO:
                audio_stream_index = i;
                audio_av_codec_params = av_format_ctx->streams[i]->codecpar;
                audio_av_codec = avcodec_find_decoder(audio_av_codec_params->codec_id);
                break;
        }
    }

    if (video_stream_index == -1) {
        printf("Couldn't find valid video stream inside file\n");
        return false;
    }

    // Set up a video codec context
    video_av_codec_ctx = avcodec_alloc_context3(video_av_codec);
    if (avcodec_parameters_to_context(video_av_codec_ctx, video_av_codec_params) < 0) {
        printf("Couldn't initialize AVCodecContext\n");
        return false;
    }
    if (avcodec_open2(video_av_codec_ctx, video_av_codec, NULL) < 0) {
        printf("Couldn't open codec\n");
        return false;
    }

    // Setup audio codec context
    if (audio_stream_index != -1) {
        audio_av_codec_ctx = avcodec_alloc_context3(audio_av_codec);
        if (avcodec_parameters_to_context(audio_av_codec_ctx, audio_av_codec_params) < 0) {
            printf("Couldn't initialize AVCodecContext\n");
            return false;
        }
        if (avcodec_open2(audio_av_codec_ctx, audio_av_codec, NULL) < 0) {
            printf("Couldn't open codec\n");
            return false;
        }
    }
    
    avformat_find_stream_info( av_format_ctx, NULL );

    video_data = new uint8_t[video_av_codec_ctx->width * video_av_codec_ctx->height * 4];

    loop_out = get_duration();

    read_first_frame();
    seek_video(std::chrono::milliseconds(loop_in));

    loaded = true;

    return true;
}

/*
    Returns in milliseconds
*/
int VideoPlayer::get_duration() {
	return av_format_ctx->duration / 1000;
}

void VideoPlayer::read_first_frame() {
    int response;
    while (av_read_frame(av_format_ctx, av_packet) >= 0) {
        if (av_packet->stream_index != video_stream_index) {
            continue;
        }

        response = avcodec_send_packet(video_av_codec_ctx, av_packet);
        if (response < 0) {
            printf("Failed to decode packet: %s\n", "a");
            return;
        }

        response = avcodec_receive_frame(video_av_codec_ctx, av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            continue;
        }
        else if (response < 0) {
            printf("Failed to decode packet: %s\n", "a");
            return;
        }

        av_packet_unref(av_packet);
        break;
    }

    if (sws_scaler_ctx == NULL) {
        set_sws_context(video_av_codec_ctx->width, video_av_codec_ctx->height, video_av_codec_ctx->pix_fmt);
    }

    long frame_time = (long)(av_frame->pts * av_q2d(av_format_ctx->streams[video_stream_index]->time_base) * 1000);

    position = frame_time;

    if (clock == nullptr) {
        clock = new Clock();
        play();
    }
}

void VideoPlayer::play() {
    if (!loaded) {
		return;
    }
    clock->set_base_time(std::chrono::milliseconds(position));
    clock->start();
    state = VIDEO_PLAYER_STATES::PLAYING;
}

void VideoPlayer::pause() {
    state = VIDEO_PLAYER_STATES::PAUSED;
}

void VideoPlayer::stop()
{
	if( !loaded )
	{
		return;
	}
    state = VIDEO_PLAYER_STATES::STOPPED;
    seek_video(std::chrono::milliseconds(loop_in));
}


bool VideoPlayer::get_video_frame( int* width_out, int* height_out, unsigned char** data_out, bool force )
{
	if( !loaded )
	{
		return false;
	}
    if (state != VIDEO_PLAYER_STATES::PLAYING && !force) {
        // sleep during one frame
        clock->wait_until(std::chrono::milliseconds((long)av_q2d(av_format_ctx->streams[video_stream_index]->time_base) * 1000 * 1000));
        return false;
    }

    if (position < loop_in || position >= loop_out) {
        seek_video(std::chrono::milliseconds(loop_in));
    }

    int response;
	while(av_read_frame( av_format_ctx, av_packet ) >= 0 )
	{
        if (av_packet->stream_index != video_stream_index) {
            continue;
        }

        response = avcodec_send_packet(video_av_codec_ctx, av_packet);
        if (response < 0) {
            printf("Failed to decode packet: %s\n", "a");
            return false;
        }

        response = avcodec_receive_frame(video_av_codec_ctx, av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            continue;
        }
        else if (response < 0) {
            printf("Failed to decode packet: %s\n", "a");
            return false;
        }

        av_packet_unref(av_packet);
        break;
	}

    if (sws_scaler_ctx == NULL) {
        set_sws_context(video_av_codec_ctx->width, video_av_codec_ctx->height, video_av_codec_ctx->pix_fmt);
    }

    long frame_time = (long)(av_frame->pts * av_q2d(av_format_ctx->streams[video_stream_index]->time_base) * 1000);

    clock->wait_until( std::chrono::milliseconds( frame_time ) );

	position = frame_time + av_frame->pkt_duration; // Position is frame_time + frame duration.

    uint8_t* dest[4] = { video_data, NULL, NULL, NULL };
    int dest_linesize[4] = { av_frame->width * 4, 0, 0, 0 };
    sws_scale(sws_scaler_ctx, av_frame->data, av_frame->linesize, 0, av_frame->height, dest, dest_linesize);

    *width_out = av_frame->width;
    *height_out = av_frame->height;
    *data_out = video_data;

    return true;
}

void VideoPlayer::seek_video( std::chrono::milliseconds seek_time )
{
	if( !loaded )
	{
		return;
	}

    int64_t timestamp = seek_time.count() / 1000 / av_q2d(av_format_ctx->streams[video_stream_index]->time_base);
    av_seek_frame(av_format_ctx, video_stream_index, timestamp, AVSEEK_FLAG_BACKWARD);
    clock->set_base_time(seek_time);
    clock->set_starting_time(std::chrono::high_resolution_clock::now());
    avcodec_flush_buffers(video_av_codec_ctx);

    int response;
    long frame_time = loop_in;
    while (av_read_frame(av_format_ctx, av_packet) >= 0) {
        if (av_packet->stream_index != video_stream_index) {
            continue;
        }

        response = avcodec_send_packet(video_av_codec_ctx, av_packet);
        if (response < 0) {
            printf("Failed to decode packet: %s\n", "a");
            return;
        }

        response = avcodec_receive_frame(video_av_codec_ctx, av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            continue;
        }
        else if (response < 0) {
            printf("Failed to decode packet: %s\n", "a");
            return;
        }

        frame_time = (long)(av_frame->pts * av_q2d(av_format_ctx->streams[video_stream_index]->time_base) * 1000);

        if (frame_time < seek_time.count()) {
            continue;
        }

        av_packet_unref(av_packet);
        break;
    }

    position = frame_time;
}

bool VideoPlayer::set_sws_context(int width, int height, AVPixelFormat pix_fmt) {
    sws_scaler_ctx = sws_getContext(width, height, pix_fmt, width, height, AV_PIX_FMT_RGB0, SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_scaler_ctx) {
        printf("Couldn't initialize sw scaler\n");
        return false;
    }
    return true;
}

void VideoPlayer::reset_variables() {
    sws_freeContext(sws_scaler_ctx);
    sws_scaler_ctx = NULL;
    avformat_close_input(&av_format_ctx);
    avcodec_free_context(&video_av_codec_ctx);
    avcodec_free_context(&audio_av_codec_ctx);

    loop_in = 0;
    loop_out = INT_MAX;
    loaded = false;
}

void VideoPlayer::free() {
    reset_variables();
    avformat_free_context(av_format_ctx);
}

