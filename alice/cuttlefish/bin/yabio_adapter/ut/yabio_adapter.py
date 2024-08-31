import logging
import yatest.common
import yatest.common.network
import json
import grpc

from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient
import alice.cuttlefish.tests.common as test_lib

import apphost.lib.grpc.protos.service_pb2_grpc as apphost_grpc

YABIO_ADAPTER_PROD_CONFIG_PATH = "alice/cuttlefish/package/yabio_adapter/config.json"
YABIO_ADAPTER_BIN_PATH = yatest.common.binary_path("alice/cuttlefish/bin/yabio_adapter/yabio_adapter")
EVLOGDUMP_BIN_PATH = yatest.common.binary_path("voicetech/tools/evlogdump/evlogdump")
RTLOGDUMP_BIN_PATH = yatest.common.binary_path("alice/rtlog/evlogdump/evlogdump")


class YabioAdapter(test_lib.ApphostServant):
    @property
    def eventlog_path(self):
        return yatest.common.test_output_path("yabio_adapter.evlog")

    @property
    def rtlog_path(self):
        return yatest.common.test_output_path("yabio_adapter.rtlog")

    def __init__(self, bin_path=YABIO_ADAPTER_BIN_PATH, env={}, args=[]):
        super().__init__(yatest.common.network.PortManager(), env, wait_ready_timeout=30)
        self.bin_path = bin_path
        self._evlog_dump = self.LogDump(EVLOGDUMP_BIN_PATH, self.eventlog_path, subprocess_run=yatest.common.execute)
        self._rtlog_dump = self.LogDump(RTLOGDUMP_BIN_PATH, self.rtlog_path, subprocess_run=yatest.common.execute)
        self._extra_args = args
        self._pm = yatest.common.network.PortManager()

    def _make_config(self):
        with open(yatest.common.source_path(YABIO_ADAPTER_PROD_CONFIG_PATH), "r") as f:
            cfg = json.load(f)

        cfg["server"]["http"]["port"] = self.http_port
        cfg["server"]["grpc"]["port"] = self.grpc_port
        cfg["server"]["lock_memory"] = False
        cfg["server"]["log"]["eventlog"] = self.eventlog_path
        cfg["server"]["rtlog"]["file"] = self.rtlog_path
        # use internal fake impl (instead real external yabio-server)
        cfg["yabio"]["protocol_version"] = 0
        # Do not wait user sessions on shutdown
        self._handler_path = cfg["yabio"]["path"]
        return cfg

    def _execute_bin(self):
        self.config = self._make_config()
        config_path = yatest.common.test_output_path("yabio_adapter.json")
        with open(config_path, "w") as f:
            json.dump(self.config, f, indent=2)
        command = [self.bin_path, "-c", config_path]
        command += self._extra_args
        return yatest.common.execute(command, wait=False, env=self.env)

    def _after_start(self):
        self._grpc_channel = grpc.insecure_channel(self.grpc_endpoint)
        self._grpc_stub = apphost_grpc.TServantStub(self._grpc_channel)
        logging.info("gRPC for yabio_adapter is ready")

        self._apphost_grpc_client = AppHostGrpcClient(self.grpc_endpoint)
        logging.info("gRPC apphost client for yabio_adapter is ready")

    # Make apphost request with pure grpc (no special client)
    # Likely to be removed in the future
    def make_grpc_request(self, request):
        request.Path = self._handler_path
        logging.debug(f"Send TServiceRequest: {request}")
        response = self._grpc_stub.Invoke((r for r in [request]))
        logging.debug(f"Received TServiceResponse: {response}")
        return response

    def get_eventlog(self, from_beginning=False):
        for l in self._evlog_dump.lines(from_beginning):
            yield l

    def get_rtlog(self, from_beginning=False):
        for l in self._rtlog_dump.lines(from_beginning):
            yield l
