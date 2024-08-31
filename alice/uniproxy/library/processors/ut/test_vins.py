import tornado
import time
import common
from alice.uniproxy.library.events import Event
from alice.uniproxy.library.extlog import AccessLogger
from alice.uniproxy.library.global_counter import GlobalTimings
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.processors import create_event_processor
from alice.uniproxy.library.processors.vins import get_response_delay
import alice.uniproxy.library.perf_tester.events as events


class FakeVinsAdapter:
    def __init__(self, parameters={}):
        self.params = parameters

    def set_classify_result(cls, *args, **kwargs):
        pass


class FakeTimings:
    def __init__(self, d):
        self._dict = d

    def get_event(self, event):
        return self._dict.get(event.NAME)


class FakeFlagsJsonResponse:
    def __init__(self, flags):
        self.flags = flags
        self._exp_boxes = "1,0,2"

    def get_all_flags(self):
        return self.flags

    def get_all_test_id(self):
        return [1]


def test_on_yabio_result():
    system = common.FakeSystem()
    event = common.FakeEvent("Vins", "VoiceInput")
    proc = create_event_processor(system, event)

    # faking
    proc.event = event
    proc.vins_adapter = FakeVinsAdapter
    proc.yabio_logger = AccessLogger("yabio_full", system)
    proc.yabio_logger.start()

    # check
    proc.on_yabio_result(None, None)

    assert system._directives['Classification'].payload is not None


def test_get_response_delay():
    assert(get_response_delay(None, dict()) is None)

    assert(get_response_delay(FakeTimings({events.EventEndOfUtterance.NAME: 2}),
                              FakeTimings({events.EventFirstTTSChunkSec.NAME: 3})) == 1)

    assert(get_response_delay(FakeTimings({}), FakeTimings({events.EventFirstTTSChunkSec.NAME: 3})) == 3)
    assert(get_response_delay(FakeTimings({}), FakeTimings({events.EventFirstTTSChunkSec.NAME: 3})) == 3)
    assert(get_response_delay(FakeTimings({events.EventEndOfUtterance.NAME: 2, events.EventVinsResponse.NAME: 1}),
                              FakeTimings({})) == 0)


def test_exposing_uaas_vins_url_to_vins_adapter_zero_testing():
    system = common.FakeSystem()

    def fake_foo(*args, **kwargs):
        return None
    # faking system
    system.log_experiment = fake_foo
    system.WARN = fake_foo
    system.staff_login = None

    event = common.FakeEvent("Vins", "VoiceInput")
    proc = create_event_processor(system, event)

    # faking proc
    proc.event = event
    proc.vins_adapter = FakeVinsAdapter()
    proc._try_process_external_experiments = fake_foo

    flags = {'UaasVinsUrl_aHR0cDovL21lZ2FtaW5kLXJjLmFsaWNlLnlhbmRleC5uZXQ=': '1'}
    proc._on_flags_json_response_wrap(FakeFlagsJsonResponse(flags))

    # check
    assert proc.vins_adapter.params['uaasVinsUrl'] == 'http://megamind-rc.alice.yandex.net'


def test_exposing_session_ref_message_ids_to_mm():
    system = common.FakeSystem()

    def fake_foo(*args, **kwargs):
        return None
    # faking system
    system.uaas_flags = {}
    system.get_oauth_token = fake_foo

    event = common.FakeEvent("Vins", "TextInput")
    event.payload = {'header': {'some_key': 'some_value'}}
    proc = create_event_processor(system, event)

    proc.finalize_uaas = fake_foo

    proc.process_event(event)

    # check
    assert proc.payload_with_session_data['header']['ref_message_id'] == '12345'
    assert proc.payload_with_session_data['header']['session_id'] == 1
    assert proc.payload_with_session_data['header']['some_key'] == 'some_value'


def test_exposing_session_ref_message_ids_to_mm_2():
    system = common.FakeSystem()

    def fake_foo(*args, **kwargs):
        return None
    # faking system
    system.uaas_flags = {}
    system.get_oauth_token = fake_foo

    event = common.FakeEvent("Vins", "TextInput")
    event.payload = {}
    proc = create_event_processor(system, event)

    proc.process_event(event)

    # check
    assert proc.payload_with_session_data['header']['ref_message_id'] == '12345'
    assert proc.payload_with_session_data['header']['session_id'] == 1


# this class create FakeUaasResponce with some flag and help to check flag exposion
class ExpHelper:
    def __init__(self):
        self.test_id = 229759
        self.flags = ['alice_complex_news_to_wizard']
        self.raw_flags = ['W3siSEFORExFUiI6IlZPSUNFIiwiQ09OVEVYVCI6eyJWT0lDRSI6eyJmb\
                           GFncyI6WyJhbGljZV9jb21wbGV4X25ld3NfdG9fd2l6YXJkIl19fX1d']

    def get_flag(self):
        return self.flags[0]


class FakeFlagsJsonClient:
    def __init__(self, iurl, on_result, *args, **kwargs):
        self.on_result_cb = on_result
        self.foo()

    @tornado.gen.coroutine
    def foo(self):
        tornado.gen.sleep(0.1)
        self.on_result_cb(FakeFlagsJsonResponse({'alice_complex_news_to_wizard' : '1'}))


def template_zero_testing(proc_type):
    system = common.FakeSystem()

    def fake_foo(*args, **kwargs):
        return None
    # faking system
    system.log_experiment = fake_foo
    system.WARN = fake_foo
    system.staff_login = None
    system.uaas_flags = {}
    system.get_oauth_token = fake_foo
    system.x_yandex_appinfo = None
    system.device_id = None

    event = common.FakeEvent('Vins', proc_type)
    event.payload = {'request': {'megamind_cookies': '{"uaas_tests":[229759]}'}}

    proc = create_event_processor(system, event)

    # faking proc
    proc.event = event
    proc.vins_adapter = FakeVinsAdapter()

    class FakeLog:
        def debug(self, *args, **kwargs):
            pass

        def error(self, *args, **kwargs):
            pass

        def warning(self, *args, **kwargs):
            pass

        def exception(self, *args, **kwargs):
            pass

    proc._log = FakeLog()
    proc.finalize_uaas = fake_foo
    proc.flags_json_client_class = FakeFlagsJsonClient

    proc.process_event(event)
    time.sleep(0.2)
    exp_helper = ExpHelper()

    # check
    assert proc._extra_experiments.get(exp_helper.get_flag()) is not None


def test_voice_input_zero_testing():
    template_zero_testing('VoiceInput')


def test_music_input_zero_testing():
    template_zero_testing('MusicInput')


def test_text_input_zero_testing():
    template_zero_testing('TextInput')


def count_hgram_metrics():
    res = {}
    metrics = GlobalTimings.get_metrics()
    for metric in metrics:
        name = metric[0][:-6]  # trim '_hgram' suffix
        res[name] = sum(bucket[1] for bucket in metric[1])
    return res


def test_vins_evage_lag():
    GlobalTimings.reset("asr_end_to_vins_response_lag")
    GlobalTimings.reset("vins_request_eou_to_vins_response_lag")
    GlobalTimings.reset("vins_session_save_duration")
    GlobalTimings.reset("vins_response_to_tts_first_chunk_lag")
    GlobalTimings.reset("tts_start_to_tts_first_chunk_lag")
    GlobalTimings.reset("tts_start_to_tts_first_chunk_nocache_lag")

    event = Event({
        "header": {"name": "VoiceInput", "namespace": "Vins", "messageId": "14"},
        "payload": {}
    })
    proc = create_event_processor(common.FakeSystem(), event)
    proc.event = event

    useful_asr_result_ts = time.monotonic()
    proc.store_event_age("asr_end_evage")
    metric_counts = count_hgram_metrics()
    assert metric_counts["asr_end_to_vins_response_lag"] == 0
    assert metric_counts["vins_request_eou_to_vins_response_lag"] == 0
    assert metric_counts["vins_session_save_duration"] == 0
    assert metric_counts["vins_response_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_nocache_lag"] == 0

    proc.store_event_age(events.EventVinsPersonalDataStart.NAME)
    proc.store_event_age(events.EventVinsPersonalDataEnd.NAME)
    proc.store_event_age("vins_request_eou_evage")
    metric_counts = count_hgram_metrics()
    assert metric_counts["asr_end_to_vins_response_lag"] == 0
    assert metric_counts["vins_request_eou_to_vins_response_lag"] == 0
    assert metric_counts["vins_session_save_duration"] == 0
    assert metric_counts["vins_response_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_nocache_lag"] == 0

    metric_counts = count_hgram_metrics()
    assert metric_counts["vins_response_sent_to_tts_first_chunk_lag"] == 0
    assert metric_counts["asr_end_to_useful_response_for_user_lag"] == 0
    assert metric_counts["useful_asr_result_to_useful_response_for_user_lag"] == 0
    metric_counts = count_hgram_metrics()

    proc.store_event_age("useful_asr_result_evage", useful_asr_result_ts)
    metric_counts = count_hgram_metrics()
    assert metric_counts["useful_asr_result_evage"] == 1
    assert metric_counts["useful_asr_result_to_vins_personal_data_end_lag"] == 1

    proc.store_event_age("vins_response_evage")
    proc.store_event_age(events.EventUsefulResponseForUser.NAME)
    metric_counts = count_hgram_metrics()
    assert metric_counts["useful_asr_result_to_useful_response_for_user_lag"] == 1
    assert metric_counts["asr_end_to_vins_response_lag"] == 1
    assert metric_counts["asr_end_to_useful_response_for_user_lag"] == 1
    assert metric_counts["vins_request_eou_to_vins_response_lag"] == 1
    assert metric_counts["vins_session_save_duration"] == 0
    assert metric_counts["vins_response_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_nocache_lag"] == 0

    proc.store_event_age("vins_session_save_start_evage")
    proc.store_event_age("vins_session_save_end_evage")
    metric_counts = count_hgram_metrics()
    assert metric_counts["asr_end_to_vins_response_lag"] == 1
    assert metric_counts["vins_request_eou_to_vins_response_lag"] == 1
    assert metric_counts["vins_session_save_duration"] == 1
    assert metric_counts["vins_response_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_nocache_lag"] == 0

    proc.store_event_age(events.EventVinsResponseSent.NAME)
    proc.store_event_age(events.EventTtsStart.NAME)
    metric_counts = count_hgram_metrics()
    assert metric_counts["asr_end_to_vins_response_lag"] == 1
    assert metric_counts["vins_request_eou_to_vins_response_lag"] == 1
    assert metric_counts["vins_session_save_duration"] == 1
    assert metric_counts["vins_response_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_lag"] == 0
    assert metric_counts["tts_start_to_tts_first_chunk_nocache_lag"] == 0

    proc.store_event_age("tts_first_chunk_evage")
    metric_counts = count_hgram_metrics()
    assert metric_counts["asr_end_to_vins_response_lag"] == 1
    assert metric_counts["vins_request_eou_to_vins_response_lag"] == 1
    assert metric_counts["vins_session_save_duration"] == 1
    assert metric_counts["vins_response_to_tts_first_chunk_lag"] == 1
    assert metric_counts["tts_start_to_tts_first_chunk_lag"] == 1
    assert metric_counts["tts_start_to_tts_first_chunk_nocache_lag"] == 0
    assert metric_counts["vins_response_sent_to_tts_first_chunk_lag"] == 1

    proc.store_event_age("tts_first_chunk_nocache_evage")
    metric_counts = count_hgram_metrics()
    assert metric_counts["asr_end_to_vins_response_lag"] == 1
    assert metric_counts["vins_request_eou_to_vins_response_lag"] == 1
    assert metric_counts["vins_session_save_duration"] == 1
    assert metric_counts["vins_response_to_tts_first_chunk_lag"] == 1
    assert metric_counts["tts_start_to_tts_first_chunk_lag"] == 1
    assert metric_counts["tts_start_to_tts_first_chunk_nocache_lag"] == 1

    proc.store_event_age(events.EventStartExecuteVinsDirectives.NAME)
    proc.store_event_age(events.EventFinishExecuteVinsDirectives.NAME)
    metric_counts = count_hgram_metrics()
    assert metric_counts["execute_vins_directives_lag"] == 1


def test_disable_biometry_classify():
    Logger.init("uniproxy_tests", is_debug=True)

    system = common.FakeSystem()
    system.uaas_flags = {}
    system.get_oauth_token = lambda: None
    system.srcrwr = common.FakeSrcrwr(rewrites={"ASR": None})

    event = common.FakeEvent("Vins", "VoiceInput")
    event.payload = {
        "request": {
            "experiments": {
                "disable_biometry_classify": True
            }
        }
    }

    proc = create_event_processor(system, event)
    proc.process_event(event)

    assert proc.classify_backend() is None


def test_disable_biometry_scoring():
    Logger.init("uniproxy_tests", is_debug=True)

    system = common.FakeSystem()
    system.uaas_flags = {}
    system.get_oauth_token = lambda: None
    system.srcrwr = common.FakeSrcrwr(rewrites={"ASR": None})

    event = common.FakeEvent("Vins", "VoiceInput")
    event.payload = {
        "request": {
            "experiments": {
                "disable_biometry_scoring": True
            }
        }
    }

    proc = create_event_processor(system, event)
    proc.process_event(event)

    assert proc.score_backend() is None
