cdef extern from "alice/library/python/decoder/stream_decoder.h":
    ctypedef void* StreamDecoderPtr

    StreamDecoderPtr createStreamDecoder(unsigned sample_rate)
    void destroyStreamDecoder(StreamDecoderPtr)
    void streamDecoderWrite(StreamDecoderPtr, const char *buf, size_t size)
    size_t streamDecoderRead(StreamDecoderPtr, char *buf, size_t size)
    int streamDecoderEof(StreamDecoderPtr)
    int streamDecoderGetError(StreamDecoderPtr, char* err, size_t err_limit, size_t* err_len)


class DecoderException(Exception):
    def __init__(self, text):
        Exception(self, text)


cdef class Decoder:
    """
        This class represents audio stream decoder.

        This class decode encoded audio stream to mono PCM format
        (16bit signed samples, with given sample_rate (default=8000)).

        >>> dc = Decoder(16000)
        >>> dc.write(data)
        >>> while True:
        >>>     data = dc.read()
        >>>     if not data:
        >>>         if dc.eof()
        >>>             # handle end of stream
        >>>         break
        >>>     # use decoded data
    """
    
    cdef unsigned _sample_rate
    cdef StreamDecoderPtr decoder

    def __init__(self, sample_rate=16000):
        self._sample_rate = sample_rate
        self.decoder = createStreamDecoder(self._sample_rate)
        if not self.decoder:
            raise Exception('can not create decoder')

    def __dealloc__(self):
        destroyStreamDecoder(self.decoder)

    @property
    def sample_rate(self):
        """
            Sample rate of the decoder.
        """
        return self._sample_rate

    def write(self, data: bytes):
        """
            Write bytes (encoded audio) to decoder. Can raise DecoderException if detected decoding problems.
        """
        err = self.getError()
        if err:
            raise DecoderException(err.decode('utf-8', errors='replace'))
        streamDecoderWrite(self.decoder, data, len(data))

    def read(self) -> bytes:
        """
            Read bytes(decoded audio/PCM).
            Return None on end of stream or insufficient data.
            Check eof() if read() return None for detecting end of output(decoded audio) stream.
            Also can throw exception DecoderException (on bad format input stream, etc).
        """
        cdef char p[16000]
        read_cnt = streamDecoderRead(self.decoder, p, sizeof(p))
        if read_cnt > 0:
            return p[:read_cnt]
        err = self.getError()
        if err:
            raise DecoderException(err.decode('utf-8', errors='replace'))
        return None

    def close(self):
        """
            Mark input (encoded audio) stream to decoder as finished.
        """
        data = b""
        streamDecoderWrite(self.decoder, data, len(data))

    def eof(self):
        """
            Return True on finish stream. Call this method after read() return None for checking stream status.
        """
        return streamDecoderEof(self.decoder) != 0

    def getError(self) -> bytes:
        """
            Return string represents last occured error
        """
        cdef char p[1024]
        cdef size_t err_len 
        res = streamDecoderGetError(self.decoder, p, 1024, &err_len)
        if res != 0:
            return p[:err_len]
