from alice.uniproxy.library.processors import create_event_processor
from voicetech.library.proto_api.ttsbackend_pb2 import GenerateResponse
import common
import tornado
import alice.uniproxy.library.backends_tts.cache
from alice.uniproxy.library import testing


def test_create_tts_processors():
    proc_speech_started = create_event_processor(common.FakeSystem, common.FakeEvent("TTS", "SpeechStarted"))
    assert proc_speech_started.event_type == "tts.speechstarted"

    proc_speech_finished = create_event_processor(common.FakeSystem, common.FakeEvent("TTS", "SpeechFinished"))
    assert proc_speech_finished.event_type == "tts.speechfinished"

    proc_list_voices = create_event_processor(common.FakeSystem, common.FakeEvent("TTS", "ListVoices"))
    assert proc_list_voices.event_type == "tts.listvoices"

    proc_generate = create_event_processor(common.FakeSystem, common.FakeEvent("TTS", "Generate"))
    assert proc_generate.event_type == "tts.generate"

    import alice.uniproxy.library.processors.tts as tts

    assert isinstance(proc_speech_started, tts.SpeechStarted)
    assert isinstance(proc_speech_finished, tts.SpeechFinished)
    assert isinstance(proc_list_voices, tts.ListVoices)
    assert isinstance(proc_generate, tts.Generate)


def test_timings_directive():
    system = common.FakeSystem()
    event = common.FakeEvent("TTS", "Generate", payload={'request': {'experiments': {'enable_tts_timings': '1'}}})
    p = create_event_processor(system, event)
    p.event = event
    p.tts_stream = object()
    p.tts_streamer = object()

    p.need_timings = True
    msg = GenerateResponse()
    t = msg.timings.timings.add()
    t.time = 1.0
    t.phoneme = 'foo'
    p.on_data(msg)

    assert len(system._directives) == 1
    assert 'Timings' in system._directives
    assert 'timings' in system._directives['Timings'].payload
    assert len(system._directives['Timings'].payload['timings']) == 1
    assert 'phoneme' in system._directives['Timings'].payload['timings'][0]
    assert 'time' in system._directives['Timings'].payload['timings'][0]

    assert system._directives['Timings'].payload['timings'][0]['phoneme'] == 'foo'
    assert system._directives['Timings'].payload['timings'][0]['time'] >= 0.999999
    assert system._directives['Timings'].payload['timings'][0]['time'] <= 1.000001


class FakeLogger:
    def __init__(self):
        pass

    def log_event(self, *args, **kwargs):
        pass

    def log_directive(self, *args, **kwargs):
        pass


class FakeStream:
    def close(self):
        pass


class FakeStreamer:
    def finalize(self, *args, **kwargs):
        return True


def test_multi_timings_directives(monkeypatch):
    system = common.FakeSystem()
    system.logger = FakeLogger()

    def fake_foo(*args, **kwargs):
        return False

    def fake_tts_response_to_cache(data: bytes, lookup_rate: float, timings=None, duration=None):
        assert duration >= 0.09999
        assert duration <= 0.10001
        return False

    monkeypatch.setattr(alice.uniproxy.library.processors.tts, 'tts_response_to_cache', fake_tts_response_to_cache)
    system.on_close_event_processor = fake_foo

    event = common.FakeEvent("TTS", "Generate", payload={'request': {'experiments': {'enable_tts_timings': '1'}}})
    p = create_event_processor(system, event)
    p.event = event
    p.tts_stream = FakeStream()  # object()
    p.tts_streamer = FakeStreamer()  # object()
    p.stream_payload = {'text': 'text'}
    p.say = fake_foo
    p.generate_queue = ['some']
    p.DLOG = fake_foo

    p.need_timings = True
    msg = GenerateResponse()
    t = msg.timings.timings.add()
    t.time = 1.0
    t.phoneme = 'foo'
    msg.completed = True
    msg.duration = 0.1
    p.on_data(msg)

    assert len(system._directives) == 1
    assert 'Timings' in system._directives
    assert 'timings' in system._directives['Timings'].payload
    assert len(system._directives['Timings'].payload['timings']) == 1
    assert 'phoneme' in system._directives['Timings'].payload['timings'][0]
    assert 'time' in system._directives['Timings'].payload['timings'][0]

    assert system._directives['Timings'].payload['timings'][0]['phoneme'] == 'foo'
    assert system._directives['Timings'].payload['timings'][0]['time'] >= 0.999999
    assert system._directives['Timings'].payload['timings'][0]['time'] <= 1.000001

    p.tts_stream = FakeStream()

    msg = GenerateResponse()
    t = msg.timings.timings.add()
    t.time = 1.0
    t.phoneme = 'bar'
    p.on_data(msg)

    assert 'Timings' in system._directives
    assert 'timings' in system._directives['Timings'].payload
    assert len(system._directives['Timings'].payload['timings']) == 1
    assert 'phoneme' in system._directives['Timings'].payload['timings'][0]
    assert 'time' in system._directives['Timings'].payload['timings'][0]

    assert system._directives['Timings'].payload['timings'][0]['phoneme'] == 'bar'
    assert system._directives['Timings'].payload['timings'][0]['time'] >= 100.99999
    assert system._directives['Timings'].payload['timings'][0]['time'] <= 101.00001


@testing.ioloop_run
def _test_process_event_enable_tts_timings(monkeypatch):

    @tornado.gen.coroutine
    def fake_memcache_tts_lookup(cache, system, request, tts_like_payload, rt_log, log_fn, metrics=None):
        assert tts_like_payload.get('need_timings', False)
    monkeypatch.setattr(alice.uniproxy.library.processors.tts, 'memcache_tts_lookup', fake_memcache_tts_lookup)

    system = common.FakeSystem()
    system.logger = FakeLogger()

    event = common.FakeEvent("TTS", "Generate", payload={'request': {'experiments': {'enable_tts_timings': '1'}}, 'text': 'озвучь меня'})
    p = create_event_processor(system, event)
    p.event = event
    p.tts_stream = object()
    p.tts_streamer = object()

    p.process_event(event)
    msg = GenerateResponse()
    t = msg.timings.timings.add()
    t.time = 1.0
    t.phoneme = 'foo'
    p.on_data(msg)

    yield tornado.gen.sleep(1.1)
    assert len(system._directives) == 1


def test_process_event_enable_tts_timings(monkeypatch):
    _test_process_event_enable_tts_timings(monkeypatch)


@testing.ioloop_run
def _test_process_event_disable_tts_timings(monkeypatch):

    @tornado.gen.coroutine
    def fake_memcache_tts_lookup(cache, system, request, tts_like_payload, rt_log, log_fn, metrics=None):
        assert not tts_like_payload.get('need_timings', False)
    monkeypatch.setattr(alice.uniproxy.library.processors.tts, 'memcache_tts_lookup', fake_memcache_tts_lookup)

    system = common.FakeSystem()
    system.logger = FakeLogger()

    event = common.FakeEvent("TTS", "Generate", payload={'request': {'experiments': {'disable_tts_timings': '1'}}, 'text': 'озвучь меня'})
    p = create_event_processor(system, event)
    p.event = event
    p.tts_stream = object()
    p.tts_streamer = object()

    p.process_event(event)
    msg = GenerateResponse()
    t = msg.timings.timings.add()
    t.time = 1.0
    t.phoneme = 'foo'
    p.on_data(msg)

    yield tornado.gen.sleep(1.1)
    assert len(system._directives) == 0


def test_process_event_disable_tts_timings(monkeypatch):
    _test_process_event_disable_tts_timings(monkeypatch)


@testing.ioloop_run
def _test_process_event_without_tts_timings_flags(monkeypatch):

    @tornado.gen.coroutine
    def fake_memcache_tts_lookup(cache, system, request, tts_like_payload, rt_log, log_fn, metrics=None):
        assert tts_like_payload.get('need_timings', False)
    monkeypatch.setattr(alice.uniproxy.library.processors.tts, 'memcache_tts_lookup', fake_memcache_tts_lookup)

    system = common.FakeSystem()
    system.logger = FakeLogger()

    event = common.FakeEvent("TTS", "Generate", payload={'request': {'experiments': {}}, 'text': 'озвучь меня'})
    p = create_event_processor(system, event)
    p.event = event
    p.tts_stream = object()
    p.tts_streamer = object()

    p.process_event(event)
    msg = GenerateResponse()
    t = msg.timings.timings.add()
    t.time = 1.0
    t.phoneme = 'foo'
    p.on_data(msg)

    yield tornado.gen.sleep(1.1)
    assert len(system._directives) == 0


def test_process_event_without_tts_timings_flags(monkeypatch):
    _test_process_event_without_tts_timings_flags(monkeypatch)
