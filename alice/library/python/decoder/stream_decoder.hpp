#include <condition_variable>
#include <queue>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct AVFilterGraph;
struct AVFilterContext;
struct AVFilterBufferRef;
struct AVFormatContext;
struct AVIOContext;
struct AVFrame;
struct AVStream;

class StreamDecoder {
public:
    // log levels https://ffmpeg.org/doxygen/3.0/group__lavu__log__constants.html
    static void initGlobalEnviroment(int logLevel = -8);

    StreamDecoder(unsigned sample_rate = 8000);
    ~StreamDecoder();

    void write(const char *buf, size_t size) {
        return m_inputDataQueue.write(buf, size);
    }
    // return size read or 0 if not has ready data
    // on receiving nil caller need check eof() for detecting end of stream
    size_t read(char *buf, size_t size) {
        return m_outputDataQueue.read(buf, size);
    }
    bool eof() {
        return m_outputDataQueue.eof();
    }
    bool getError(std::string* err = nullptr);

    static int readFunc(void* ptr, uint8_t* buf, int buf_size_i);
    static bool test();

private:
    void setError(const std::string& err);
    void processor();

    unsigned sampleRate_ = 8000;
    struct ProbeInputBuffer {
        ProbeInputBuffer();

        const size_t m_bufferSize;
        std::vector<char> m_buffer;
        size_t m_bufferPos = 0;
        size_t m_streamPos = 0;
    };
    // buff data for codec detection
    ProbeInputBuffer m_probeInputBuffer;
    AVIOContext* m_avioContext = nullptr;
    AVFormatContext* m_formatContext = nullptr;
    AVFrame* m_frame = nullptr;

    using data_ptr = std::shared_ptr<std::vector<char>>;
    class DataQueue {
    public:
        void write(const char *buf, size_t buf_size);
        size_t read(char *buf, size_t buf_size);
        void signal() {
            std::unique_lock<std::mutex> m(m_mutex);
            m_skip_wait = true;
            m_event.notify_one();
        }
        bool is_blocked() {
            std::unique_lock<std::mutex> m(m_mutex);
            return m_blocked && m_dataQueue.empty();
        }
        bool eof() {
            return m_eof;
        }

        static bool test();

        StreamDecoder::DataQueue *m_chainedToDataQueue = 0;
        StreamDecoder::DataQueue *m_chainedFromDataQueue = 0;

    private:
        bool m_skip_wait = false;
        bool m_blocked = false;
        bool m_eof = false;
        std::mutex m_mutex;
        std::queue<data_ptr> m_dataQueue;
        std::condition_variable m_event;
        size_t m_dataShift = 0; // next readed data chunk shift
    };
    DataQueue m_inputDataQueue;
    DataQueue m_outputDataQueue;
    std::thread m_thread;
    std::mutex m_errorMutex;
    std::string m_error;
};
