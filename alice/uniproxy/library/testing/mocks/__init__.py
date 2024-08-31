from .async_http_stream import FakeAsyncHttpStream
from .blackbox_server import BlackboxServerMock
from .mds_server import MdsServerMock
from .log_collector import LogCollector
from .personal_data_helper import FakePersonalDataHelper
from .rt_log import FakeRtLog, fake_begin_request
from .server import QueuedTcpServer, read_http_request, write_http_response
from .spotter_stream import FakeSpotterStream
from .tts_stream import FakeTtsStream, FakeCacheStorageClient
from .tvm_server import TvmServerMock, TvmKnife
from .uaas import FakeFlagsJsonClient
from .uni_web_socket import FakeUniWebSocket
from .yabio_stream import FakeYabioStream
from .yaldi_stream import FakeYaldiStream, fake_get_yaldi_stream_type


def reset_mocks():
    FakeAsyncHttpStream.REQUESTS = []
    FakeCacheStorageClient.CLIENTS = []
    FakeFlagsJsonClient.STREAMS = []
    FakeSpotterStream.STREAMS = []
    FakeTtsStream.STREAMS = []
    FakeYabioStream.STREAMS = []
    FakeYaldiStream.STREAMS = []


__all__ = [
    BlackboxServerMock,
    MdsServerMock,
    fake_begin_request,
    fake_get_yaldi_stream_type,
    FakeAsyncHttpStream,
    FakeCacheStorageClient,
    FakeFlagsJsonClient,
    FakePersonalDataHelper,
    FakeRtLog,
    FakeSpotterStream,
    FakeTtsStream,
    FakeUniWebSocket,
    FakeYabioStream,
    FakeYaldiStream,
    LogCollector,
    QueuedTcpServer,
    read_http_request,
    TvmKnife,
    TvmServerMock,
    write_http_response,
]
