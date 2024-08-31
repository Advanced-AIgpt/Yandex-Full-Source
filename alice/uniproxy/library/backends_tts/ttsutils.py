import sys
import re
import hashlib
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.protos.ttscache_pb2 import TtsCacheRecord
from alice.uniproxy.library.utils.experiments import conducting_experiment


def get_language(input_lang: str):
    for pattern, lang in config["ttsserver"]["langs"]:
        if pattern.match(input_lang):
            return lang
    return None


def format2mime(options):
    audio_format = options.get("format", "opus").lower()
    quality = options.get("quality", "high").lower()

    if audio_format == "pcm":
        return "audio/x-pcm;bit=16;rate=%s" % ({
            "low": 8000,
            "high": 16000,
            "ultrahigh": 48000,
        }.get(quality, 16000),)
    else:
        return {
            "wav": "audio/x-wav",
            "spx": "audio/x-speex",
            "opus": "audio/opus",
        }.get(audio_format, "opus")


def mime2format(options):
    tmp = options.copy()
    format_line = options.get("format", "opus").lower()
    if "pcm" in format_line:
        tmp["format"] = "Pcm"
        if "48000" in format_line:
            tmp["quality"] = "UltraHigh"
        elif "16000" in format_line:
            tmp["quality"] = "High"
        elif "8000" in format_line:
            tmp["quality"] = "Low"
        else:
            tmp["quality"] = options.get("quality", "High")
    elif "wav" in format_line:
        tmp["format"] = "Wav"
        tmp["quality"] = options.get("quality", "High")
    elif "spx" in format_line or "speex" in format_line:
        tmp["format"] = "Spx"
        tmp["quality"] = options.get("quality", "High")
    else:
        tmp["format"] = "Opus"
        tmp["quality"] = options.get("quality", "UltraHigh")
    return tmp


G_PARAM_REGEXPS = {
    "voice": re.compile(r"""voice=['"]([^'"]*)['"]"""),
    "lang": re.compile(r"""lang=['"]([^'"]*)['"]"""),
    "volume": re.compile(r"""volume=['"]([^'"]*)['"]"""),
    "speed": re.compile(r"""speed=['"]([^'"]*)['"]"""),
    "is_whisper": re.compile(r"""is_whisper=['"]([^'"]*)['"]"""),
    "audio": re.compile(r"""audio=['"]([^'"]*)['"]"""),
    "effect": re.compile(r"""effect=['"]([^'"]*)['"]"""),
    "emotion": re.compile(r"""emotion=['"]([^'"]*)['"]"""),
    # "background": re.compile(r"""background=['"]([^'"]*)['"]"""),
}


def _get_speaker_params(speaker_tag, settings):
    res = {}
    for param, regexp in G_PARAM_REGEXPS.items():
        tmp = regexp.search(speaker_tag)
        if tmp:
            tmp = tmp.groups()
            if tmp and tmp[0]:
                res[param] = tmp[0]
    if "volume" in res:
        try:
            res["volume"] = float(res["volume"])
        except ValueError:
            del res["volume"]
    if "speed" in res:
        try:
            res["speed"] = float(res["speed"])
        except ValueError:
            del res["speed"]
    if "is_whisper" in res:
        if settings and settings.get('tts_allow_whisper', False):
            try:
                res["is_whisper"] = (res["is_whisper"].lower() == "true")
            except ValueError:
                del res["is_whisper"]
        else:
            del res["is_whisper"]
    return res


def split_text_by_speaker_tags(what, settings=None):
    res = []
    while what:
        if "<speaker" not in what:
            res.append({"text": what.strip()})
            break

        tag_pos = what.index("<speaker")

        if ">" not in what[tag_pos:]:
            res.append({"text": what.strip()})
            break

        if tag_pos:
            res.append({"text": what[:tag_pos].strip()})
            what = what[tag_pos:]

        tag = what[:what.index(">")]
        what = what[what.index(">") + 1:]

        params = _get_speaker_params(tag, settings)
        if "<speaker" in what:
            tag_end = what.index("<speaker")
            params["text"] = what[:tag_end].strip()
            res.append(params)
            what = what[tag_end:]
        else:
            params["text"] = what.strip()
            res.append(params)
            break
    return res or [{"text": u""}]


def _get_string_for_cache_key(params, speed, volume):
    parts = [
        str(params.get("lang", "ru")),
        str(params.get("voice", "shitova")),
        str(speed),
        str(params.get("emotion", "neutral")),
        str(params.get("quality", "UltraHigh")),
        str(params.get("format", "Opus")),
        str(volume),
        str(params.get("effect", "")),
    ]

    if params.get('is_whisper', False):
        parts.append('whisper')

    return '_'.join(parts)


def cache_key(params):
    try:
        speed = float(params.get("speed", 1.0))
    except ValueError:
        speed = 1.0
    try:
        volume = float(params.get("volume", 1.0))
    except ValueError:
        volume = 1.0
    return "%s_%s_%s" % (
        config["tts"].get("cache", {}).get("prefix", ""),
        hashlib.md5((_get_string_for_cache_key(params, speed, volume)).lower().encode("utf8")).hexdigest(),
        hashlib.md5(params.get("text", "").lower().encode("utf8")).hexdigest(),
    )


def generate_wav_header(sample_rate, channels=1, data_size=0):
    header = b"RIFF\xff\xff\xff\xffWAVEfmt \x10\x00\x00\x00\x01\x00" + bytes((channels,)) + b"\x00"
    wav_rate = b""
    wav_rate_align = b""
    sample_rate_align = sample_rate * 2 * channels
    for _ in range(0, 4):
        if sys.version_info[0] == 3:
            wav_rate += bytes([sample_rate % 256])  # sample_rate as int32
            wav_rate_align += bytes([sample_rate_align % 256])  # sample_rate * block_align (2 for mono) as int32
        else:
            wav_rate += chr(sample_rate % 256)  # sample_rate as int32
            wav_rate_align += chr(sample_rate_align % 256)  # sample_rate * block_align (2 for mono) as int32
        sample_rate //= 256
        sample_rate_align //= 256
    header += wav_rate
    header += wav_rate_align
    header += bytes((channels * 2,))  # block_align
    header += b"\x00\x10\x00data\xff\xff\xff\xff"

    if data_size > 0:
        size_of_wav = data_size + 36
        hex_wav_size = b""
        hex_data_size = b""
        for _ in range(0, 4):
            if sys.version_info[0] == 3:
                hex_wav_size += bytes([size_of_wav % 256])
                hex_data_size += bytes([data_size % 256])
            else:
                hex_wav_size += chr(size_of_wav % 256)
                hex_data_size += chr(data_size % 256)
            size_of_wav //= 256
            data_size //= 256
        header = header[:4] + hex_wav_size + header[8:40] + hex_data_size

    return header


def channels_from_mime(mime):
    channels = re.findall(r"channels=(\d+)", mime)
    if channels and len(channels) == 1:
        try:
            return int(channels[0])
        except Exception:
            return 1
    else:
        return 1


def tts_response_to_cache(data: bytes, lookup_rate: float, timings=None, duration=None) -> bytes:
    record = TtsCacheRecord(audio_data=data, lookup_rate=lookup_rate)
    if timings:
        record.timings.extend(timings)
    if duration:
        record.duration = duration
    return record.SerializeToString()


def tts_response_from_cache(cached_data: bytes) -> TtsCacheRecord:
    record = TtsCacheRecord()
    record.ParseFromString(cached_data)
    return record


def override_voice_if_needed(payload):
    # WARNING: Order of overriding is important
    if conducting_experiment('enable_tts_gpu', payload):
        voice = payload.get("voice")
        if voice is None or "shitova" in voice:
            payload["voice"] = "shitova.gpu"

    if payload.get('is_whisper', False) and payload.get('voice', "") == "shitova.gpu":
        payload['voice'] = 'shitova_whisper.gpu'
