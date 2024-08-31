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
import time
import tornado.concurrent
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
def ioloop_test_vins_adapter_with_delayed_future():
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
    )

    vins_adapter.personal_data_getter_future = tornado.concurrent.Future()

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
    vins_adapter.set_asr_result(asr_result)
    vins_session = {}
    vins_adapter.set_vins_session(vins_session)
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    assert vins_adapter.delayed_vins_params['asr_partial_number'] == 0
    assert vins_adapter.delayed_vins_params['request_text'] == 'раз'
    assert callbacks.vins_partial_count == 0

    # ############### process second asr partial ###################
    asr_result2 = deepcopy(asr_result)
    asr_result2["e2e_recognition"][0]["normalized"] = "1 2 tree"
    asr_result2["e2e_recognition"][0]["words"][0]["value"] = "раз два tree"
    asr_result2["asr_partial_number"] = 1
    vins_adapter.set_asr_result(asr_result2)
    vins_adapter.set_vins_session(vins_session)
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    assert vins_adapter.delayed_vins_params['asr_partial_number'] == 1
    assert vins_adapter.delayed_vins_params['request_text'] == 'раз два tree'
    assert callbacks.vins_partial_count == 0

    # ############### process third asr partial ###################
    # check here that new partial with shorter result will restore previous partial
    asr_result3 = deepcopy(asr_result)
    asr_result3["e2e_recognition"][0]["normalized"] = "1 2"
    asr_result3["e2e_recognition"][0]["words"][0]["value"] = "раз два"
    asr_result3["asr_partial_number"] = 2
    vins_adapter.set_asr_result(asr_result3)
    vins_adapter.set_vins_session(vins_session)
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    assert vins_adapter.delayed_vins_params['asr_partial_number'] == 2
    assert vins_adapter.delayed_vins_params['request_text'] == 'раз два'
    assert callbacks.vins_partial_count == 0

    # ############### process fourh asr partial ###################
    # check here that empty asr partial will not restore previous partial
    asr_result4 = deepcopy(asr_result)
    asr_result4["e2e_recognition"] = []
    asr_result4["asr_partial_number"] = 3
    vins_adapter.set_asr_result(asr_result4)

    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert callbacks.vins_partial_count == 0

    # ############### release future  ###################
    # check here using last not empty asr partial
    vins_adapter.personal_data_getter_future.set_result(True)
    vins_adapter.personal_data_getter_future = None

    assert len(WrappedVinsRequest.REQUESTS) == 1
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[0].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    event = WrappedVinsRequest.REQUESTS[0]._request['request']['event']
    assert event['asr_result'][0]['words'][0]['value'] == 'раз два'
    assert event['hypothesis_number'] == 2


def test_vins_adapter_with_delayed_future(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsApplyRequest', WrappedVinsApplyRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    ioloop_test_vins_adapter_with_delayed_future()
