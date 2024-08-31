from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.global_counter import GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.perf_tester import events
from alice.uniproxy.library.processors import create_event_processor
import alice.uniproxy.library.processors.vins
from alice.uniproxy.library.processors.vins import ProxyBackend
from alice.uniproxy.library.testing.mocks import (
    FakeAsyncHttpStream,
    FakePersonalDataHelper,
    FakeRtLog,
    FakeUniWebSocket,
    FakeYabioStream,
    reset_mocks,
)
from alice.uniproxy.library.testing.wrappers import WrappedVinsRequest, reset_wrappers
import alice.uniproxy.library.vins.vinsadapter
from alice.uniproxy.library.vins.vinsadapter import VinsAdapter
import alice.uniproxy.library.vins.vinsrequest
# from copy import deepcopy
import datetime
import time
from tornado.concurrent import Future
import tornado.gen
from tornado.ioloop import IOLoop
# import tornado.ioloop
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
        self.vins_cancel_count = 0
        self.processor = processor
        self.on_vins_response_finished_future = Future()

    def on_vins_partial(self, what_to_say=None):
        self.vins_partial_count += 1

    def on_vins_response(self, *args, **kwargs):
        self.vins_response_count += 1
        IOLoop.current().spawn_callback(self.on_vins_response_coro, *args, **kwargs)

    def on_vins_cancel(self, *args, **kwargs):
        self.vins_cancel_count += 1

    async def on_vins_response_coro(self, *args, **kwargs):
        await self.processor.on_vins_response(*args, **kwargs)
        self.on_vins_response_finished_future.set_result(True)


@testing.ioloop_run
def ioloop_test_vins_prepare_request_counters(music=False, bad_partials=False):
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
        has_spotter=True,
        has_biometry=True,
    )
    need_buffer = False
    processor.streaming_backends['yabio'] = ProxyBackend(processor.create_yabio, need_buffer)
    assert len(FakeYabioStream.STREAMS) == 1
    processor.start_scoring(need_buffer=False, spotter=False)
    assert len(FakeYabioStream.STREAMS) == 2
    asr_result = {
        "endOfUtt": False,
        "asr_partial_number": 0,
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

    if music:
        # receiving music result cause separate request to Vins/MM
        music_result = {}
        vins_adapter.set_music_result(music_result)
        vins_session = {}
        vins_adapter.set_vins_session(vins_session)
        assert len(WrappedVinsRequest.REQUESTS) == 1
        vins_request = WrappedVinsRequest.REQUESTS[0]
        yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), vins_request.start_request_finished)
        assert len(FakeAsyncHttpStream.REQUESTS) == 1
        yield FakeAsyncHttpStream.REQUESTS[0].return_result()

        metrics_dict = get_metrics_dict()
        assert count_events(events.EventUsefulVinsPrepareRequestAsr.NAME + '_hgram', metrics_dict) == 0
        assert count_events(events.EventUsefulVinsPrepareRequestClassify.NAME + '_hgram', metrics_dict) == 0
        assert count_events(events.EventUsefulVinsPrepareRequestMusic.NAME + '_hgram', metrics_dict) == 1
        assert count_events(events.EventUsefulVinsPrepareRequestSession.NAME + '_hgram', metrics_dict) == 1
        assert count_events(events.EventUsefulVinsPrepareRequestYabio.NAME + '_hgram', metrics_dict) == 0
    else:
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

        if bad_partials:
            # emulate asr_end result diff from previous partial
            asr_result.update({
                'endOfUtt': True,
                "asr_partial_number": 1,
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
            vins_adapter.set_asr_result(asr_result)
            vins_adapter.set_scoring_result(scoring_result, processed_chunks)
            vins_adapter.set_classify_result(classify_result, processed_chunks)
            vins_adapter.set_vins_session(vins_session)
            assert len(WrappedVinsRequest.REQUESTS) == 2
            vins_request = WrappedVinsRequest.REQUESTS[1]
            yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), vins_request.start_request_finished)
            assert len(FakeAsyncHttpStream.REQUESTS) == 2
            yield FakeAsyncHttpStream.REQUESTS[1].return_result()
            yield callbacks.on_vins_response_finished_future  # wait processor callback-coroutine
            d = vins_adapter.vins_timings.to_dict()
            assert d.get(events.EventStartVinsRequestEOU.NAME) is not None
            assert d.get(events.EventFinishVinsRequestEOU.NAME) is not None
        else:
            # emulate asr_end result same as previous partial
            # use already exist vins response (for partial) in processor, - not make second request to vins
            asr_result['endOfUtt'] = True
            vins_adapter.set_asr_result(asr_result)
            yield callbacks.on_vins_response_finished_future  # wait processor callback-coroutine

            metrics_dict = get_metrics_dict()
            assert count_events(events.EventUsefulVinsPrepareRequestAsr.NAME + '_hgram', metrics_dict) == 1
            assert count_events(events.EventUsefulVinsPrepareRequestClassify.NAME + '_hgram', metrics_dict) == 1
            assert count_events(events.EventUsefulVinsPrepareRequestMusic.NAME + '_hgram', metrics_dict) == 0
            assert count_events(events.EventUsefulVinsPrepareRequestSession.NAME + '_hgram', metrics_dict) == 1
            assert count_events(events.EventUsefulVinsPrepareRequestYabio.NAME + '_hgram', metrics_dict) == 1


def test_vins_prepare_request_counters(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    UniproxyTimings.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    monkeypatch.setattr(alice.uniproxy.library.processors.vins, 'YabioStream', FakeYabioStream)
    ioloop_test_vins_prepare_request_counters()
    UniproxyCounter.init()
    UniproxyTimings.init()
    ioloop_test_vins_prepare_request_counters(music=True)
    UniproxyCounter.init()
    UniproxyTimings.init()
    ioloop_test_vins_prepare_request_counters(bad_partials=True)
