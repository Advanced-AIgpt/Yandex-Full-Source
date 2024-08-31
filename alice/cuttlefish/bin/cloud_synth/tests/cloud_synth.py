import alice.cuttlefish.tests.common as test_lib

from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient
from alice.cuttlefish.library.python.testing.constants import ServiceHandles

from voicetech.asr.cloud_engine.api.speechkit.v3.service.tts.tts_service_pb2_grpc import Synthesizer
from voicetech.asr.cloud_engine.api.speechkit.v3.proto.tts.tts_pb2 import UtteranceSynthesisResponse

import logging
import yatest.common
import yatest.common.network


logging.basicConfig(level=logging.DEBUG)


EVLOGDUMP_BIN_PATH = yatest.common.binary_path("voicetech/tools/evlogdump/evlogdump")
RTLOGDUMP_BIN_PATH = yatest.common.binary_path("alice/rtlog/evlogdump/evlogdump")
CLOUD_SYNTH_BIN_PATH = yatest.common.binary_path("alice/cuttlefish/bin/cloud_synth/cloud_synth")


class CloudSynth(test_lib.ApphostServant):
    @property
    def eventlog_path(self):
        return yatest.common.test_output_path("cloud_synth.evlog")

    @property
    def rtlog_path(self):
        return yatest.common.test_output_path("cloud_synth.rtlog")

    def __init__(self, bin_path=CLOUD_SYNTH_BIN_PATH, env={}, args=[]):
        super().__init__(yatest.common.network.PortManager(), env)
        self.bin_path = bin_path
        self._evlog_dump = self.LogDump(EVLOGDUMP_BIN_PATH, self.eventlog_path, subprocess_run=yatest.common.execute)
        self._rtlog_dump = self.LogDump(RTLOGDUMP_BIN_PATH, self.rtlog_path, subprocess_run=yatest.common.execute)
        self._extra_args = args

    def _execute_bin(self):
        command = [
            self.bin_path,
            "-V",
            f"server.http.port={self.http_port}",
            "-V",
            f"server.grpc.port={self.grpc_port}",
            "-V",
            f"server.log.eventlog={self.eventlog_path}",
            "-V",
            f"server.rtlog.file={self.rtlog_path}",
            "-V",
            "server.lock_memory=false",
        ]
        command += self._extra_args
        return yatest.common.execute(command, wait=False, env=self.env)

    def _after_start(self):
        self._apphost_grpc_client = AppHostGrpcClient(self.grpc_endpoint)
        logging.info("gRPC apphost client for tts adapter is ready")

    def create_apphost_grpc_stream(self, path=ServiceHandles.TTS, **kwargs):
        logging.debug(f"Create gRPC apphost stream: path=/{path}")
        return self._apphost_grpc_client.create_stream(path=path, **kwargs)

    # Use this method if you want to make a single (non-stream) request to servant
    async def make_grpc_request(self, items, timeout=None, path=ServiceHandles.TTS, **kwargs):
        async with self.create_apphost_grpc_stream(path=path, timeout=timeout, **kwargs) as stream:
            logging.debug(f"Write items to gRPC apphost stream: {items}")
            stream.write_items(items=items, last=True)

            logging.debug(f"Await response from gRPC apphost stream: timeout={timeout}")
            response = await stream.read(timeout=timeout)
            logging.debug(f"Received response from gRPC apphost stream: {response}")

            return response

    def get_eventlog(self, from_beginning=False):
        for l in self._evlog_dump.lines(from_beginning):
            yield l

    def get_rtlog(self, from_beginning=False):
        for l in self._rtlog_dump.lines(from_beginning):
            yield l


class SynthesizerMock(Synthesizer):
    def __init__(self):
        self.requests = []

    def UtteranceSynthesis(self, request, context):
        self.requests.append(request)
        for data in (b"123", b"", b"456"):
            resp = UtteranceSynthesisResponse()
            resp.audio_chunk.data = data
            yield resp