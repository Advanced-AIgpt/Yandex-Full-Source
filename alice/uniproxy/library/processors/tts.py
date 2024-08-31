import time
from tornado import gen
from tornado.ioloop import IOLoop

from . import EventProcessor, register_event_processor

from alice.uniproxy.library.backends_tts.audioplayer import AudioPlayer
from alice.uniproxy.library.backends_tts.realtimestreamer import create_streamer
from alice.uniproxy.library.backends_tts.ttsstream import TtsStream
from alice.uniproxy.library.events import Directive, StreamControl
from alice.uniproxy.library.extlog import AccessLogger
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import TTS_FIRST_CHUNK_HGRAM
from alice.uniproxy.library.perf_tester import events
from alice.uniproxy.library.perf_tester.events import EventFirstTTSChunkSec
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.utils.deepcopyx import deepcopy
from alice.uniproxy.library.utils.experiments import conducting_experiment, experiment_value
from alice.uniproxy.library.utils.proto_to_json import MessageToDict2

from alice.uniproxy.library.backends_tts.ttsutils import (
    cache_key,
    format2mime,
    get_language,
    mime2format,
    split_text_by_speaker_tags,
    tts_response_from_cache,
    tts_response_to_cache,
)

from alice.uniproxy.library.backends_tts.cache import (
    Cache,
    fetch_cached_tts_stream,
    is_cachable,
    lookup_result_to_global_counters,
    memcache_tts_lookup,
    CacheStorageClient,
    UNIPROXY_FULL_TTS_PRECACHE_EXP,
)


def contains_spotter_text(text):
    if not text:
        return False
    lower = text.lower()
    return ("алиса" in lower or "яндекс" in lower) and "яндекс новост" not in lower


@register_event_processor
class SpeechStarted(EventProcessor):
    def process_event(self, event):
        self.DLOG("Speech started")
        self.close()


@register_event_processor
class SpeechFinished(EventProcessor):
    def process_event(self, event):
        self.DLOG("Speech ended")
        self.close()


class SpeechCanceled(EventProcessor):
    def process_event(self, event):
        self.DLOG("Speech canceled")
        self.close()


@register_event_processor
class ListVoices(EventProcessor):
    def __init__(self, *args, **kwargs):
        self.voices_stream = None
        super(ListVoices, self).__init__(*args, **kwargs)

    def close(self):
        if self.voices_stream:
            self.voices_stream.close()
        super(ListVoices, self).close()

    def process_event(self, event):
        super(ListVoices, self).process_event(event)

        default_languages = [x[1] for x in config["ttsserver"]["langs"]]
        res = []
        for voice, params in config["ttsserver"]["voices"].items():
            res.append({
                "displayName": params.get("displayName", voice),
                "name": voice,
                "coreVoice": params.get("coreVoice", True),
                "gender": params.get("gender"),
                "languages": params.get("lang", default_languages),
                "rate": params.get("sampleRate", 48000)
            })
        if not self.is_closed():
            self.system.write_directive(Directive("TTS", "Voices", res, self.event.message_id))
            self.close()


class UniproxyTTSTimings(object):
    def __init__(self, request_start_time_sec):
        self._request_start_time_sec = request_start_time_sec
        self._dict = {
            EventFirstTTSChunkSec.NAME: None
        }

    def import_events(self, d):
        for k, v in d.items():
            if k in events.TTS_STAGE_BY_NAME:
                self._dict[k] = v

    def on_event(self, event, ts=None):
        if ts is None:
            ts = time.monotonic()
        self._dict[event.NAME] = ts - self._request_start_time_sec

    def on_tts_cache(self, success):
        self._dict[events.EventTtsCacheSuccess.NAME] = success

    def get_event(self, event):
        return self._dict.get(event.NAME)

    def to_dict(self):
        return self._dict

    def to_directive(self, event_id, params=None):
        payload = self.to_dict()
        if params:
            payload['params'] = params

        return Directive(namespace='TTS',
                         name='UniproxyTTSTimings',
                         payload=payload,
                         event_id=event_id)


@register_event_processor
class Generate(EventProcessor):
    def __init__(self, *args, **kwargs):
        super(Generate, self).__init__(*args, **kwargs)
        self.tts_stream = None  # receives audio from TTS backend
        self.http_stream = None
        self.audio_player = None
        self.tts_streamer = None  # sends audio to user
        self.data = b""
        self.first_logger = AccessLogger("tts_first_chunk", self.system, rt_log=self.rt_log)
        self.full_logger = AccessLogger("tts_full", self.system, rt_log=self.rt_log)
        self.body_size = 0
        self.generate_queue = []
        self.stream_payload = {}
        self.initial_stream_payload = {}
        self.on_close = None
        self.on_tts_cache_response = None
        self.on_first_chunk = None
        self.rtf = 0.0
        self.lookups = 0.0
        self.realtime_streamer_cfg = None
        self.disable_spotter = False
        self.enable_bargin = False
        self.enable_lazy_tts_streaming = False
        self.start_time = None

        self.audio_player_rtlog_token = None
        self.tts_stream_rtlog_token = None
        self._cache = None
        self._tts_cache = None
        self.need_timings = None
        self.log_info = {
            'event_birth_time': None,
            'start_time': None,
            'first_chunk_time': None,
            'full_response_time': None,
            'memcache_time': None,
            'stream_control_close_time': None
        }
        self.log_info_written = False
        self.tts_timings = []
        self.current_duration = 0

    def process_streamcontrol(self, streamcontrol):
        if streamcontrol.action == StreamControl.ActionType.CLOSE:
            self.log_info['stream_control_close_time'] = time.monotonic() - self.log_info['start_time']
            if self.tts_streamer:
                self.tts_streamer.abort()
            self.close()
            return EventProcessor.StreamControlAction.Close
        elif streamcontrol.action == StreamControl.ActionType.NEXT_CHUNK:
            if self.tts_streamer:
                self.tts_streamer.next_chunk(streamcontrol.size)
        else:
            # next chunk event
            return EventProcessor.StreamControlAction.Ignore

    def close(self):
        if not self.log_info_written:
            self.log_info_written = True
            self.system.logger.log_directive(
                {
                    "type": "TtsGenerateTimings",
                    "ForEvent": self.event.message_id,
                    "result": self.log_info,
                },
                rt_log=self.rt_log,
            )

        if self.on_close:
            self.on_close()
            self.on_close = None
        if self.audio_player:
            self.audio_player.close()
            self.audio_player = None
        if self.tts_streamer:
            self.tts_streamer.close()
            self.tts_streamer = None
        if self.tts_stream:
            self.tts_stream.close()
            self.tts_stream = None
        if self.http_stream:
            self.http_stream.close()
            self.http_stream = None

        super(Generate, self).close()

    def on_data(self, res, from_cache=None, fallback=False):
        if res and res.completed:
            self.rt_log.log_child_activation_finished(self.tts_stream_rtlog_token, True)

        self.rt_log('tts.generate_response_received',
                    completed=res.completed if res else None,
                    audio_data_size=len(res.audioData) if res and res.audioData is not None else None)

        has_valid_source = from_cache is not None or self.tts_stream
        if not has_valid_source or self.is_closed():
            return

        if self.first_logger:
            self.first_logger.end(code=200, size=0)
            self.first_logger = None

        if self.tts_streamer is None:
            self.send_tts_speak(from_cache=from_cache is True,
                                payload_patch={'disableInterruptionSpotter': self.disable_spotter})

        if res.metainfo and res.metainfo.rtf and res.audioData:
            self.rtf += res.metainfo.rtf * len(res.audioData)
            self.lookups += res.metainfo.lookup * len(res.audioData)

        if res.words:
            self.write_meta(res)

        if res.timings:
            self.write_timings(res.timings)

        # very important do it after write_directives

        duration = None
        if res.duration:
            duration = res.duration
            self.current_duration += duration

        if res.audioData and self.tts_streamer:
            self._update_first_chunk_stats()
            self.body_size += len(res.audioData)
            self.tts_streamer.add_data(res.audioData)
            self.data += res.audioData

        if res.completed:
            if (
                has_valid_source and
                is_cachable(self.stream_payload.get("text")) and
                not fallback and
                not self._conducting_experiment("tts_reverse_exp")
            ):
                self.rt_log('tts.post_to_cache_started',
                            data_size=len(self.data) if self.data is not None else None,
                            body_size=self.body_size,
                            lookups=self.lookups)

                key = cache_key(self.stream_payload)
                value = tts_response_to_cache(self.data, (self.lookups / self.body_size)
                                              if self.body_size else 0, self.tts_timings, duration)

                IOLoop.current().spawn_callback(self._store_tts_cache, key, value)

                self.tts_timings = []

            self.data = b""

            if self.tts_stream:
                self.tts_stream.close()
                self.tts_stream = None

            if self.generate_queue:
                self.DLOG("Continue with queue", len(self.generate_queue))
                self.say()
            else:
                if self.body_size:
                    AccessLogger(
                        "tts_lookups",
                        self.system,
                        event_id=self.event.message_id,
                        rt_log=self.rt_log
                    ).end(200, 0, self.lookups / self.body_size)
                    AccessLogger(
                        "tts_rtf",
                        self.system,
                        event_id=self.event.message_id,
                        rt_log=self.rt_log
                    ).end(200, 0, self.rtf / self.body_size)
                self.full_logger.end(code=200, size=self.body_size)
                self.rt_log('tts.complete', body_size=self.body_size, rtf=self.rtf)
                self.body_size = 0
                if self.tts_streamer:
                    if not self.tts_streamer.finalize(self.on_close_tts_streamer):
                        return  # must delay closing while streamer work (has data)
                    self.tts_streamer = None
                if self.on_close is None:
                    self.close()
                else:
                    self.on_close()
                    self.on_close = None

    def send_tts_speak(self, format=None, from_cache=False, payload_patch=None):
        self.tts_streamer = create_streamer(self.system, self.realtime_streamer_cfg, self.event.ref_stream_id)
        payload = dict(
            format=self.initial_stream_payload["mime"],
            enable_bargin=self.enable_bargin,
            lazy_tts_streaming=self.enable_lazy_tts_streaming,
        )
        if format:
            payload['format'] = format
        if from_cache:
            payload['from_cache'] = True
        if payload_patch:
            payload.update(payload_patch)
        directive = Directive(
            'TTS',
            'Speak',
            payload,
            self.event.message_id,
            stream_id=self.tts_streamer.stream_id,
        )

        self.system.write_directive(directive, self)

    def on_tts_error(self, error):
        self.rt_log.log_child_activation_finished(self.tts_stream_rtlog_token, False)
        self.rt_log.error('tts.error_received', error=error)
        self.dispatch_error(error)

    def on_audio_data(self, audio_data):
        self.rt_log.log_child_activation_finished(self.audio_player_rtlog_token, True)
        self.rt_log('tts.audio_chunk_received', chunk_size=len(audio_data) if audio_data is not None else None)

        # we can use tts_stream as flag still even when using audio_stream.
        # looks almost like copy of on_data, but without cache (TO DO: need refactoring to implement nice caching)
        if self.is_closed():
            self.DLOG("tts stream ignore audio", len(audio_data))
            self.rt_log('tts.stream_ignore_audio')
            return

        if self.first_logger:
            self.first_logger.end(code=200, size=0)
            self.first_logger = None

        if self.tts_streamer is None:
            self.send_tts_speak()

        if audio_data:
            self.body_size += len(audio_data)
            self.tts_streamer.add_data(audio_data)
            self._update_first_chunk_stats()
        else:
            if self.audio_player:
                self.audio_player.close()
                self.audio_player = None

            if self.generate_queue:
                self.DLOG("Continue with queue", len(self.generate_queue))
                self.say()
            else:
                self.full_logger.end(code=200, size=self.body_size)
                self.rt_log('tts.complete', body_size=self.body_size)
                if self.tts_streamer:
                    if not self.tts_streamer.finalize(self.on_close_tts_streamer):
                        return  # must delay closing while streamer work (has data)
                    self.tts_streamer = None
                if self.on_close is None:
                    self.close()
                else:
                    self.on_close()
                    self.on_close = None

    def on_close_tts_streamer(self):
        if self.tts_streamer:
            self.log_info['full_response_time'] = time.monotonic() - self.log_info['start_time']
        self.tts_streamer = None
        if self.on_close is None:
            self.close()
        else:
            self.on_close()
            self.on_close = None

    def on_audio_error(self, e):
        self.rt_log.log_child_activation_finished(self.audio_player_rtlog_token, False)
        self.rt_log.error('tts.audio_error_received', error=e)
        self.dispatch_error(e)

    def process_event(self, event):
        super(Generate, self).process_event(event)
        self.start_time = time.monotonic()
        self.log_info['start_time'] = self.start_time
        self.log_info['event_birth_time'] = event.birth_ts

        self.rt_log('process_event_started', event_type='tts.generate')

        # make shallow copy and change `format` and `quality` feilds
        d = mime2format(self.payload_with_session_data)

        cache_id = d.pop("cache_id", None)
        self._cache = self.system.cache_manager.get_cache(cache_id) if cache_id is not None else Cache("nonexisting")

        tts_cache_id = d.pop("full_tts_cache_id", None)
        self._tts_cache = self.system.cache_manager.get_cache(tts_cache_id) if tts_cache_id is not None \
            else Cache("nonexisting")

        text = d.pop("text", None)
        if self._conducting_experiment('disable_interruption_spotter'):
            # Actually this flag doesn't disable interruption spotter but asks Uiproxy to check
            # voice response if it contains spotter words. If it does then TTS.Speakdirective will
            # contain "disableInterruptionSpotter = True".
            # Exception - don't disable interruption spotter if:
            #  - VINS forces interruption spotter (i.e. interruption spotter MUST work)
            #  - for Yandex.Novosti (for a while)
            if self._conducting_experiment('force_interruption_spotter_flag'):
                self.disable_spotter = False
            elif not self.event.payload.get("force_interruption_spotter"):
                self.disable_spotter = contains_spotter_text(text)

        d['need_timings'] = True  # by default we want to save timings in cache
        if self._conducting_experiment('enable_tts_timings'):
            self.need_timings = True
            d['need_timings'] = True

        if self._conducting_experiment('disable_tts_timings'):
            self.need_timings = False
            d['need_timings'] = False

        if self._conducting_experiment('enable_bargin'):
            self.enable_bargin = True

        if self._conducting_experiment('enable_lazy_tts_streaming'):
            self.enable_lazy_tts_streaming = True

        prepend_text = experiment_value("prepend_tts_text", self.event.payload)
        if prepend_text:
            text = prepend_text + text

        self.realtime_streamer_cfg = d.pop("realtime_streamer", None)
        if self.enable_lazy_tts_streaming:
            if self.realtime_streamer_cfg is None:
                self.realtime_streamer_cfg = {}
            self.realtime_streamer_cfg['sync_chunker'] = True

        self.initial_stream_payload = d.copy()

        self.initial_stream_payload["lang"] = get_language(d.get("lang", "ru"))

        self.initial_stream_payload["mime"] = format2mime(d)
        if self.realtime_streamer_cfg:
            self.realtime_streamer_cfg["mime"] = self.initial_stream_payload["mime"]
            if self._conducting_experiment("enable_tts_background"):
                self.realtime_streamer_cfg["background"] = "weather-hail.pcm"  # just some cool background...

        self.stream_payload = deepcopy(self.initial_stream_payload)

        if text is not None:
            self.say(text)

    def write_meta(self, tts_response):
        words = []
        phones = []
        for x in tts_response.phonemes:
            phones.append((x.positionInBytesStream / 2.0 / 48000, x.ttsPhoneme, x.durationMs * 0.001))

        for x in tts_response.words:
            words.append(
                {
                    'word': x.text,
                    'from': x.bytesLengthInSignal / 2.0 / 48000,
                    'postag': x.postag,
                    'homographTag': x.homographTag,
                }
            )
        if tts_response.words:
            words.append({'word': 'END OF UTT', 'from': -1, 'postag': '', 'homographTag': ''})

        word_i = 0
        trans = [[] for x in words]
        for p in phones:
            while words[word_i]['from'] < 0 and word_i < len(words) - 1:
                word_i += 1
            if (
                words[word_i]['from'] >= 0
                and (word_i == len(words) - 1 or words[word_i + 1]['from'] > p[0] + p[2] * 0.9)
            ):
                pass
            else:
                word_i += 1
            trans[word_i].append(p[1])
        self.system.write_directive(
            Directive("TTS", "Meta", {
                'words': words,
                'transcriptions': trans,
            }, self.event.message_id)
        )

    def write_timings(self, timings, from_cache=False):
        data = MessageToDict2(timings)
        if data:
            self.tts_timings.append(timings)
            if not self.need_timings:
                return

            data['from_cache'] = from_cache

            for timing in data['timings']:
                timing['time'] += self.current_duration * 1000

            self.system.write_directive(
                Directive("TTS", "Timings", data, self.event.message_id)
            )

    @gen.coroutine
    def say(self, what=""):
        if self.is_closed():
            return

        if not self.generate_queue:
            self._create_generate_queue(what)

        next_request = self.generate_queue.pop(0)
        self.rt_log('tts.next_request')

        if next_request:
            self.stream_payload.update(next_request)
        else:
            self.stream_payload = deepcopy(self.initial_stream_payload)

        if next_request and "background" in next_request:
            if self.generate_queue:
                self.DLOG("Ignore background sound due to multiple audio streams")
            else:
                self.realtime_streamer_cfg["background"] = next_request["background"]

        if next_request and next_request.get("audio", None):
            # while we can't mix audio and text, we just move text to next item to speak
            if next_request.get("text"):
                self.generate_queue.insert(0, {"text": next_request.get("text")})

            self.audio_player_rtlog_token = self.rt_log.log_child_activation_started('audio_player')
            self.audio_player = AudioPlayer(
                self.on_audio_data,
                self.on_audio_error,
                self.stream_payload)
            return

        cache_lookup_res =\
            yield memcache_tts_lookup(self._cache, next_request, self.stream_payload, self.rt_log,
                                      self.DLOG, self.log_info)
        self.stream_payload = cache_lookup_res.payload

        lookup_result_to_global_counters(cache_lookup_res)
        cache_success = cache_lookup_res.result is not None
        if self.on_tts_cache_response:
            self.on_tts_cache_response(cache_success)
            self.on_tts_cache_response = None
        if cache_success:
            self.cache_get_success(format2mime(self.stream_payload), cache_lookup_res.result)
        else:
            self.tts_stream_rtlog_token = self.rt_log.log_child_activation_started('tts')
            if self._conducting_experiment(UNIPROXY_FULL_TTS_PRECACHE_EXP):
                self.DLOG("Starting cached TTS stream")
                fetch_cached_tts_stream(self.system.srcrwr['TTS'], self._tts_cache, self.stream_payload, self.rt_log,
                                        self.DLOG, self.event.message_id, self.on_data, self.on_tts_error)
            else:
                self.DLOG("Starting non-cached TTS stream")
                self.start_tts_stream()

    def start_tts_stream(self):
        self.tts_stream = TtsStream(
            self.on_data,
            self.on_tts_error,
            self.stream_payload,
            host=self.system.srcrwr['TTS'],
            rt_log=self.rt_log,
            system=self.system,
            message_id=self.event.message_id,
        )

    def cache_get_success(self, mime, data):
        if self.is_closed():
            return

        if self.tts_streamer is None:
            self.send_tts_speak(
                format=mime,
                from_cache=True,
                payload_patch={'disableInterruptionSpotter': self.disable_spotter},
            )

        audio_data = data
        lookup_rate = 0.0
        timings = None
        duration = None

        if not data.startswith(b'OggS'):
            try:
                tts_response = tts_response_from_cache(data)
                audio_data = tts_response.audio_data
                lookup_rate = tts_response.lookup_rate
                timings = tts_response.timings
                duration = tts_response.duration
            except Exception:
                audio_data = data

        if timings:
            for timing in timings:
                self.write_timings(timing, from_cache=True)
            if duration:
                self.current_duration += duration
        self._update_first_chunk_stats(from_cache=True)
        self.tts_streamer.add_data(audio_data)
        AccessLogger("tts_lookups", self.system, rt_log=self.rt_log).end(200, 0, lookup_rate)

        if self.tts_stream:
            self.tts_stream.close()
            self.tts_stream = None

        if self.generate_queue:
            self.DLOG("Continue with queue", len(self.generate_queue))
            self.say()
        else:
            self.full_logger.end(code=200, size=self.body_size)
            self.rt_log('tts.complete', body_size=self.body_size)
            if self.tts_streamer:
                if not self.tts_streamer.finalize(self.on_close_tts_streamer):
                    return  # must delay closing while streamer work (has data)
                self.tts_streamer = None
            if self.on_close is None:
                self.close()
            else:
                self.on_close()

    def _conducting_experiment(self, experiment):
        return conducting_experiment(experiment, self.event.payload)

    def _update_first_chunk_stats(self, from_cache=False):
        if self.start_time is None:
            return

        suffix = "_quasar" if self.system.is_quasar else ""
        first_chunk_time = time.monotonic()
        GlobalTimings.store(TTS_FIRST_CHUNK_HGRAM + suffix,
                            first_chunk_time - self.start_time)
        self.log_info['first_chunk_time'] = first_chunk_time - self.start_time
        self.start_time = None

        if self.on_first_chunk:
            self.on_first_chunk(from_cache=from_cache)
        self.on_first_chunk = None

    def _create_generate_queue(self, text):
        self.event.payload["text"] = text
        self.system.logger.log_event(self.event)
        self.first_logger.start(event_id=self.event.message_id)
        self.full_logger.start(event_id=self.event.message_id)
        self.generate_queue = split_text_by_speaker_tags(text, self.payload_with_session_data.get('settings_from_manager', dict()))

        if self._conducting_experiment("oksana.gpu_to_good_oksana"):
            for req in self.generate_queue:
                if req.get("voice") == "oksana.gpu":
                    req["voice"] = "good_oksana"
                    getattr(GlobalCounter, "EXP_OKSANA.GPU_TO_GOOD_OKSANA_SUMM").increment()

    async def _store_tts_cache(self, key, value):
        cache_client = CacheStorageClient()
        return await cache_client.store(key, value)
