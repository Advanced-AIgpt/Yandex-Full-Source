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
    WrappedVinsRequest,
    reset_wrappers,
)
import alice.uniproxy.library.vins.vinsadapter
from alice.uniproxy.library.vins.vinsadapter import VinsAdapter
import alice.uniproxy.library.vins.vinsrequest
from copy import deepcopy
import datetime
import time
import tornado.gen
import tornado.ioloop
import uuid


class VinsAdapterCallbacks:
    def __init__(self):
        self.vins_partial_count = 0
        self.vins_response_count = 0
        self.vins_cancel_count = 0

    def on_vins_partial(self, what_to_say=None):
        self.vins_partial_count += 1

    def on_vins_response(self, raw_response=None, error=None, what_to_say=None,
                         vins_directives=None, force_eou=False, uniproxy_vins_timings=None):
        assert raw_response.get('voice_response', {}).get('directives') is None
        self.vins_response_count += 1

    def on_vins_cancel(self, reason=None):
        self.vins_cancel_count += 1


@testing.ioloop_run
def ioloop_test_vins_adapter():
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

    vins_session = {}
    vins_adapter.set_vins_session(vins_session)

    # ############### process first asr partial ###################
    processed_chunks = 1
    # create VinRequest, but without bio info, it MUST not make request to Vins
    vins_adapter.set_asr_result(asr_result)
    assert len(WrappedVinsRequest.REQUESTS) == 1
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[0].start_request_finished)
    # got all bio, make request to VINS
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    # emulate fast response from VINS
    yield FakeAsyncHttpStream.REQUESTS[0].return_result()
    assert WrappedVinsRequest.REQUESTS[0].partial_numbers['asr'] == 0
    assert WrappedVinsRequest.REQUESTS[0].asr_core_debug is None
    assert callbacks.vins_partial_count == 1

    # ############### process second asr partial ###################
    processed_chunks = 2
    asr_result2 = deepcopy(asr_result)
    asr_result2["e2e_recognition"][0]["normalized"] = "1 2"
    asr_result2["e2e_recognition"][0]["words"][0]["value"] = "раз два"
    asr_result2["asr_partial_number"] += 1
    core_debug = {"key": "value"}
    asr_result2["coreDebug"] = core_debug
    # create VinRequest, but without bio info, it MUST not make request to Vins
    vins_adapter.set_asr_result(asr_result2)
    assert len(WrappedVinsRequest.REQUESTS) == 2
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[1].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 2
    assert WrappedVinsRequest.REQUESTS[1].partial_numbers['asr'] == 1
    assert vins_adapter.last_asr_core_debug == core_debug
    assert WrappedVinsRequest.REQUESTS[1].asr_core_debug == core_debug
    # here we delay reponse from VINS

    # ############### process third asr partial (same as second) ###################
    # check we not create VinsRequest for same partial
    asr_result2["asr_partial_number"] += 1
    vins_adapter.set_asr_result(asr_result2)
    assert len(WrappedVinsRequest.REQUESTS) == 2  # << not changed
    assert WrappedVinsRequest.REQUESTS[1].asr_core_debug == core_debug

    # ############### process eou asr result (same as second) ###################
    # check here we not go to VINS for asr_eou if already has partial request for same result
    asr_result2eou = deepcopy(asr_result2)
    asr_result2eou["asr_partial_number"] += 1
    asr_result2eou["endOfUtt"] = True
    vins_adapter.set_asr_result(asr_result2eou)
    assert len(WrappedVinsRequest.REQUESTS) == 2  # << not changed
    assert WrappedVinsRequest.REQUESTS[1].asr_core_debug == core_debug
    assert callbacks.vins_partial_count == 1
    assert callbacks.vins_response_count == 0

    # check for haldling partial result with same text as eou as final vins reponse
    res = {
        'response': {
            #
        },
        'voice_response': {
            'directives': [
                {}
            ]
        }
    }
    yield FakeAsyncHttpStream.REQUESTS[1].return_result(result=res)
    assert callbacks.vins_partial_count == 1
    assert callbacks.vins_response_count == 1  # got final vins response << SUCCESS

    assert len(vins_adapter.vins_timings._vins_preparing_requests) == 2

    # TODO?: check using apply_request?


def test_vins_adapter(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    ioloop_test_vins_adapter()


@testing.ioloop_run
def ioloop_test_vins_adapter_classification(actions):
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
    personal_data_helper = FakePersonalDataHelper(system, payload, rt_log, need_future=True)
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
    # check foolproof
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0

    vins_session = {}
    vins_adapter.set_vins_session(vins_session)

    def check(answer):
        assert len(vins_adapter.classify_result['simple']) == len(answer)
        for i in range(len(answer)):
            tag1 = vins_adapter.classify_result['simple'][i]['tag']
            classname1 = vins_adapter.classify_result['simple'][i]['classname']
            tag2 = answer[i]['tag']
            classname2 = answer[i]['classname']
            assert tag1 == tag2
            assert classname1 == classname2

    for item in actions:
        action = item['action']
        if action == 'classify':
            result = item['result']
            chunks = item['chunks']
            vins_adapter.set_classify_result(result, chunks)
            answer = item['answer']
            check(answer)
        elif action == 'datasync':
            personal_data_helper.future.set_result(item['result'])
            yield tornado.gen.sleep(0.3)
        elif action == 'check':
            check(item['answer'])
        else:
            assert False


def get_datasync_with_children_bio(children_bio_enabled):
    str_res = 'enabled' if children_bio_enabled else 'disabled'
    result = {
        'action': 'datasync',
        'result': {
            '/v1/personality/profile/alisa/kv/alice_children_biometry': str_res
        }
    }
    return result


def test_vins_adapter_classification(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    actions = []
    classify1 = {
        'action': 'classify',
        'result': {
            'simple': [
                {
                    'tag': 'children',
                    'classname': 'a'
                }
            ]
        },
        'chunks': 1,
        'answer': [
            {
                'tag': 'children',
                'classname': 'a'
            }
        ]
    }
    actions.append(classify1)
    ioloop_test_vins_adapter_classification(actions)


def test_vins_adapter_classification2(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    actions = []
    classify = {
        'action': 'classify',
        'result': {
            'simple': [
                {
                    'tag': 'children',
                    'classname': 'a'
                },
                {
                    'tag': 'adult',
                    'classname': 'b'
                },
                {
                    'tag': 'children',
                    'classname': 'c'
                },
            ]
        },
        'chunks': 1,
        'answer': [
            {
                'tag': 'children',
                'classname': 'a'
            },
            {
                'tag': 'adult',
                'classname': 'b'
            },
            {
                'tag': 'children',
                'classname': 'c'
            },
        ]
    }
    actions.append(classify)
    actions.append(get_datasync_with_children_bio(False))
    check = {
        'action': 'check',
        'answer': [
            {
                'tag': 'adult',
                'classname': 'b'
            }
        ]
    }
    actions.append(check)
    ioloop_test_vins_adapter_classification(actions)


def test_vins_adapter_classification3(monkeypatch):
    Logger.init("uniproxy", is_debug=True)
    UniproxyCounter.init()
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsadapter, 'VinsRequest', WrappedVinsRequest)
    monkeypatch.setattr(alice.uniproxy.library.vins.vinsrequest, 'AsyncHttpStream', FakeAsyncHttpStream)
    actions = []
    classify = {
        'action': 'classify',
        'result': {
            'simple': [
                {
                    'tag': 'children',
                    'classname': 'a'
                },
                {
                    'tag': 'adult',
                    'classname': 'b'
                },
                {
                    'tag': 'children',
                    'classname': 'c'
                },
            ]
        },
        'chunks': 1,
        'answer': [
            {
                'tag': 'adult',
                'classname': 'b'
            }
        ]
    }
    actions.append(get_datasync_with_children_bio(False))
    actions.append(classify)
    ioloop_test_vins_adapter_classification(actions)


@testing.ioloop_run
def ioloop_test_vins_adapter_with_cache_keys():
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
        "cacheKey": "one"
    }
    scoring_result = {
    }
    classify_result = {
    }
    # check foolproof
    assert len(WrappedVinsRequest.REQUESTS) == 0
    assert len(FakeAsyncHttpStream.REQUESTS) == 0

    vins_session = {}
    vins_adapter.set_vins_session(vins_session)

    # ############### process first asr partial ###################
    processed_chunks = 1
    # create VinRequest, but without bio info, it MUST not make request to Vins
    vins_adapter.set_asr_result(asr_result)
    assert len(WrappedVinsRequest.REQUESTS) == 1
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    assert len(FakeAsyncHttpStream.REQUESTS) == 0
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[0].start_request_finished)
    # got all bio, make request to VINS
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    # emulate fast response from VINS
    yield FakeAsyncHttpStream.REQUESTS[0].return_result()
    assert WrappedVinsRequest.REQUESTS[0].asr_partial_number == 0
    assert callbacks.vins_partial_count == 1

    # ############### process second asr partial ###################
    processed_chunks = 2
    asr_result2 = deepcopy(asr_result)
    asr_result2["cacheKey"] = "one two"
    asr_result2["e2e_recognition"][0]["normalized"] = "1 2"
    asr_result2["e2e_recognition"][0]["words"][0]["value"] = "раз два"
    asr_result2["asr_partial_number"] += 1
    # create VinRequest, but without bio info, it MUST not make request to Vins
    vins_adapter.set_asr_result(asr_result2)
    assert len(WrappedVinsRequest.REQUESTS) == 2
    assert len(FakeAsyncHttpStream.REQUESTS) == 1
    vins_adapter.set_scoring_result(scoring_result, processed_chunks)
    vins_adapter.set_classify_result(classify_result, processed_chunks)
    # need wait VinsRequest coroutine for avoid race with it
    yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), WrappedVinsRequest.REQUESTS[1].start_request_finished)
    assert len(FakeAsyncHttpStream.REQUESTS) == 2
    assert WrappedVinsRequest.REQUESTS[1].asr_partial_number == 1
    # here we delay reponse from VINS

    # ############### process third asr partial (same cache_key, different partial) ###################
    # check we not create VinsRequest for same partial
    asr_result2["asr_partial_number"] += 1
    asr_result2["e2e_recognition"][0]["normalized"] = "1 2 3"
    asr_result2["e2e_recognition"][0]["words"][0]["value"] = "раз два три"
    vins_adapter.set_asr_result(asr_result2)
    assert len(WrappedVinsRequest.REQUESTS) == 2  # << not changed

    # ############### process fourth asr partial (different cache_key, same partial) ###################
    # check we not create VinsRequest for same partial
    asr_result2["asr_partial_number"] += 1
    asr_result2["cacheKey"] = "one two three"
    vins_adapter.set_asr_result(asr_result2)
    assert len(WrappedVinsRequest.REQUESTS) == 3  # << changed
