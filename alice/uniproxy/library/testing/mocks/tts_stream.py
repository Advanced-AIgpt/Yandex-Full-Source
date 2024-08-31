from typing import Callable
from voicetech.library.proto_api.ttsbackend_pb2 import GenerateResponse


class FakeTtsStream:
    STREAMS = []

    def __init__(
            self,
            callback: Callable[[GenerateResponse], None],
            error_callback: Callable[[str], None],
            params,
            host=None,
            rt_log=None,
            unistat_counter='tts',
            system=None,
            message_id=None,
    ):
        FakeTtsStream.STREAMS.append(self)
        self.callback = callback

    def send_response(self):
        response = GenerateResponse(
            completed=True,
            audioData=b'\0\0',
        )
        self.callback(response)


class FakeCacheStorageClient:
    CLIENTS = []

    def __init__(self):
        FakeCacheStorageClient.CLIENTS.append(self)
        self.data = dict()

    async def lookup(self, key):
        return self.data.get(key)

    async def store(self, key, value, ttl=None):
        self.data[key] = value
