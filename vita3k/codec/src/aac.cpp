#include <codec/state.h>

#define DEBUG

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <util/log.h>

static void convert_f32_to_s16(const float *f32, int16_t *s16, uint32_t channels, uint32_t samples, uint32_t freq) {
    const int channel_type = channels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;

    SwrContext *swr = swr_alloc_set_opts(nullptr,
        channel_type, AV_SAMPLE_FMT_S16, freq,
        channel_type, AV_SAMPLE_FMT_FLTP, freq,
        0, nullptr);
    swr_init(swr);

    const int result = swr_convert(swr, (uint8_t **)&s16, samples, (const uint8_t **)(f32), samples);
    swr_free(&swr);
    assert(result > 0);
}

uint32_t AacDecoderState::get(DecoderQuery query) {
    switch (query) {
    case DecoderQuery::CHANNELS: return context->channels;
    case DecoderQuery::BIT_RATE: return context->bit_rate;
    case DecoderQuery::SAMPLE_RATE: return context->sample_rate;
    default:
        return 0;
    }
}

bool AacDecoderState::send(const uint8_t *data, uint32_t size) {
    AVPacket *packet = av_packet_alloc();
    packet->data = const_cast<uint8_t *>(data);
    packet->size = size;

    int err = avcodec_send_packet(context, packet);
    av_packet_free(&packet);
    if (err < 0) {
        LOG_WARN("Error sending Aac packet: {}.", codec_error_name(err));
        return false;
    }

    return true;
}

bool AacDecoderState::receive(uint8_t *data, DecoderSize *size) {
    AVFrame *frame = av_frame_alloc();

    int err = avcodec_receive_frame(context, frame);
    if (err < 0) {
        LOG_WARN("Error receiving Aac frame: {}.", codec_error_name(err));
        av_frame_free(&frame);
        return false;
    }

    assert(frame->format == AV_SAMPLE_FMT_FLTP);

    if (data) {
        convert_f32_to_s16(
            reinterpret_cast<float *>(frame->extended_data),
            reinterpret_cast<int16_t *>(data),
            frame->channels, frame->nb_samples,
            frame->sample_rate);
    }

    if (size) {
        size->samples = frame->nb_samples;
    }

    av_frame_free(&frame);
    return true;
}

AacDecoderState::AacDecoderState(uint32_t sample_rate, uint32_t channels) {
    constexpr size_t buffer_size = 8192;

    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    assert(codec);

    context = avcodec_alloc_context3(codec);
    assert(context);

    context->codec_type = AVMEDIA_TYPE_AUDIO;
    context->channels = channels;
    context->sample_rate = sample_rate;
    context->channel_layout = AV_CH_LAYOUT_STEREO;

    int err = avcodec_open2(context, codec, nullptr);
    assert(err == 0);
}
