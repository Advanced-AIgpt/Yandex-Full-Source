from .ttsutils import format2mime
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.backends_common.httpstream import HttpStream
from alice.uniproxy.library.global_counter import GlobalCounter


class AudioPlayer(object):
    def __init__(self, on_data, on_error, params):
        self.converter = None
        self.request = None
        self.on_data = on_data
        self.on_error = on_error
        self.params = params
        self.audio_name = self.params["audio"]
        self.skip_wav_header = False
        target_format = format2mime(params)

        if self.audio_name.endswith(".wav"):
            source_format = "audio/x-pcm;bit=16;rate=48000"
            self.skip_wav_header = True
        elif self.audio_name.endswith(".opus"):
            source_format = "audio/opus"
        else:
            self.on_error(self.audio_name + " is not supported audio file type.")
            return

        if target_format != source_format:
            self.on_error("Format {} is not supported".format(target_format))
            return

        self.request = HttpStream(
            config["tts"]["s3_url"] % (
                self.audio_name,
            ),
            on_result=self._on_http_data,
            on_error=self._on_error,
            request_timeout=config["tts"]["s3_timeout"]
        )

    def close(self):
        if self.request:
            self.request.close()
            self.request = None

    def _on_error(self, message):
        GlobalCounter.S3_AUDIO_ERR_SUMM.increment()
        if self.on_error:
            self.on_error(message)

    def _on_http_data(self, result):
        GlobalCounter.S3_AUDIO_OK_SUMM.increment()
        data = result.body
        if self.skip_wav_header:
            data = data[44:]
        if self.converter:
            self.converter.add_data(data)
            self.converter.add_data(b"")
        else:
            self.on_data(data)
            self.on_data(b"")
