from tests.mocks.proto_server import ProtoServer, AsyncProtoStream
from tests.mocks import DBG, INFO
import voicetech.library.proto_api.yaldi_pb2 as protos
from alice.uniproxy.library.settings import config

from tornado.ioloop import IOLoop
import datetime
import logging
import base64


class YaldiServerMock:
    def __enter__(self):
        self.__orig_endpoint = (config["asr"]["yaldi_host"], config["asr"]["yaldi_port"])
        self.server.listen(port=self.__endpoint[0], address=self.__endpoint[1])
        config.set_by_path("asr.yaldi_host", self.__endpoint[1])
        config.set_by_path("asr.yaldi_port", self.server.port)
        INFO("Yaldi server is listening on {}:{}".format(self.__endpoint[1], self.server.port))
        return self

    def __exit__(self, exp_type, exp_value, traceback):
        self.server.stop()
        config.set_by_path("asr.yaldi_host", self.__orig_endpoint[0])
        config.set_by_path("asr.yaldi_port", self.__orig_endpoint[1])

    def __init__(self, port=None, address="localhost"):
        self.__orig_endpoint = None
        self.__endpoint = (port, address)
        self.server = ProtoServer()

    async def accept_proto_stream(self, timeout=None) -> AsyncProtoStream:
        return await self.server.handle_proto_stream(accept=True, timeout=timeout)

    async def reject_proto_stream(self, timeout=None):
        await self.server.handle_proto_stream(accept=False, timeout=timeout)


def __get_response_protobuffs(data: bytes):
    """Yields all protobuf messages packed into 'bytes'
    """
    start, end = 0, 0
    while True:
        start = data.find(b"<", end)
        if start == -1:
            return
        start += 1
        end = data.find(b">", start)
        if end == -1:
            return
        decoded = base64.decodebytes(data[start:end])
        proto = protos.AddDataResponse()
        proto.ParseFromString(decoded)
        yield proto
        end += 1


class VoiceGenerator:
    """
    Its' methods create fake audio data that makes YaldiServerMock send something in response.
    Generated data may be concatenated to produce a few responses.
    """

    @staticmethod
    def protobuf_message(proto):
        return b"<" + base64.encodebytes(proto.SerializeToString()) + b">"

    @staticmethod
    def add_data_response(words, message_count=None, eou=True, confidence=0.8, response_code=protos.OK):
        if not isinstance(words, (list, tuple)):
            words = (words,)
        resp = protos.AddDataResponse()
        resp.responseCode = response_code
        resp.endOfUtt = eou
        if message_count is not None:
            resp.messagesCount = message_count

        recognition = resp.recognition.add()
        recognition.confidence = confidence
        for word in words:
            recognition.words.add(confidence=confidence, value=word)

        return VoiceGenerator.protobuf_message(resp)


async def handle_yaldi_stream_simply(server: YaldiServerMock, timeout=None, on_init_request=None):
    DBG("Waiting fro incoming protobuf stream...")
    ps = await server.accept_proto_stream()
    if ps is None:
        INFO("Server is stopped")
        return
    INFO("Accepted protobuf connection for {}".format(ps.uri))

    # to process next incoming streams
    IOLoop.current().spawn_callback(handle_yaldi_stream_simply, server, timeout=timeout, on_init_request=on_init_request)

    init_request = await ps.read_protobuf(protos.InitRequest, timeout)
    DBG("Received InitRequest:\n{}".format(init_request))

    if on_init_request is None:
        init_response = protos.InitResponse(responseCode=protos.OK, hostname="localhost", topic="some-topic")
    elif callable(on_init_request):
        init_response = on_init_request(init_request)
    else:
        init_response = on_init_request
    await ps.send_protobuf(init_response, timeout)
    DBG("Sent InitResponse:\n{}".format(init_response))

    while True:
        data = await ps.read_protobuf(protos.AddData, timeout)
        if data is None:
            DBG("Connection closed or unexpected message received")
            ps.close()
            return
        DBG("Received AddData:\n{}".format(data))
        for resp in __get_response_protobuffs(data.audioData):
            await ps.send_protobuf(resp, timeout)
            DBG("Send AddDataResponse:\n{}".format(resp))


def handle_yaldi_stream(yaldi, timeout=None, on_init_request=None):
    """ Accepts protobuf connection and answers with messages packed into incoming 'AddData.audioData'
    : param on_init_request: either InitResponse message of callable(msg: InitRequest) -> InitResponse
    """

    if timeout is None:
        timeout = datetime.timedelta(seconds=2)
    IOLoop.current().spawn_callback(handle_yaldi_stream_simply, yaldi, timeout=timeout, on_init_request=on_init_request)
