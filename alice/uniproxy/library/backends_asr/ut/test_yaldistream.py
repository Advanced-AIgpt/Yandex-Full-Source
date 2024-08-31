import time
import tornado.gen

from common import AsrClient
from alice.uniproxy.library.backends_asr import GaldiStream, YaldiStream, DoubleYaldiStream, get_yaldi_stream_type
from alice.uniproxy.library.backends_asr.yaldistream import YaldiStat, get_asr_srcrwr_name
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.logging import Logger

from voicetech.library.proto_api.yaldi_pb2 import AddDataResponse

Logger.init("test_yaldistream", is_debug=True)


def test_get_asr_srcrwr_name():
    assert get_asr_srcrwr_name('ru-RU', 'dialog-general') == 'ASR_RU_DIALOG_GENERAL'


def test_create_yaldi_stream():
    client = AsrClient(YaldiStream)
    assert client.asr_stream


def test_create_double_yaldi_stream():
    client = AsrClient(DoubleYaldiStream, params={"topic": "A+B"})
    assert client.asr_stream


def test_get_yaldi_stream_type():
    assert get_yaldi_stream_type(topic="galdiTopic") == GaldiStream
    assert get_yaldi_stream_type(topic="googleTopic") == GaldiStream
    assert get_yaldi_stream_type(topic="googleTopic+B") == DoubleYaldiStream
    assert get_yaldi_stream_type(topic="A+googleTopic") == DoubleYaldiStream
    assert get_yaldi_stream_type(topic="someTopic") == YaldiStream


def test_yaldi_stat_counter_name():
    yaldi_stat = YaldiStat('dialogeneral')
    assert yaldi_stat.enable
    assert yaldi_stat.eq_counter_name == 'ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM'
    assert yaldi_stat.eq_timing_name == 'asr_eou_eq_time_dialoggeneral'
    assert yaldi_stat.ne_timing_name == 'asr_eou_ne_time_dialoggeneral'

    yaldi_stat = YaldiStat('dialog-general-gpu')
    assert yaldi_stat.enable
    assert yaldi_stat.eq_counter_name == 'ASR_PARTIAL_EQ_DIALOGGENERAL_GPU_SUMM'
    assert yaldi_stat.eq_timing_name == 'asr_eou_eq_time_dialoggeneral_gpu'
    assert yaldi_stat.ne_timing_name == 'asr_eou_ne_time_dialoggeneral_gpu'

    yaldi_stat = YaldiStat('desktopgeneral')
    assert yaldi_stat.enable
    assert yaldi_stat.eq_counter_name == 'ASR_PARTIAL_EQ_DESKTOPGENERAL_SUMM'
    assert yaldi_stat.eq_timing_name == 'asr_eou_eq_time_desktopgeneral'
    assert yaldi_stat.ne_timing_name == 'asr_eou_ne_time_desktopgeneral'

    yaldi_stat = YaldiStat('desktopgeneral-gpu')
    assert not yaldi_stat.enable

    yaldi_stat = YaldiStat('quasar-general')
    assert yaldi_stat.enable
    assert yaldi_stat.eq_counter_name == 'ASR_PARTIAL_EQ_QUASARGENERAL_SUMM'
    assert yaldi_stat.eq_timing_name == 'asr_eou_eq_time_quasargeneral'
    assert yaldi_stat.ne_timing_name == 'asr_eou_ne_time_quasargeneral'

    yaldi_stat = YaldiStat('quasar-general-gpu')
    assert yaldi_stat.enable
    assert yaldi_stat.eq_counter_name == 'ASR_PARTIAL_EQ_QUASARGENERAL_GPU_SUMM'
    assert yaldi_stat.eq_timing_name == 'asr_eou_eq_time_quasargeneral_gpu'
    assert yaldi_stat.ne_timing_name == 'asr_eou_ne_time_quasargeneral_gpu'

    yaldi_stat = YaldiStat('dialogmaps')
    assert yaldi_stat.enable
    assert yaldi_stat.eq_counter_name == 'ASR_PARTIAL_EQ_DIALOGMAPS_SUMM'
    assert yaldi_stat.eq_timing_name == 'asr_eou_eq_time_dialogmaps'
    assert yaldi_stat.ne_timing_name == 'asr_eou_ne_time_dialogmaps'

    yaldi_stat = YaldiStat('dialogmapsgpu')
    assert yaldi_stat.enable
    assert yaldi_stat.eq_counter_name == 'ASR_PARTIAL_EQ_DIALOGMAPS_GPU_SUMM'
    assert yaldi_stat.eq_timing_name == 'asr_eou_eq_time_dialogmaps_gpu'
    assert yaldi_stat.ne_timing_name == 'asr_eou_ne_time_dialogmaps_gpu'

    yaldi_stat = YaldiStat('')
    assert not yaldi_stat.enable

    yaldi_stat = YaldiStat('smth')
    assert not yaldi_stat.enable


def test_yaldi_stat_words_normalization():
    yaldi_stat = YaldiStat('dialogeneral')
    assert yaldi_stat.enable
    words = yaldi_stat._get_normalized_words({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}]})
    assert words == 'one|two|three'
    words = yaldi_stat._get_normalized_words({'recognition': []})
    assert words == ''
    words = yaldi_stat._get_normalized_words({})
    assert words == ''
    words = yaldi_stat._get_normalized_words({'recognition': [{'words': []}]})
    assert words == ''


def test_yaldi_stat_without_partials():
    UniproxyCounter.init()
    yaldi_stat = YaldiStat('dialogeneral')
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True})
    assert GlobalCounter.ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM.value() == 0


def get_timing(name):
    timings = GlobalTimings.get_metrics()
    name += '_HGRAM'
    for timing in timings:
        if timing[0].upper() == name:
            return timing
    return None


def are_timings_equal(init_timing, final_timing):
    for i in range(len(init_timing)):
        if init_timing[i] != final_timing[i]:
            return False
    return True


def test_yaldi_stat_with_partials():
    UniproxyCounter.init()
    UniproxyTimings.init()
    init_timing = get_timing('ASR_EOU_NE_TIME_DIALOGGENERAL')
    yaldi_stat = YaldiStat('dialogeneral')
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': False})
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True})
    final_timing = get_timing('ASR_EOU_NE_TIME_DIALOGGENERAL')
    assert not are_timings_equal(init_timing, final_timing)
    assert GlobalCounter.ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM.value() == 0


def test_yaldi_stat_with_partials_2():
    UniproxyCounter.init()
    UniproxyTimings.init()
    init_timing = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')
    yaldi_stat = YaldiStat('dialogeneral')
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False})
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False})
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False})
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': False})
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': False})
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': False})
    yaldi_stat.process_new_data({'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': True})
    final_timing = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')
    assert not are_timings_equal(init_timing, final_timing)
    assert GlobalCounter.ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM.value() == 1


class MyProto:
    def __init__(self, results_to_return, yaldi):
        self._counter = 0
        self._res = results_to_return
        self.yaldi_stream = yaldi

    def is_closed(self):
        return False

    def is_closed_2(self):
        return False, ''

    def read_protobuf(self, data_to_ignore, foo):
        foo(proto_from_json(AddDataResponse, self._res[self._counter % len(self._res)]))
        self._counter += 1

    def send_protobuf(self, data_to_ignore, foo):
        foo(True)

    def mark_as_failed(self):
        pass

    def soft_close(self):
        pass

    @tornado.gen.coroutine
    def send_protobuf_ex(self, data_to_ignore):
        return True

    @tornado.gen.coroutine
    def read_protobuf_ex(self, data_to_ignore):
        data = proto_from_json(AddDataResponse, self._res[self._counter % len(self._res)])
        self._counter += 1
        return data


class YaldiStreamHolder:
    def __init__(self, answers):
        self.yaldi_stream = YaldiStream(self.foo, self.foo, {'topic': 'dialogeneral'}, None, None)
        proto = MyProto(answers, self.yaldi_stream)
        self.yaldi_stream.yaldi_proto = proto
        self.yaldi_stream.init_sent = True
        self.yaldi_stream.is_closed = self.is_closed

    def is_closed(self):
        return False

    def foo(self, res):
        pass

    def add_chunk(self):
        data = bytes('data', encoding='utf-8')
        time.sleep(0.05)
        self.yaldi_stream.add_chunk(data)


def test_yaldi_stream_with_stat():
    UniproxyCounter.init()
    UniproxyTimings.init()
    init_timing_eq = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')
    init_timing_ne = get_timing('ASR_EOU_NE_TIME_DIALOGGENERAL')

    answers = [
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True, 'responseCode': 200}
    ]
    yaldi_stream_holder = YaldiStreamHolder(answers)
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()

    final_timing_eq = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')
    final_timing_ne = get_timing('ASR_EOU_NE_TIME_DIALOGGENERAL')
    assert not are_timings_equal(init_timing_eq, final_timing_eq)
    assert are_timings_equal(init_timing_ne, final_timing_ne)
    assert GlobalCounter.ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM.value() == 1


def test_yaldi_stream_with_stat_2():
    UniproxyCounter.init()
    UniproxyTimings.init()
    init_timing_eq = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')
    init_timing_ne = get_timing('ASR_EOU_NE_TIME_DIALOGGENERAL')

    answers = [
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True, 'responseCode': 200}
    ]
    yaldi_stream_holder = YaldiStreamHolder(answers)
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()

    final_timing_eq = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')
    final_timing_ne = get_timing('ASR_EOU_NE_TIME_DIALOGGENERAL')
    assert are_timings_equal(init_timing_eq, final_timing_eq)
    assert not are_timings_equal(init_timing_ne, final_timing_ne)
    assert GlobalCounter.ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM.value() == 0


def test_yaldi_stream_with_stat_3():
    UniproxyCounter.init()
    UniproxyTimings.init()
    init_timing_eq = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')
    init_timing_ne = get_timing('ASR_EOU_NE_TIME_DIALOGGENERAL')

    answers = [
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True, 'responseCode': 200}
    ]
    yaldi_stream_holder = YaldiStreamHolder(answers)
    yaldi_stream_holder.add_chunk()

    final_timing_eq = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')
    final_timing_ne = get_timing('ASR_EOU_NE_TIME_DIALOGGENERAL')
    assert are_timings_equal(init_timing_eq, final_timing_eq)
    assert are_timings_equal(init_timing_ne, final_timing_ne)
    assert GlobalCounter.ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM.value() == 0


def test_equal_timings():
    UniproxyTimings.init()
    answers = [
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True, 'responseCode': 200}
    ]
    yaldi_stream_holder = YaldiStreamHolder(answers)
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    final_timing_for_3 = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')

    UniproxyTimings.init()
    answers = [
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': True, 'responseCode': 200}
    ]
    yaldi_stream_holder = YaldiStreamHolder(answers)
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    final_timing_for_another_3 = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')

    assert are_timings_equal(final_timing_for_3, final_timing_for_another_3)


def test_different_timings():
    UniproxyTimings.init()
    answers = [
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True, 'responseCode': 200}
    ]
    yaldi_stream_holder = YaldiStreamHolder(answers)
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    final_timing_for_3 = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')

    UniproxyTimings.init()
    answers = [
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'Four'}]}], 'endOfUtt': True, 'responseCode': 200}
    ]
    yaldi_stream_holder = YaldiStreamHolder(answers)
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    final_timing_for_2 = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')

    assert not are_timings_equal(final_timing_for_3, final_timing_for_2)


def test_multieou():
    UniproxyTimings.init()
    UniproxyCounter.init()
    answers = [
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': False, 'responseCode': 200},
        {'recognition': [{'words': [{'value': 'OnE'}, {'value': 'TWO'}, {'value': 'THREE'}]}], 'endOfUtt': True, 'responseCode': 200}
    ]
    yaldi_stream_holder = YaldiStreamHolder(answers)
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    final_timing_1 = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')

    assert GlobalCounter.ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM.value() == 1

    UniproxyTimings.init()
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    yaldi_stream_holder.add_chunk()
    final_timing_2 = get_timing('ASR_EOU_EQ_TIME_DIALOGGENERAL')

    assert are_timings_equal(final_timing_1, final_timing_2)  # check that each eou has it's own timings
    assert GlobalCounter.ASR_PARTIAL_EQ_DIALOGGENERAL_SUMM.value() == 2  # check that each eou increment counter


def test_get_url():

    class FakeClass:
        def fake_callback(self, *args, **kwargs):
            pass

    fake_class = FakeClass()
    yaldi_stream = YaldiStream(fake_class.fake_callback, fake_class.fake_callback, {'topic': 'dialog-maps', 'lang': 'tr', 'request': {'experiments': {}}}, 'session_id', 'message_id')
    yaldi_stream.mapped_lang = 'aaa'
    yaldi_stream.mapped_topic = 'bbb'
    assert yaldi_stream._get_url_path() == '/aaa/bbb/'
    yaldi_stream.mapped_lang = 'tr-TR'
    yaldi_stream.mapped_topic = 'dialogmapsgpu'
    assert yaldi_stream._get_url_path() == '/ru-ru/dialogmapsgpu/'
