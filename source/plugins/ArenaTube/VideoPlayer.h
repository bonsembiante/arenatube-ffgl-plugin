#include "Clock.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <inttypes.h>
}

enum VIDEO_PLAYER_STATES : int {
    PLAYING,
    STOPPED,
    PAUSED,
};

class VideoPlayer {
public:
    VideoPlayer();
    bool open_file(const char* filename);
    void read_first_frame();
    bool get_video_frame(int* width_out, int* height_out, unsigned char** data_out, bool force = false);
    bool set_sws_context(int width, int height, AVPixelFormat pix_fmt);
    void seek_video(std::chrono::milliseconds seek_time);
    void reset_variables();
    void free();
	int get_duration();

    void play();
    void pause();
    void stop();

    Clock* clock = nullptr;
    long position = 0;
    int state = VIDEO_PLAYER_STATES::STOPPED;
    bool loop = true;
    int loop_in;
    int loop_out;
    bool loaded = false;

private:

    AVFormatContext* av_format_ctx = NULL;

    int video_stream_index = -1;
    AVCodecParameters* video_av_codec_params = NULL;
    AVCodec* video_av_codec = NULL;
    AVCodecContext* video_av_codec_ctx = NULL;
    uint8_t* video_data = nullptr;

    int audio_stream_index = -1;
    AVCodecParameters* audio_av_codec_params = NULL;
    AVCodec* audio_av_codec = NULL;
    AVCodecContext* audio_av_codec_ctx = NULL;

    AVFrame* av_frame = av_frame_alloc();
    AVPacket* av_packet = av_packet_alloc();

    SwsContext* sws_scaler_ctx = NULL;
};

