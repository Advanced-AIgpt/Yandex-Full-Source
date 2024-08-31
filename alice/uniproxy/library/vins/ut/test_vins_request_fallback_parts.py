from alice.uniproxy.library import testing
from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.processors import create_event_processor
from alice.uniproxy.library.testing.mocks import (
    FakeAsyncHttpStream,
    FakePersonalDataHelper,
    FakeRtLog,
    FakeUniWebSocket,
    FakeYabioStream,
    reset_mocks,
)
from alice.uniproxy.library.testing.wrappers import (
    WrappedVinsApplyRequest,
    WrappedVinsRequest,
    reset_wrappers,
)
import alice.uniproxy.library.vins.vinsadapter
from alice.uniproxy.library.vins.vinsadapter import VinsAdapter
import alice.uniproxy.library.vins.vinsrequest
from copy import deepcopy
import datetime
import json
import time
import tornado.gen
import tornado.ioloop
import uuid


class VinsAdapterCallbacks:
    def __init__(self):
        self.vins_partial_count = 0
        self.vins_response_count = 0

    def on_vins_partial(self, what_to_say=None):
        self.vins_partial_count += 1

    def on_vins_response(self, raw_response=None, error=None, what_to_say=None,
                         vins_directives=None, force_eou=False, uniproxy_vins_timings=None):
        self.vins_response_count += 1

    def on_vins_cancel(self, reason=None):
        pass


@testing.ioloop_run
def ioloop_test_vins_request_fallback_parts():
    reset_mocks()
    reset_wrappers()

    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4())
        },
        "payload": {}
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
    callbacks = VinsAdapterCallbacks()
    on_vins_partial = callbacks.on_vins_partial
    on_vins_response = callbacks.on_vins_response
    on_vins_cancel = callbacks.on_vins_cancel
    rt_log = FakeRtLog()
    processor = create_event_processor(system, event)

    def return_fake_backed():
        return FakeYabioStream('yabio-score')

    processor.score_backend = return_fake_backed
    processor.classify_backend = return_fake_backed
    personal_data_helper = FakePersonalDataHelper(system, payload, rt_log)
    event_start_time = time.time() - 1.
    epoch = time.time()
    vins_adapter = VinsAdapter(
        system,
        message_id,
        payload,
        on_vins_partial,
        on_vins_response,
        on_vins_cancel,
        personal_data_helper,
        rt_log,
        processor,
        event_start_time,
        epoch,
        has_spotter=True,
        has_biometry=True,
    )
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
    # check foolproof
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0

    # ############### process first asr partial ###################
    processed_chunks = 1
    # create VinRequest, but without set bio info, it MUST make request to Vins with delay = WAIT_BIOMETRY_TIMELIMIT1)
    time_partial1 = time.monotonic()
    vins_adapter.set_asr_result(asr_result)
    vins_session = {}
    vins_adapter.set_vins_session(vins_session)
    assert len(WrappedVinsRequest.REQUESTS) == 1
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[0].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    # check using timelimit for yabio futures without result
    assert (time.monotonic() - time_partial1) > (WrappedVinsRequest.WAIT_BIOMETRY_TIMELIMIT1 * 0.9)
    # emulate fast response from VINS
    yield FakeAsyncHttpStream.REQUESTS[0].return_result()
    assert callbacks.vins_partial_count == 1

    # ############### process second asr partial ###################
    asr_result2 = deepcopy(asr_result)
    asr_result2["e2e_recognition"][0]["normalized"] = "1 2"
    asr_result2["e2e_recognition"][0]["words"][0]["value"] = "раз два"
    asr_result2["asr_partial_number"] = 1
    scoring_result = {
        "partial_scoring": 2
    }
    classify_result = {
        "partial_classify": 2
    }
    processed_chunks = 2
    # create VinRequest, but without bio info, it MUST not make request to Vins
    vins_adapter.set_asr_result(asr_result2)
    vins_adapter.set_vins_session(vins_session)
    # hacks for ensure set_<bio>_result executed successfuly
    vins_adapter.need_score_processed_chunks = processed_chunks
    vins_adapter.need_classify_processed_chunks = processed_chunks
    assert len(WrappedVinsRequest.REQUESTS) == 2
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[1].start_request_finished)
    # got all bio, make request to VINS
    assert len(FakeAsyncHttpStream.REQUESTS) == 2
    # check using scoring & classify results
    event = WrappedVinsRequest.REQUESTS[1]._request['request']['event']
    assert event['biometry_scoring']['partial_scoring'] == 2
    assert event['biometry_classification']['partial_classify'] == 2
    # emulate fast response from VINS (for checking apply need defer_apply in response)
    yield FakeAsyncHttpStream.REQUESTS[1].return_result({
        "response": {
            "directives": [
                {
                    "type": "uniproxy_action",
                    "name": "defer_apply",
                    "payload": {
                        "session": {}
                    }
                },
            ]
        }
    })
    assert callbacks.vins_partial_count == 2

    # ############### process third asr partial ###################
    # check here reusing biometry results from prevous responses (wit delay = WAIT_BIOMETRY_TIMELIMIT2)
    processed_chunks = 3
    asr_result3 = deepcopy(asr_result)
    asr_result3["e2e_recognition"][0]["normalized"] = "1 2 3"
    asr_result3["e2e_recognition"][0]["words"][0]["value"] = "раз два три"
    asr_result3["asr_partial_number"] = 2
    # create VinRequest, but without bio info, it MUST not make request to Vins
    vins_adapter.set_asr_result(asr_result3)
    vins_adapter.set_vins_session(vins_session)
    assert len(WrappedVinsRequest.REQUESTS) == 3
    assert len(FakeAsyncHttpStream.REQUESTS) == 2
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[2].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 3
    # check using timelimit for yabio futures with using fallback parts
    assert (time.monotonic() - time_partial1) > (WrappedVinsRequest.WAIT_BIOMETRY_TIMELIMIT2 * 0.9)
    print(json.dumps(WrappedVinsRequest.REQUESTS[2]._request, indent=4))
    event = WrappedVinsRequest.REQUESTS[2]._request['request']['event']
    assert event['biometry_scoring']['partial_scoring'] == 2
    assert event['biometry_classification']['partial_classify'] == 2
    # emulate fast response from VINS
    yield FakeAsyncHttpStream.REQUESTS[2].return_result()
    assert callbacks.vins_partial_count == 3

    # ############### process fourh asr partial ###################
    # check here reusing biometry results from prevous responses (wit delay = WAIT_BIOMETRY_TIMELIMIT2)
    processed_chunks = 4
    asr_result4 = deepcopy(asr_result)
    asr_result4["e2e_recognition"][0]["normalized"] = "1 2 3 4"
    asr_result4["e2e_recognition"][0]["words"][0]["value"] = "раз два три четыре"
    asr_result4["asr_partial_number"] = 3
    # create VinRequest, but without bio info, it MUST not make request to Vins
    vins_adapter.set_asr_result(asr_result4)
    vins_adapter.set_vins_session(vins_session)
    vins_adapter.need_score_processed_chunks = processed_chunks
    vins_adapter.need_classify_processed_chunks = processed_chunks
    scoring_result4 = {
        "partial_scoring": 4
    }
    classify_result4 = {
        "partial_classify": 4
    }
    vins_adapter.set_scoring_result(scoring_result4, processed_chunks)
    vins_adapter.set_classify_result(classify_result4, processed_chunks)

    assert len(WrappedVinsRequest.REQUESTS) == 4
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[3].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 4
    event = WrappedVinsRequest.REQUESTS[3]._request['request']['event']
    assert event['biometry_scoring']['partial_scoring'] == 4
    assert event['biometry_classification']['partial_classify'] == 4
    # emulate fast response from VINS
    yield FakeAsyncHttpStream.REQUESTS[3].return_result()
    assert callbacks.vins_partial_count == 4

    # ############### process eou asr result (same as third) ###################
    # check here we use already exist biometry in apply request
    asr_result2eou = deepcopy(asr_result2)
    asr_result2eou["asr_partial_number"] = 4
    asr_result2eou["endOfUtt"] = True
    vins_adapter.set_asr_result(asr_result2eou)
    vins_adapter.set_vins_session(vins_session)
    assert len(WrappedVinsApplyRequest.REQUESTS) == 1
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsApplyRequest.REQUESTS[0].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 5
    event = WrappedVinsApplyRequest.REQUESTS[0]._request['request']['event']
    assert event['biometry_scoring']['partial_scoring'] == 2
    assert event['biometry_classification']['partial_classify'] == 2


@testing.ioloop_run
def ioloop_test_vins_request_notifier_timeout():
    reset_mocks()
    reset_wrappers()

    event = Event({
        "header": {
            "namespace": "Vins",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4())
        },
        "payload": {}
    })
    web_socket = FakeUniWebSocket()
    system = web_socket.system
    system.session_data['supported_features'] = ['notifications']
    # system.fake_vin_requests = []
    message_id = None
    payload = {
        "settings_from_manager": {"use_mm_partials": True},
        "biometry_classify": False,
        "need_scoring": False,
    }
    callbacks = VinsAdapterCallbacks()
    on_vins_partial = callbacks.on_vins_partial
    on_vins_response = callbacks.on_vins_response
    on_vins_cancel = callbacks.on_vins_cancel
    rt_log = FakeRtLog()
    processor = create_event_processor(system, event)

    def return_fake_backed():
        return FakeYabioStream('yabio-score')

    processor.score_backend = return_fake_backed
    processor.classify_backend = return_fake_backed
    personal_data_helper = FakePersonalDataHelper(system, payload, rt_log)
    event_start_time = time.time() - 1.
    epoch = time.time()
    vins_adapter = VinsAdapter(
        system,
        message_id,
        payload,
        on_vins_partial,
        on_vins_response,
        on_vins_cancel,
        personal_data_helper,
        rt_log,
        processor,
        event_start_time,
        epoch,
        has_spotter=True,
        has_biometry=False,
    )
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
    # check foolproof
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0

    # ############### process first asr partial ###################
    # processed_chunks = 1
    # create VinRequest
    time_partial1 = time.monotonic()
    vins_adapter.set_asr_result(asr_result)
    vins_session = {}
    vins_adapter.set_vins_session(vins_session)
    assert len(WrappedVinsRequest.REQUESTS) == 1
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[0].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    # check using timelimit for notifier futures without result
    assert (time.monotonic() - time_partial1) > (WrappedVinsRequest.WAIT_NOTIFIER_TIMELIMIT * 0.9)


def test_vins_request_fallback_parts(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsApplyRequest', WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    ioloop_test_vins_request_fallback_parts()


def test_vins_request_notifier_timeout(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsApplyRequest', WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    ioloop_test_vins_request_notifier_timeout()
