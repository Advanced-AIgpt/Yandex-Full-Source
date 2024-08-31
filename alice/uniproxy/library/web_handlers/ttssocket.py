import json
import uuid
import time
import re
from functools import partial
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.backends_tts.ttsstream import TtsStream
from alice.uniproxy.library.backends_common.apikey import check_key
from alice.uniproxy.library.common_handlers import CommonRequestHandler, CommonWebSocketHandler


def generateWavHeader(sample_rate, mono=True, data_size=0):
    header = b"RIFF\xff\xff\xff\xffWAVEfmt \x10\x00\x00\x00\x01\x00" + (b"\x01" if mono else b"\x02") + b"\x00"
    wav_rate = b""
    wav_rate_align = b""
    sample_rate_align = sample_rate * 2
    for i in range(0, 4):
        wav_rate += bytes([sample_rate % (256 if mono else 512)])  # sample_rate * block_align (2 for mono) as int32
        wav_rate_align += bytes([sample_rate_align % 256])  # sample_rate as int32
        sample_rate //= 256
        sample_rate_align //= 256
    header += wav_rate
    header += wav_rate_align
    header += b"\x02" if mono else b"\x04"
    header += b"\x00\x10\x00data\xff\xff\xff\xff"

    if data_size > 0:
        size_of_wav = data_size + 36
        hexWavSize = b""
        hexDataSize = b""
        for i in range(0, 4):
            hexWavSize += bytes([size_of_wav % 256])
            hexDataSize += bytes([data_size % 256])
            size_of_wav /= 256
            data_size /= 256
        header = header[:4] + hexWavSize + header[8:40] + hexDataSize

    return header


class TtsSpeakersHandler(CommonRequestHandler):
    unistat_handler_name = 'speakers'

    def get(self):
        Logger.get().debug(u"Start new voices request")

        all_voices = config["ttsserver"]["voices"]
        hidden_voices = config.get("wsproxy", {}).get("ttsVoicesBlacklist", [])

        try:
            self.set_header("Content-Type", "application/json")

            voices = [{
                'name': x,
                'languages': ['ru-RU', 'en-US', 'uk-UA', 'tr-TR'],
                'gender': all_voices[x].get('gender', 2),
                'displayName': all_voices[x].get('displayName', x),
                'coreVoice': all_voices[x].get('coreVoice', True),
                'rate': 48000
            } for x in all_voices if x not in hidden_voices]

            self.write(json.dumps(voices))
            self.finish()
        except Exception as exp:
            self.write({'type': 'Error', 'data': 'Error: ' + str(exp)})
            self.finish()
            return

    def check_origin(self, origin):
        return True


class TtsWebSocket(CommonWebSocketHandler):
    unistat_handler_name = 'ttssocket_ws'

    def open(self):
        self._logger = Logger.get('.ttswebsocket')
        self.stream = None
        self.session_id = str(uuid.uuid4())
        self.index_map = {}
        self.key = ""
        self.status = 200
        self.client_ip = self.request.headers.get(
            'X-Real-Ip',
            self.request.headers.get('X-Forwarded-For', self.request.remote_ip)
        )
        self.start_time = time.monotonic()
        self._logger.debug("Start new connection")

    def get_compression_options(self):
        return {}

    def on_message(self, message):
        self._logger.debug(message)
        try:
            msg = json.loads(message).get('data', {})

            self.rate = {'high': 16000, 'ultra': 48000, 'low': 8000}[msg.get('quality', 'ultra')]

            speed = msg.get('speed', None)
            try:
                speed = float(speed)
            except ValueError:
                speed = 1.0

            volume = msg.get('volume', None)
            try:
                volume = float(volume)
            except ValueError:
                volume = 1.0

            self.payload = {
                "sessionId": self.session_id,
                "uuid": msg.get('uuid', '123'),
                "voice": msg.get("voice", msg.get("speaker", None)),
                "lang": msg.get('lang', 'ru_RU'),
                "text": msg.get('text', ''),
                "emotion": msg.get('emotion', ''),
                "speed": speed,
                "volume": volume,
                "mime": "audio/x-pcm;bit=16;rate=%s" % self.rate,
                "requireMetainfo": True
            }

            self.key = msg.get('apikey') or msg.get('apiKey') or msg.get('key')
            check_key(
                self.key,
                self.client_ip,
                self.generate,
                partial(self.on_error, "Bad apiKey"),
                None
            )
        except Exception as exp:
            self.on_error(exp)

    def generate(self, _):
        try:
            self.index_map = sum(
                map(
                    lambda x: [x[0]] * len(x[1].encode('utf-8')),
                    enumerate(self.payload.get('text', ''))
                ),
                []
            )

            self._logger.debug(self.payload)
            self.stream = TtsStream(
                self.on_data,
                self.on_error,
                self.payload,
                rt_log=None,
                unistat_counter='ws_tts',
            )

            self.write_message({
                'type': "InitResponse",
                'data': {
                    'sessionId': self.session_id,
                    'code': 200
                }
            })

            self.write_message(generateWavHeader(self.rate), binary=True)
        except Exception as exp:
            self.on_error(exp)

    def on_close(self):
        Logger.access(
            "ttssocket.ws?key=%s&requestId=%s&uuid=%s&lang=%s&voice=%s" %
            (
                self.key,
                self.session_id,
                self.payload.get("uuid"),
                self.payload.get("lang"),
                self.payload.get("voice")
            ),
            self.status,
            self.client_ip,
            time.monotonic() - self.start_time
        )
        self._logger.debug("WebSocket closed")
        if self.stream:
            self.stream.close()
            self.stream = None

    def on_data(self, res):
        if True or res.audioData:
            try:
                words = []
                phones = []
                for x in res.phonemes:
                    phones.append((x.positionInBytesStream / 2.0 / 48000, x.ttsPhoneme, x.durationMs * 0.001))

                for x in res.words:
                    words.append(
                        {
                            'word': x.text if x.text != "" else re.match(
                                r'(.*?)($|\s|,|\.|\?|!|:|"|\')',
                                self.payload.get("text", "")[self.index_map[x.firstCharPositionInText]:]
                            ).groups()[0],
                            'from': x.bytesLengthInSignal / 2.0 / 48000,
                            'postag': x.postag,
                            'homographTag': x.homographTag,
                        }
                    )
                if res.words:
                    words.append({'word': 'END OF UTT', 'from': -1, 'postag': '', 'homographTag': ''})

                word_i = 0
                trans = [[] for x in words]
                for p in phones:
                    while word_i < len(words) - 1 and words[word_i]['from'] < 0:
                        word_i += 1
                    phone_fit = word_i == len(words) - 1 or words[word_i + 1]['from'] > p[0] + p[2] * 0.9
                    if (words[word_i]['from'] >= 0 and phone_fit):
                        pass
                    else:
                        word_i += 1
                    trans[word_i].append(p[1])
                self.write_message({
                    'type': 'Phonemes',
                    'data': {
                        'words': words,
                        'transcriptions': trans,
                    }
                })
            except Exception as exc:
                self._logger.warning("Can't write phonemes: %s" % (exc,))
        self.write_message(res.audioData, binary=True)
        if res.completed:
            self.close()

    def on_error(self, error):
        self._logger.error(error)
        try:
            self.write_message({'type': 'Error', 'data': 'Error: ' + str(error)})
            self.close()
        except Exception:
            pass
        if self.stream:
            self.stream.close()
            self.stream = None

    def check_origin(self, origin):
        return True
