#include "stream_decoder.hpp"
#include "stream_decoder.h"

#include <stdio.h>
#include <string.h>

#include <chrono>
#include <condition_variable>
#include <fstream>
#include <queue>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include <contrib/libs/ffmpeg-3/libavfilter/avfilter.h>
#include <contrib/libs/ffmpeg-3/libavcodec/avcodec.h>
#include <contrib/libs/ffmpeg-3/libavformat/avformat.h>


#include <contrib/libs/ffmpeg-3/libavfilter/avfiltergraph.h>
#include <contrib/libs/ffmpeg-3/libavfilter/buffersink.h>
#include <contrib/libs/ffmpeg-3/libavfilter/buffersrc.h>
#include <contrib/libs/ffmpeg-3/libavutil/error.h>
#include <contrib/libs/ffmpeg-3/libavutil/samplefmt.h>
#include <contrib/libs/ffmpeg-3/libavutil/opt.h>
#include <contrib/libs/ffmpeg-3/libavutil/log.h>
#include <contrib/libs/ffmpeg-3/libavutil/channel_layout.h>
}

using namespace std;

//#define DBGCOUT(v) cout << v << endl << flush
#define DBGCOUT(v)

namespace {
    // arcadia style helper (&v.front() -> ~v)
    char * operator~(std::vector<char>& v) {
        return &v.front();
    }

    // whence: SEEK_SET, SEEK_CUR, SEEK_END (like fseek) and AVSEEK_SIZE
    int64_t seekFunc(void* /*ptr*/, int64_t /*pos*/, int /*whence*/) {
        // empty plugin for avio_alloc_context()
        return -1;
    }
}

//******************************************************************************

// resampler (PCM rate convertor)
class AudioOutputFormatter {
public:
    AudioOutputFormatter()
        : m_strBuf(512)
    {}
    ~AudioOutputFormatter() {
        avfilter_graph_free(&m_filterGraph);
    }

    int init(AVStream *audio_st, unsigned sample_rates = 8000);
    int writeFrame(AVFrame *frame);
    int readFrame(AVFrame *frame);

private:
    vector<char> m_strBuf;
    AVFilterGraph *m_filterGraph = nullptr;
    AVFilterContext *m_abufferCtx = nullptr;
    AVFilterContext *m_aformatCtx = nullptr;
    AVFilterContext *m_abuffersinkCtx = nullptr;
};


// hide here all filters (PCM rate convertor) logic
int AudioOutputFormatter::init(AVStream *audio_st, unsigned sample_rates) {
    // create new graph
    avfilter_graph_free(&m_filterGraph);
    m_filterGraph = avfilter_graph_alloc();
    if (!m_filterGraph) {
        av_log(NULL, AV_LOG_ERROR, "unable to create filter graph: out of memory\n");
        return -1;
    }

    AVFilter *abuffer = avfilter_get_by_name("abuffer");
    AVFilter *aformat = avfilter_get_by_name("aformat");
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");

    int err;
    // create abuffer filter
    AVCodecContext *avctx = audio_st->codec;
    AVRational time_base = audio_st->time_base;

    snprintf(~m_strBuf, m_strBuf.size(),
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
            time_base.num, time_base.den, avctx->sample_rate,
            av_get_sample_fmt_name(avctx->sample_fmt),
            avctx->channel_layout);
    // fprintf(stderr, "abuffer: %s\n", strBuf_);
    err = avfilter_graph_create_filter(&m_abufferCtx, abuffer,
            NULL, ~m_strBuf, NULL, m_filterGraph);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error initializing abuffer filter\n");
        return err;
    }

    // create aformat filter
    snprintf(~m_strBuf, m_strBuf.size(),
            "sample_fmts=%s:sample_rates=%d:channel_layouts=0x%" PRIx64,
            av_get_sample_fmt_name(AV_SAMPLE_FMT_S16), sample_rates,
            (uint64_t)AV_CH_LAYOUT_MONO);
    // fprintf(stderr, "aformat: %s\n", strBuf_);
    err = avfilter_graph_create_filter(&m_aformatCtx, aformat,
            NULL, ~m_strBuf, NULL, m_filterGraph);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "unable to create aformat filter\n");
        return err;
    }

    // create abuffersink filter
    err = avfilter_graph_create_filter(&m_abuffersinkCtx, abuffersink,
            NULL, NULL, NULL, m_filterGraph);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "unable to create aformat filter\n");
        return err;
    }

    // connect inputs and outputs
    err = avfilter_link(m_abufferCtx, 0, m_aformatCtx, 0);
    if (err >= 0) err = avfilter_link(m_aformatCtx, 0, m_abuffersinkCtx, 0);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error connecting filters\n");
        return err;
    }

    err = avfilter_graph_config(m_filterGraph, NULL);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "error configuring the filter graph\n");
        return err;
    }
    return 0;
}

int AudioOutputFormatter::writeFrame(AVFrame *frame) {
    return av_buffersrc_write_frame(m_abufferCtx, frame);
}

int AudioOutputFormatter::readFrame(AVFrame *frame) {
    return av_buffersink_get_frame(m_abuffersinkCtx, frame);
}

//******************************************************************************

void StreamDecoder::initGlobalEnviroment(int logLevel) {
    DBGCOUT("StreamDecoder::initGlobalEnviroment log_level=" << logLevel);
    // Register all available file formats and codecs
    av_register_all();
    // Register all filters
    avfilter_register_all();
    // more verbosive errors
    av_log_set_level(logLevel);
}

StreamDecoder::~StreamDecoder() {
    DBGCOUT("~StreamDecoder");
    m_inputDataQueue.write("", 0); //EOF for unblock read/join
    m_thread.join();

    av_free(m_avioContext);
    av_frame_free(&m_frame);
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
    }
}

namespace {

class PacketHolder {
public:
    PacketHolder(AVPacket *p)
        : packet_(p)
    {}
    ~PacketHolder() {
        av_free_packet(packet_);
    }
private:
    AVPacket *packet_;
};

class CodecHolder {
public:
    CodecHolder(AVCodecContext *cc)
        : codec_(cc)
    {}
    ~CodecHolder() {
        avcodec_close(codec_);
    }
private:
    AVCodecContext *codec_;
};

}


StreamDecoder::ProbeInputBuffer::ProbeInputBuffer()
    : m_bufferSize(256)
    , m_buffer(m_bufferSize + AVPROBE_PADDING_SIZE)
{ }


StreamDecoder::StreamDecoder(unsigned sample_rate)
    : sampleRate_(sample_rate)
{
    DBGCOUT("StreamDecoder");
    m_inputDataQueue.m_chainedToDataQueue = &m_outputDataQueue;
    m_outputDataQueue.m_chainedFromDataQueue = &m_inputDataQueue;
    m_thread = std::thread([this] {this->processor();});
}

bool StreamDecoder::getError(std::string* err) {
    std::lock_guard<std::mutex> m(m_errorMutex);
    if (m_error.size()) {
        if (err) {
            *err = m_error;
        }
        return true;
    }
    return false;
}

int StreamDecoder::readFunc(void* ptr, uint8_t* buf, int buf_size_i) {
    StreamDecoder* self = reinterpret_cast<StreamDecoder*>(ptr);
    size_t buf_size = size_t(buf_size_i);

    ProbeInputBuffer *probe = &self->m_probeInputBuffer;
    if (probe->m_bufferPos < probe->m_bufferSize) {
        size_t left_bytes = probe->m_bufferSize - probe->m_bufferPos;
        size_t can_read = left_bytes > buf_size ? buf_size : left_bytes;
        memcpy(buf, &probe->m_buffer[probe->m_bufferPos], can_read);
        probe->m_bufferPos += can_read;
        probe->m_streamPos += can_read;
        DBGCOUT("readFunc: Buffer = " << can_read);
        return can_read;
    }
    size_t res = self->m_inputDataQueue.read((char*)buf, buf_size);
    if (res == 0) {
        // successfully finished stream
        DBGCOUT("readFunc: EOF");
        return AVERROR_EOF;
    }
    // DBGCOUT("readFunc");
    probe->m_streamPos += res; // debug purpose counter
    return static_cast<int>(res);
}

bool StreamDecoder::test() {
    return DataQueue::test();
}

void StreamDecoder::processor() {
    DBGCOUT("StreamDecoder::processor...");
    ProbeInputBuffer& probe = this->m_probeInputBuffer;
    size_t n = 0, need_data = probe.m_bufferSize;
    do {
        size_t res = m_inputDataQueue.read(~probe.m_buffer + n, need_data);
        DBGCOUT("fill probe input buffer: " << res);
        if (!res) {
            setError("EOF while collect probe buffer");
            return;
        }
        n += res;
        need_data -= res;
    } while (need_data);

    // need detect stream format before call avformat_open_input (main method)
    AVProbeData probe_data;
    probe_data.filename = "";
    probe_data.buf = (unsigned char *)~probe.m_buffer;
    probe_data.buf_size = probe.m_bufferSize;
    probe_data.mime_type = nullptr;
    AVInputFormat *iformat = av_probe_input_format(&probe_data, 1);
    if (!iformat) {
        setError("can't recognize input audio format");
        return;
    }
    // tiny cleanup
    probe_data.buf = NULL;

    // Create internal buffer for reading input:
    vector<char> avioCtxBuffer(32 * 1024);

    // Allocate the AVIOContext:
    // The fourth parameter (pStream) is a user parameter which will be passed to our callback functions
    m_avioContext = avio_alloc_context(
        (unsigned char *)~avioCtxBuffer, avioCtxBuffer.size(),  // internal Buffer and its size
        0,                  // bWriteable (1=true,0=false)
        this,               // user data ; will be passed to our callback functions
        readFunc,
        0,                  // Write callback function (not used in this example)
        seekFunc);
    m_avioContext->seekable = 0;

    m_formatContext = avformat_alloc_context();
    // Set the IOContext (with read() callback)
    m_formatContext->pb = m_avioContext;
    m_formatContext->iformat = iformat;
    m_formatContext->probesize = m_probeInputBuffer.m_bufferSize;
    m_formatContext->max_analyze_duration = m_probeInputBuffer.m_bufferSize;

    DBGCOUT("avformat_open_input...");
    int err = avformat_open_input(&m_formatContext, "filename", NULL, NULL);
    if (err < 0) {
        setError("ffmpeg: Unable to open input file");
        return;
    }

    if (0) {
        printf("Get stream info\n");
        // Retrieve stream information
        err = avformat_find_stream_info(m_formatContext, NULL);
        if (err < 0) {
            setError("ffmpeg: Unable to find stream info");
            return;
        }

        printf("Dump format\n");
        // Dump information about file onto standard error
        av_dump_format(m_formatContext, 0, "", 0);
    }

    // Find the first video stream
    unsigned audio_stream;
    for (audio_stream = 0; audio_stream < m_formatContext->nb_streams; ++audio_stream) {
        if (m_formatContext->streams[audio_stream]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            break;
        }
    }
    if (audio_stream == m_formatContext->nb_streams) {
        setError("ffmpeg: Unable to find audio stream");
        return;
    }

    AVStream *audio_st = m_formatContext->streams[audio_stream];
    audio_st->discard = AVDISCARD_DEFAULT;

    AVCodecContext* codec_context = audio_st->codec;
    // helper for free struct
    CodecHolder codecHolder(codec_context);
    AVCodec* codec = avcodec_find_decoder(codec_context->codec_id);
    err = avcodec_open2(codec_context, codec, NULL);
    if (err < 0) {
        setError("ffmpeg: Unable to open codec");
        return;
    }
    AudioOutputFormatter outputFormatter;
    if (outputFormatter.init(audio_st, sampleRate_) < 0) {
        setError("ffmpeg: unable to init filter graph");
        return;
    }
    DBGCOUT("input audio channels: " << codec_context->channels);
    m_frame = av_frame_alloc();
    AVPacket packet;
    int read_frame_res = 0;
    while (true) {
        read_frame_res = av_read_frame(m_formatContext, &packet);
        if (read_frame_res < 0) {
            break;
        }
        // Free the packet that was allocated by av_read_frame
        PacketHolder packetHolder(&packet);

        if ((unsigned)packet.stream_index == audio_stream) {
            // Audio stream packet
            // DBGCOUT("Next frame " << packet.size << " " << packet.duration);
            int frame_finished;
            int len = avcodec_decode_audio4(codec_context, m_frame, &frame_finished, &packet);
            if (len < 0) {
                setError("Error while decoding");
                return;
            }
            if (frame_finished) {
                // push the audio data from decoded frame into the filtergraph
                int err = outputFormatter.writeFrame(m_frame);
                if (err < 0) {
                    setError("error writing frame to buffersrc");
                    return;
                }
            } else {
                av_log(NULL, AV_LOG_ERROR, "frame not finished\n");
                break;
            }
            // pull filtered audio from the filtergraph
            for (;;) {
                int err = outputFormatter.readFrame(m_frame);
                if (err == AVERROR_EOF || err == AVERROR(EAGAIN))
                    break;
                if (err < 0) {
                    setError("error reading buffer from buffersink");
                    return;
                }

                int nb_channels = av_get_channel_layout_nb_channels(m_frame->channel_layout);
                int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)m_frame->format);
                int data_size = m_frame->nb_samples * nb_channels * bytes_per_sample;
                m_outputDataQueue.write((char *)m_frame->data[0], data_size);
            }
        }
    }
    if (read_frame_res != AVERROR_EOF) {
        vector<char> ebuff(256);
        if (!av_strerror(read_frame_res, ~ebuff, ebuff.size())) {
            DBGCOUT("StreamDecoder::process: end reading: " << read_frame_res
                    << " " << AVERROR_EOF);
        } else if (!strerror_r(AVUNERROR(read_frame_res), ~ebuff, ebuff.size())) {
            ;
        } else {
            snprintf(~ebuff, ebuff.size(), "av_read_frame return %d", read_frame_res);
        }
        setError(~ebuff);
        return;
    }
    m_outputDataQueue.write("", 0); //EOF
}

void StreamDecoder::setError(const std::string& err) {
    DBGCOUT("setError: " << err);
    {
        std::lock_guard<std::mutex> m(m_errorMutex);
        m_error = err;
    }
    m_outputDataQueue.write("", 0); //EOF
}

void StreamDecoder::DataQueue::write(const char* buf, size_t buf_size) {
    auto data = data_ptr(new vector<char>(buf, buf + buf_size));
    unique_lock<mutex> m(m_mutex);
    m_dataQueue.push(data);
    m_skip_wait = true;
    m_event.notify_one();
}

size_t StreamDecoder::DataQueue::read(char *buf, size_t size) {
    data_ptr data;
    size_t shift = 0;
    {
        // mutex locking order always MUST be:
        // 1. lock chainged _from_
        // 2. lock chained _to_
        unique_lock<mutex> m(m_mutex);
        chrono::seconds dur(1);
        while (!m_dataQueue.size()) {
            bool inputChainedQueueBlocked = false;
            if (m_chainedFromDataQueue) {
                m.unlock(); // <<< avoid wrong locking order
                inputChainedQueueBlocked = m_chainedFromDataQueue->is_blocked();
                m.lock();
            }
            if (m_skip_wait) {
                m_skip_wait = false;
                continue;
            }
            if (m_dataQueue.size()) {
                break;
            }
            if (inputChainedQueueBlocked) {
                // not have any ready data and input queue blocked, so need try again later
                return 0;
            }
            // not have any ready data, but input source now not blocked so wait result from it
            m_blocked = true;
            if (m_chainedToDataQueue) {
                 // transmit blocked status to next queue (for unblocking if need)
                m_chainedToDataQueue->signal();
            }
            m_event.wait_for(m, dur);
            m_blocked = false;
        }
        data = m_dataQueue.front();
        if (data->empty()) {
            DBGCOUT("read EOF marker");
            m_eof = true;
            return 0;
        }
        shift = m_dataShift;
        if (data->size() - m_dataShift <= size) {
            // current data content less than size
            size = data->size() - m_dataShift;
            m_dataQueue.pop();
            m_dataShift = 0;
        } else {
            m_dataShift += size;
        }
    }
    memcpy(buf, &data->front() + shift, size);
    return size;
}

bool StreamDecoder::DataQueue::test() {
    stringstream ss;
    for (char i = 1; i < 127; ++i) {
        ss.write(&i, 1);
    }
    const string sample = ss.str();
    DataQueue dq;
    const size_t rdsz[] = {1, 3, 2};
    for (size_t i = 0; i < sample.size();) {
        size_t useBytes = 1;
        if (i < sample.size() - 3) {
            useBytes = rdsz[i % 3];
        }
        dq.write(&sample[i], useBytes);
        i += useBytes;
    }
    char buf[3];
    dq.write(buf, 0);
    size_t useBytes = 2;
    stringstream sso;
    while (size_t rd = dq.read(buf, useBytes)) {
        sso.write(buf, rd);
    }
    if (sample != sso.str()) {
        cerr << "StreamDecoder::DataQueue corrupt stream" << endl << flush;
        return false;
    }
    return true;
}

//
// C api (for build .so and call from python using ctypes module)
//
extern "C" {

StreamDecoderPtr createStreamDecoder(unsigned sample_rate) {
    static bool initGlobalEnv = true;
    if (initGlobalEnv) {
        initGlobalEnv = false;
        StreamDecoder::initGlobalEnviroment();
    }

    try {
        return new StreamDecoder(sample_rate);
    } catch (const std::exception& exc) {
        std::cerr << "createStreamDecoder failed: " << exc.what() << std::endl;
    } catch (...) { }
    return nullptr;
}

void destroyStreamDecoder(StreamDecoderPtr sd) {
    if (sd) {
        delete reinterpret_cast<StreamDecoder*>(sd);
    }
}

void streamDecoderWrite(StreamDecoderPtr sd, const char *buf, size_t size) {
    reinterpret_cast<StreamDecoder*>(sd)->write(buf, size);
}

size_t streamDecoderRead(StreamDecoderPtr sd, char *buf, size_t size) {
    return reinterpret_cast<StreamDecoder*>(sd)->read(buf, size);
}

int streamDecoderEof(StreamDecoderPtr sd) {
    return reinterpret_cast<StreamDecoder*>(sd)->eof();
}

int streamDecoderGetError(StreamDecoderPtr sd, char* err, size_t err_limit, size_t* err_len) {
    if (err && err_len) {
        string s;
        bool has_err = reinterpret_cast<StreamDecoder*>(sd)->getError(&s);
        strncpy(err, s.c_str(), err_limit);
        *err_len = strlen(err);
        return has_err;
    } else {
        return reinterpret_cast<StreamDecoder*>(sd)->getError();
    }
}

}  // extern "C"
