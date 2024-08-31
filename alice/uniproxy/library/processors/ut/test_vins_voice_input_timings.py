from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.global_counter import GlobalTimings, GlobalCounter
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.perf_tester import events
from alice.uniproxy.library.processors import create_event_processor
import alice.uniproxy.library.processors.vins
from alice.uniproxy.library.processors.vins import ProxyBackend
from alice.uniproxy.library.testing.mocks import (
    FakeAsyncHttpStream,
    FakeCacheStorageClient,
    FakePersonalDataHelper,
    FakeRtLog,
    FakeTtsStream,
    FakeUniWebSocket,
    FakeYabioStream,
    reset_mocks,
)
from alice.uniproxy.library.testing.wrappers import WrappedVinsRequest, reset_wrappers
import alice.uniproxy.library.vins.vinsadapter
from alice.uniproxy.library.vins.vinsadapter import VinsAdapter
import alice.uniproxy.library.vins.vinsrequest
import datetime
import time
from tornado.concurrent import Future
import tornado.gen
from tornado.ioloop import IOLoop
import uuid


def get_metrics_dict():
    metrics = GlobalTimings.get_metrics()
    metrics_dict = {}
    for it in metrics:
        metrics_dict[it[0]] = it[1]
    return metrics_dict


def count_events(metric_name, metrics_dict):
    return sum([it[1] for it in metrics_dict[metric_name]])


class VinsAdapterCallbacks:
    def __init__(self, processor):
        self.vins_partial_count = 0
        self.vins_response_count = 0
        self.processor = processor
        self.on_vins_response_finished_future = Future()

    def on_vins_partial(self, what_to_say=None):
        self.vins_partial_count += 1

    def on_vins_response(self, *args, **kwargs):
        self.vins_response_count += 1
        IOLoop.current().spawn_callback(self.on_vins_response_coro, *args, **kwargs)

    def on_vins_cancel(self, reason=None):
        pass

    async def on_vins_response_coro(self, *args, **kwargs):
        await self.processor.on_vins_response(*args, **kwargs)
        self.on_vins_response_finished_future.set_result(True)


@testing.ioloop_run
def ioloop_test_vins_voice_input_timings():
    """test one partial & eou result != partial"""
    reset_mocks()
    reset_wrappers()

    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4())
        },
        "payload": {
            "biometry_classify": "gender,children",
            "need_scoring": True,
            "request": {
                "experiments": [
                    "uniproxy_vins_timings"
                ],
                "session": {}
            },
        },
    })
    web_socket = FakeUniWebSocket()
    system = web_socket.system
    # system.fake_vin_requests = []
    message_id = None
    payload = {
        "settings_from_manager": {"use_mm_partials": True},
        "biometry_classify": True,
        "need_scoring": True,
    }
    rt_log = FakeRtLog()
    processor = create_event_processor(system, event)
    processor.payload_with_session_data.update({
        "request": {
            "experiments": {
                "uniproxy_vins_timings": 1
            }
        }
    })
    processor.event = event
    callbacks = VinsAdapterCallbacks(processor)
    personal_data_helper = FakePersonalDataHelper(system, payload, rt_log)
    event_start_time = time.time() - 1.
    epoch = time.time()
    vins_adapter = VinsAdapter(
        system,
        message_id,
        payload,
        callbacks.on_vins_partial,
        callbacks.on_vins_response,
        callbacks.on_vins_cancel,
        personal_data_helper,
        rt_log,
        processor,
        event_start_time,
        epoch,
    )
    need_buffer = False
    processor.streaming_backends['yabio'] = ProxyBackend(processor.create_yabio, need_buffer)
    assert len(FakeYabioStream.STREAMS) == 1
    processor.start_scoring(need_buffer=False, spotter=False)
    assert len(FakeYabioStream.STREAMS) == 2
    asr_result = {
        "asr_partial_number": 0,
        "endOfUtt": False,
        "e2e_recognition": [
            {
                "endOfPhrase": False,
                "normalized": "1",
                "words": [
                    {
                        "value": "раз",
                        "confidence": 1
                    }
                ],
                "confidence": 1,
                "parentModel": ""
            }
        ],
    }
    scoring_result = {
    }
    classify_result = {
    }
    # check foolproof
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0

    processor.vins_adapter = vins_adapter
    processor.on_asr_result(asr_result)

    processed_chunks = 1
    vins_adapter.set_asr_result(asr_result)
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    vins_session = {}
    vins_adapter.set_vins_session(vins_session)
    # emulate fast response from VINS
    assert len(WrappedVinsRequest.REQUESTS) == 1
    vins_request = WrappedVinsRequest.REQUESTS[0]
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), vins_request.start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    yield FakeAsyncHttpStream.REQUESTS[0].return_result()

    metrics_dict = get_metrics_dict()
    assert count_events(events.EventUsefulVinsPrepareRequestAsr.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestClassify.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestMusic.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestSession.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestYabio.NAME + '_hgram', metrics_dict) == 0

    # emulate asr_end result diff from previous partial
    asr_result.update({
        'endOfUtt': True,
        "e2e_recognition": [
            {
                "endOfPhrase": False,
                "normalized": "2",
                "words": [
                    {
                        "value": "два",
                        "confidence": 1
                    }
                ],
                "confidence": 1,
                "parentModel": ""
            }
        ],
    })
    processor.on_asr_result(asr_result)
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    vins_adapter.set_vins_session(vins_session)
    assert len(WrappedVinsRequest.REQUESTS) == 2
    vins_request = WrappedVinsRequest.REQUESTS[1]
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), vins_request.start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 2
    d = vins_adapter.vins_timings.to_dict()
    assert d.get(events.EventFirstAsrResult.NAME) is None
    yield FakeAsyncHttpStream.REQUESTS[1].return_result()
    yield callbacks.on_vins_response_finished_future  # wait processor callback-coroutine
    d = vins_adapter.vins_timings.to_dict()
    assert d.get(events.EventFirstAsrResult.NAME) is not None
    vins_timings_msg = None
    tts_timings_msg = None
    for msg in web_socket.output_messages:
        if 'directive' not in msg:
            continue
        if msg['directive']['header']['name'] == 'UniproxyVinsTimings':
            vins_timings_msg = msg
        if msg['directive']['header']['name'] == 'UniproxyTTSTimings':
            tts_timings_msg = msg

    assert vins_timings_msg is not None
    assert vins_timings_msg is not None
    vt = vins_timings_msg['directive']['payload']
    tt = tts_timings_msg['directive']['payload']
    assert vt.get(events.EventUsefulVinsRequest.NAME) is not None
    assert tt.get(events.EventUsefulResponseForUser.NAME) is not None


@testing.ioloop_run
def ioloop_test_vins_voice_input_timings2():
    """one partial(with quick response from vins) & eou same as partial"""
    reset_mocks()
    reset_wrappers()

    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4())
        },
        "payload": {
            "biometry_classify": "gender,children",
            "need_scoring": True,
            "request": {
                "experiments": [
                    "uniproxy_vins_timings"
                ],
                "session": {}
            },
        },
    })
    web_socket = FakeUniWebSocket()
    system = web_socket.system
    # system.fake_vin_requests = []
    message_id = None
    payload = {
        "settings_from_manager": {"use_mm_partials": True},
        "biometry_classify": True,
        "need_scoring": True,
    }
    rt_log = FakeRtLog()
    processor = create_event_processor(system, event)
    processor.payload_with_session_data.update({
        "request": {
            "experiments": {
                "uniproxy_vins_timings": 1
            }
        }
    })
    processor.event = event
    callbacks = VinsAdapterCallbacks(processor)
    personal_data_helper = FakePersonalDataHelper(system, payload, rt_log)
    event_start_time = time.time() - 1.
    epoch = time.time()
    vins_adapter = VinsAdapter(
        system,
        message_id,
        payload,
        callbacks.on_vins_partial,
        callbacks.on_vins_response,
        callbacks.on_vins_cancel,
        personal_data_helper,
        rt_log,
        processor,
        event_start_time,
        epoch,
    )
    need_buffer = False
    processor.streaming_backends['yabio'] = ProxyBackend(processor.create_yabio, need_buffer)
    assert len(FakeYabioStream.STREAMS) == 1
    processor.start_scoring(need_buffer=False, spotter=False)
    assert len(FakeYabioStream.STREAMS) == 2
    asr_result = {
        "asr_partial_number": 0,
        "endOfUtt": False,
        "e2e_recognition": [
            {
                "endOfPhrase": False,
                "normalized": "1",
                "words": [
                    {
                        "value": "раз",
                        "confidence": 1
                    }
                ],
                "confidence": 1,
                "parentModel": ""
            }
        ],
    }
    scoring_result = {
    }
    classify_result = {
    }
    # check foolproof
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0

    processed_chunks = 1
    vins_adapter.set_asr_result(asr_result)
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    vins_session = {}
    vins_adapter.set_vins_session(vins_session)
    # emulate fast response from VINS
    assert len(WrappedVinsRequest.REQUESTS) == 1
    vins_request = WrappedVinsRequest.REQUESTS[0]
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), vins_request.start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    # for test execute_vins_directives_lag_hgram use directive with future
    yield FakeAsyncHttpStream.REQUESTS[0].return_result({
        "voice_response": {
            "directives": [
                {
                    "type": "uniproxy_action",
                    "name": "force_interruption_spotter",
                }
            ],
            "output_speech": {
                "text": "кто здесь?",
            },
        }
    })
    metrics_dict = get_metrics_dict()
    assert count_events(events.EventUsefulVinsPrepareRequestAsr.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestClassify.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestMusic.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestPersonalData.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestSession.NAME + '_hgram', metrics_dict) == 0
    assert count_events(events.EventUsefulVinsPrepareRequestYabio.NAME + '_hgram', metrics_dict) == 0
    assert count_events('execute_vins_directives_lag_hgram', metrics_dict) == 0

    # emulate asr_end result same as previous partial
    asr_result.update({
        'endOfUtt': True,
    })
    processor.vins_adapter = vins_adapter
    assert len(WrappedVinsRequest.REQUESTS) == 1
    processor.on_asr_result(asr_result)
    assert len(WrappedVinsRequest.REQUESTS) == 1
    yield callbacks.on_vins_response_finished_future  # wait processor callback-coroutine
    metrics_dict = get_metrics_dict()
    summ_metrics_dict = dict(GlobalCounter.get_metrics())
    # print(GlobalCounter.get_metrics())
    # got vins response before EOU so here (wait after eou) MUST BE 0
    assert count_events(events.EventVinsWaitAfterEOUDurationSec.NAME + '_hgram', metrics_dict) == 0
    # print('T'*80, processor.events_timings())
    assert count_events('user_useful_partial_to_asr_end_hgram', metrics_dict) == 1
    assert count_events('robot_useful_partial_to_asr_end_hgram', metrics_dict) == 0
    assert summ_metrics_dict.get('user_useful_vins_response_before_eou_summ') == 1
    assert summ_metrics_dict.get('quasar_user_useful_vins_response_before_eou_summ') == 0
    assert summ_metrics_dict.get('robot_useful_vins_response_before_eou_summ') == 0
    assert summ_metrics_dict.get('quasar_robot_useful_vins_response_before_eou_summ') == 0
    # print('U'*80, metrics_dict)
    assert count_events('execute_vins_directives_lag_hgram', metrics_dict) == 1
    assert count_events(events.EventUsefulVinsPrepareRequestAsr.NAME + '_hgram', metrics_dict) == 1
    assert count_events(events.EventUsefulVinsPrepareRequestPersonalData.NAME + '_hgram', metrics_dict) == 1
    assert len(FakeTtsStream.STREAMS) == 1
    assert len(FakeCacheStorageClient.CLIENTS) == 1
    assert count_events('user_tts_cache_response_lag_hgram', metrics_dict) == 1

    processor._spotter_validation_result = None
    processor._spotter_future = Future()
    processor._spotter_future.set_result({})
    yield processor.get_spotter_validation_result()
    metrics_dict = get_metrics_dict()
    # print('U'*80, metrics_dict)
    assert count_events('user_spotter_validation_lag_hgram', metrics_dict) == 1


@testing.ioloop_run
def ioloop_test_vins_voice_input_timings3():
    """ case with vins response (to useful partial) after EOU"""
    reset_mocks()
    reset_wrappers()

    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4())
        },
        "payload": {
            "biometry_classify": "gender,children",
            "need_scoring": True,
            "request": {
                "experiments": [
                    "uniproxy_vins_timings"
                ],
                "session": {}
            },
        },
    })
    web_socket = FakeUniWebSocket()
    system = web_socket.system
    # system.fake_vin_requests = []
    message_id = None
    payload = {
        "settings_from_manager": {"use_mm_partials": True},
        "biometry_classify": True,
        "need_scoring": True,
    }
    rt_log = FakeRtLog()
    processor = create_event_processor(system, event)
    processor.payload_with_session_data.update({
        "request": {
            "experiments": {
                "uniproxy_vins_timings": 1
            }
        }
    })
    processor.event = event
    callbacks = VinsAdapterCallbacks(processor)
    personal_data_helper = FakePersonalDataHelper(system, payload, rt_log)
    event_start_time = time.time() - 1.
    epoch = time.time()
    vins_adapter = VinsAdapter(
        system,
        message_id,
        payload,
        callbacks.on_vins_partial,
        callbacks.on_vins_response,
        callbacks.on_vins_cancel,
        personal_data_helper,
        rt_log,
        processor,
        event_start_time,
        epoch,
    )
    need_buffer = False
    processor.streaming_backends['yabio'] = ProxyBackend(processor.create_yabio, need_buffer)
    assert len(FakeYabioStream.STREAMS) == 1
    processor.start_scoring(need_buffer=False, spotter=False)
    assert len(FakeYabioStream.STREAMS) == 2
    asr_result = {
        "asr_partial_number": 0,
        "endOfUtt": False,
        "e2e_recognition": [
            {
                "endOfPhrase": False,
                "normalized": "1",
                "words": [
                    {
                        "value": "раз",
                        "confidence": 1
                    }
                ],
                "confidence": 1,
                "parentModel": ""
            }
        ],
    }
    scoring_result = {
    }
    classify_result = {
    }
    # check foolproof
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0

    processed_chunks = 1
    vins_adapter
    vins_adapter.set_asr_result(asr_result)
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    vins_session = {}
    vins_adapter.set_vins_session(vins_session)
    # emulate fast response from VINS
    assert len(WrappedVinsRequest.REQUESTS) == 1
    vins_request = WrappedVinsRequest.REQUESTS[0]
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), vins_request.start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    metrics_dict = get_metrics_dict()
    assert count_events(events.EventVinsWaitAfterEOUDurationSec.NAME + '_hgram', metrics_dict) == 0

    # emulate asr_end result same as previous partial
    asr_result.update({
        'endOfUtt': True,
    })
    processor.vins_adapter = vins_adapter
    processor.on_asr_result(asr_result)
    assert len(WrappedVinsRequest.REQUESTS) == 1
    assert len(WrappedVinsRequest.REQUESTS) == 1
    # for test execute_vins_directives_lag_hgram use directive with future
    yield FakeAsyncHttpStream.REQUESTS[0].return_result({
        "voice_response": {
            "directives": [
                {
                    "type": "uniproxy_action",
                    "name": "force_interruption_spotter",
                }
            ],
            "output_speech": {
                "text": "кто здесь?",
            },
        }
    })
    yield callbacks.on_vins_response_finished_future  # wait processor callback-coroutine
    metrics_dict = get_metrics_dict()
    assert count_events(events.EventVinsWaitAfterEOUDurationSec.NAME + '_hgram', metrics_dict) == 1


def test_vins_voice_input_timings(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    UniproxyTimings.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'YabioStream', FakeYabioStream)
    ioloop_test_vins_voice_input_timings()
    UniproxyCounter.init()
    UniproxyTimings.init()


def test_vins_voice_input_timings2(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    UniproxyTimings.init()
    GlobalState.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    monkeypatch.setattr(alice.uniproxy.library.backends_tts.cache, 'CacheStorageClient', FakeCacheStorageClient)
    monkeypatch.setattr(alice.uniproxy.library.processors.tts, 'TtsStream', FakeTtsStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'YabioStream', FakeYabioStream)
    ioloop_test_vins_voice_input_timings2()
    UniproxyCounter.init()
    UniproxyTimings.init()


def test_vins_voice_input_timings3(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    UniproxyTimings.init()
    GlobalState.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.tts, 'TtsStream', FakeTtsStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'YabioStream', FakeYabioStream)
    ioloop_test_vins_voice_input_timings3()
    UniproxyCounter.init()
    UniproxyTimings.init()
