import alice.uniproxy.library.testing
import tornado

from alice.uniproxy.library.backends_bio import YabioStream
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger
from voicetech.library.proto_api.yabio_pb2 import AddDataResponse

from tornado.concurrent import Future

from .mocks import wait_for_mock, N_voiceprint, UnisystemMock

fut = None


def on_msg(*args):
    print(*args)
    global fut
    fut.set_result('OK')


def on_error(*args):
    print(*args)
    global fut
    fut.set_exception(args)
    assert True


@alice.uniproxy.library.testing.ioloop_run
def test_bio():
    yield wait_for_mock()

    session_id = 'test-session'
    message_id = 'test-message-id'
    device_id = 'test-device-id'
    device_manuf = 'test-dev-manuf'
    device_model = 'test-device-model'

    unisystem = UnisystemMock(session_id, device_model, device_manuf)
    params = {
        'biometry_group': 'bio-group',
        'hostname': 'localhost',
        'request': {
            'device_state': {
                'device_id': device_id,
            },
        },
        'uuid': 'uuid',
        'vins': {
            'application': {
                'device_manufacturer': device_manuf,
            }
        },
    }
    stream = YabioStream(YabioStream.YabioStreamType.Score, on_msg, on_error, params, session_id,
                         host='localhost',
                         spotter=True, system=unisystem, message_id=message_id)

    ctx = stream.context_storage.yabio_context

    # wait for connect & handshake
    yield tornado.gen.sleep(0.1)
    assert stream.init_sent is True
    assert stream.message_id == message_id

    assert len(ctx.users[0].voiceprints) == N_voiceprint*2

    global fut

    # send spotter chunks
    fut = Future()
    N_spotter_data = 10
    for i in range(1, N_spotter_data):
        data = 'spotter_data_%d' % i
        stream.add_chunk(data=data.encode())
    stream.add_chunk(last_spotter_chunk=True, need_result=True, text='spotter')

    assert stream.last_chunk is False
    assert stream.need_results == 1
    assert stream.sent_chunks == N_spotter_data
    assert stream.processed_results == 0
    assert stream.has_finish_code is False

    yield fut
    assert stream.processed_results == 1
    assert stream.has_finish_code is False

    # send request chunks
    fut = Future()
    N_data = 30
    for i in range(1, N_data):
        data = 'data_%d' % i
        stream.add_chunk(data=data.encode(), need_result=i == N_data // 2)
    stream.add_chunk(last_chunk=True, need_result=True, text='request')
    yield fut

    assert stream.last_chunk is True
    assert stream.need_results == 3
    assert stream.sent_chunks == N_data + N_spotter_data
    assert stream.processed_results == 3
    assert stream.has_finish_code is True

    # wait for `save_new_enrolling_coro()`
    yield tornado.gen.sleep(0.4)
    # check context
    assert len(ctx.users) == 1
    assert ctx.users[0].user_id == 'user_id'
    assert len(ctx.users[0].voiceprints) == 4

    assert len(ctx.enrolling) == 6

    assert ctx.enrolling[0].request_id == 'enroll_id_1'
    assert ctx.enrolling[0].source == 'spotter'
    assert ctx.enrolling[1].request_id == 'enroll_id_1'
    assert ctx.enrolling[1].source == 'request'
    assert ctx.enrolling[2].request_id == 'enroll_id_2'
    assert ctx.enrolling[2].source == 'spotter'
    assert ctx.enrolling[3].request_id == 'enroll_id_2'
    assert ctx.enrolling[3].source == 'request'

    assert ctx.enrolling[4].request_id == message_id
    assert ctx.enrolling[4].source == 'spotter'
    assert ctx.enrolling[4].device_id == device_id
    assert ctx.enrolling[4].device_model == device_model
    assert ctx.enrolling[4].device_manufacturer == device_manuf

    assert ctx.enrolling[5].request_id == message_id
    assert ctx.enrolling[5].source == 'request'
    assert ctx.enrolling[5].device_id == device_id
    assert ctx.enrolling[5].device_model == device_model
    assert ctx.enrolling[5].device_manufacturer == device_manuf

    assert GlobalCounter.YABIO_REGISTERED_USERS_SUMM.value() == 1

    stream.close()


def check_process_add_data_response(create_proto, check_answer):
    Logger.init("uniproxy", is_debug=True)

    session_id = 'test-session'
    message_id = 'test-message-id'
    device_id = 'test-device-id'
    device_manuf = 'test-dev-manuf'
    # device_model = 'test-device-model'

    class FakeSystem:
        def __init__(self, *args, **kwargs):
            self.hostname = 'localhost'
            self.uaas_bio_flags = None
            self.use_balancing_hint = True
            self.use_spotter_glue = False
            self.use_laas = True
            self.use_datasync = True
            self.use_personal_cards = True
            self.ab_asr_topic = None

        def increment_stats(self, *args, **kwargs):
            pass

    unisystem = FakeSystem()
    params = {
        'biometry_group': 'bio-group',
        'hostname': 'localhost',
        'request': {
            'device_state': {
                'device_id': device_id,
            },
        },
        'uuid': 'uuid',
        'vins': {
            'application': {
                'device_manufacturer': device_manuf,
            }
        },
    }

    visited_on_data = 0

    def on_data(result, processed_chunks):
        check_answer(result)
        nonlocal visited_on_data
        visited_on_data += 1

    stream = YabioStream(YabioStream.YabioStreamType.Classify, on_data, on_error, params, session_id,
                         host='localhost',
                         spotter=True, system=unisystem, message_id=message_id)

    counter = 0
    def fake_read_protobuf(protos, callback):
        nonlocal counter
        if counter != 0:
            res = None
        else:
            res = create_proto()
            counter += 1
        callback(res)

    stream.read_protobuf = fake_read_protobuf

    def fake_is_closed():
        return stream._closed

    stream.is_closed = fake_is_closed
    stream._closed = False
    stream.last_chunk = True
    stream.read_add_data_response(True, True)
    stream.close()
    assert visited_on_data == 1


def test_classification_process_add_data_response_empty():
    def create_proto():
        res = AddDataResponse()
        return res

    def check_answer(res):
        assert len(res['classification_results']) == 0

    check_process_add_data_response(create_proto, check_answer)


def test_classification_process_add_data_response_full():
    def create_proto():
        res = AddDataResponse()
        classificationSimple = res.classificationResults.add()
        classificationSimple.tag = 'tag'
        classificationSimple.classname = 'classname'
        return res

    def check_answer(res):
        assert len(res['classification_results']) == 1
        assert res['classification_results'][0]['tag'] == 'tag'
        assert res['classification_results'][0]['classname'] == 'classname'

    check_process_add_data_response(create_proto, check_answer)
