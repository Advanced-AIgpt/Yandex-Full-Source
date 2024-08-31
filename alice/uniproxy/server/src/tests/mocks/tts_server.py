from .proto_server import ProtoServer, AsyncProtoStream
from . import DBG, INFO
from alice.uniproxy.library.settings import config
from voicetech.library.proto_api.ttsbackend_pb2 import Generate, GenerateResponse

from tornado.ioloop import IOLoop


def get_tts_endpoint():
    return (config["ttsserver"]["host"], config["ttsserver"]["port"])


def set_tts_endpoint(endpoint):
    config.set_by_path("ttsserver.host", endpoint[0])
    config.set_by_path("ttsserver.port", endpoint[1])


async def default_stream_handler(stream: AsyncProtoStream):
    request = await stream.read_protobuf(Generate)
    DBG("Received Generate:\n{}".format(request))

    response = GenerateResponse(
        completed=True,
        audioData="raw audio generated for '{}'".format(request.text).encode("utf-8")
    )
    await stream.send_protobuf(response)
    DBG("Sent GenerateResponse:\n{}".format(response))


class TtsServerMock:
    def __enter__(self):
        self.server.listen(address=self.__endpoint[0], port=self.__endpoint[1])
        self.__endpoint = (self.__endpoint[0], self.server.port)
        IOLoop.current().spawn_callback(self.on_new_stream)

        self.__orig_endpoint = get_tts_endpoint()
        set_tts_endpoint(self.__endpoint)

        INFO("TTS server is listening on {}:{}".format(self.__endpoint[0], self.__endpoint[1]))
        return self

    def __exit__(self, exp_type, exp_value, traceback):
        self.server.stop()
        set_tts_endpoint(self.__orig_endpoint)
        INFO("TTS server endpoint is restored to {}:{}".format(self.__orig_endpoint[0], self.__orig_endpoint[1]))

    def __init__(self, address="localhost", port=None, stream_handler=default_stream_handler):
        self.__orig_endpoint = None
        self.__endpoint = (address, port)
        self.server = ProtoServer()
        self.stream_handler = stream_handler

    async def accept_proto_stream(self, timeout=None) -> AsyncProtoStream:
        return await self.server.handle_proto_stream(accept=True, timeout=timeout)

    async def reject_proto_stream(self, timeout=None):
        await self.server.handle_proto_stream(accept=False, timeout=timeout)

    async def on_new_stream(self):
        DBG("Waiting for incoming TTS stream...")
        while True:
            stream = await self.accept_proto_stream()
            if stream is None:
                INFO("Server is stopped")
                return
            INFO("Accepted protobuf connection for {}".format(stream.uri))
            IOLoop.current().spawn_callback(self.stream_handler, stream)
