import tornado
from datetime import datetime

from alice.uniproxy.library.backends_bio import ContextStorage
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.backends_common.mocks import ProtoStreamServerMock

from voicetech.library.proto_api.yabio_pb2 import YabioContext
from voicetech.library.proto_api.yabio_pb2 import YabioRequest, AddData, UserScore
from voicetech.library.proto_api.yabio_pb2 import YabioResponse, AddDataResponse
from voicetech.library.proto_api.yaldi_pb2 import ResponseCode
from voicetech.library.proto_api.yabio_pb2 import YabioVoiceprint

from yatest.common.network import PortManager

N_voiceprint = 10

_g_yabio_mock = None
_g_yabio_initialized = False
_g_yabio_initializing = False
_g_yabio_future = tornado.concurrent.Future()


def get_vp(num, spotter, enrolling):
    t = 'some_req_text_' + str(num)
    if spotter:
        t = 'some_spotter_text_' + str(num)
    request_id = 'reqid_' + str(num)
    if enrolling:
        request_id = 'enroll_id_' + str(num)
    return YabioVoiceprint(
        request_id=request_id,
        compatibility_tag='tag_' + str(num),
        source='spotter' if spotter else 'request',
        format='format',
        voiceprint=[0.1, 0.5, 1.0],
        mds_url='mds_url' if not enrolling else None,
        timestamp=int(datetime.now().timestamp() - num),
        reg_num=N_voiceprint + 1 - num,
        text=t,
        device_model='model',
        device_id='device_id',
        device_manufacturer='device_manufacturer',
    )


def init_yabio_ctx(ctx):
    vps = []
    for i in range(0, N_voiceprint):
        vps.extend([get_vp(i, True, False), get_vp(i, False, False)])
    ctx.users.add(user_id='user_id', voiceprints=vps)

    for i in range(0, N_voiceprint):
        ctx.enrolling.extend([get_vp(i, True, True), get_vp(i, False, True)])


class ContextStorageMock(ContextStorage):
    @tornado.gen.coroutine
    def load(self):
        if self.yabio_context is None:
            self.yabio_context = YabioContext(group_id=self.group_id)
            init_yabio_ctx(self.yabio_context)

    @tornado.gen.coroutine
    def save(self):
        pass


class UnisystemMock:

    def __init__(self, session_id, device_model, device_manuf):
        self.hostname = 'localhost'
        self.session_id = session_id
        self.device_model = device_model
        self.device_manuf = device_manuf
        self.uaas_bio_flags = 'some_uaas_flags'

    def get_yabio_storage(self, group_id):
        return ContextStorageMock(
            self.session_id,
            group_id=group_id,
            dev_model=self.device_model,
            dev_manuf=self.device_manuf
        )

    def increment_stats(*args, **kwargs):
        pass


# ------------------------------------------------------------------------------------------------
class YabioServerMock(ProtoStreamServerMock):
    def __init__(self, host, port):
        super().__init__(host, port)
        self.inited = False
        self.msg_count = 0
        self.was_last_spotter_chunk = False
        self.spotter = False
        self.group_id = None

    async def on_upgrade(self, start_line, iheaders):
        assert start_line.path == '/bio'
        return None

    async def on_proto(self, proto):
        Logger.get().info('on_proto')
        if not self.inited:
            self.inited = True
            req = YabioRequest()
            req.ParseFromString(proto)
            assert req.experiments == 'some_uaas_flags'
            resp = self.get_initial_response(req)
            return resp.SerializeToString()

        try:
            req = AddData()
            req.ParseFromString(proto)
            resp = self.add_data(req)
            if req.HasField('lastSpotterChunk') and req.lastSpotterChunk:
                self.was_last_spotter_chunk = True
            return resp.SerializeToString() if resp else None
        except Exception as e:
            print(e)
            resp = AddDataResponse()
            resp.responseCode = ResponseCode.InvalidParams
            return resp.SerializeToString()

    def get_initial_response(self, req):
        self.group_id = req.context.group_id
        self.spotter = req.spotter
        resp = YabioResponse()
        resp.responseCode = ResponseCode.OK
        resp.hostname = 'yabiohost'
        return resp

    def add_data(self, addData):
        if addData.HasField('audioData'):
            self.msg_count += 1
        if addData.needResult or addData.lastChunk:
            resp = AddDataResponse()
            resp.responseCode = ResponseCode.OK
            resp.messagesCount = self.msg_count
            resp.supported_tags[:] = ['tag_1', 'tag_2']
            resp.scores_with_mode.add(mode='mode1', scores=[UserScore(user_id='user_id', score=0.5)])
            resp.scores_with_mode.add(mode='mode2', scores=[UserScore(user_id='user_id', score=0.9)])
            if self.spotter and not self.was_last_spotter_chunk:
                resp.context.group_id = self.group_id
                resp.context.enrolling.extend([get_vp(1, True, True)])
            else:
                resp.context.group_id = self.group_id
                resp.context.enrolling.extend([get_vp(1, False, True)])

            return resp
        return None


# ------------------------------------------------------------------------------------------------
@tornado.gen.coroutine
def wait_for_mock():
    Logger.init('unittest', True)
    log = Logger.get()
    global _g_yabio_mock, _g_yabio_initialized, _g_yabio_initializing, _g_yabio_future

    if _g_yabio_initialized:
        return True

    if _g_yabio_initializing:
        yield _g_yabio_future
        return True

    _g_yabio_initializing = True

    with PortManager() as pm:
        port = pm.get_port()
        config.set_by_path('yabio.host', 'localhost')
        config.set_by_path('yabio.port', port)

        log.info('starting yabio mock server at port', port)
        _g_yabio_mock = YabioServerMock('localhost', port)
        _g_yabio_mock.start()

        yield tornado.gen.sleep(0.2)

        log.info('starting yabio mock server at port %d done' % port)

    _g_yabio_initialized = True
    _g_yabio_future.set_result(True)

    UniproxyCounter.init()
    return True
