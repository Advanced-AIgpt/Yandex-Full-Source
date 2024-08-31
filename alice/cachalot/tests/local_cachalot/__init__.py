import json
import os

from library.python import resource

from alice.cuttlefish.tests.common import ApphostServant
from alice.cachalot.client import CachalotClient, SyncCachalotClient


def deepupdate(target, patch):
    for k, v in patch.items():
        if isinstance(v, dict) and (k in target):
            target[k] = deepupdate(target[k], v)
        else:
            target[k] = v
    return target


class LocalCachalotStorageConfig:
    def __init__(self):
        self.ydb_database = str()
        self.ydb_endpoint = str()
        self.fakes = list()

    def with_ydb_database(self, value):
        self.ydb_database = value
        return self

    def with_ydb_endpoint(self, value):
        self.ydb_endpoint = value
        return self

    def add_fake(self, value):
        self.fakes.append(value)
        return self


class LocalCachalot(ApphostServant):
    def __init__(
        self,
        bin_dir,
        work_dir,
        port_manager,
        storage_config: LocalCachalotStorageConfig,
        env={},
    ):
        super().__init__(port_manager, env)

        self.binary_path = os.path.join(bin_dir, "cachalot")
        self.config_path = os.path.join(work_dir, "config.json")
        self.log_path = os.path.join(work_dir, "cachalot.evlog")
        self.rtlog_path = os.path.join(work_dir, "cachalot.rtlog")
        self.storage_config = storage_config

        if not os.path.exists(work_dir):
            os.makedirs(work_dir)

    def _run_command(self, cmd):
        # Should be implemented in subclass
        raise NotImplementedError()

    def _update_sub_config(self, sub_config, path, value, create=None):
        key = path[0]

        if create and (key not in sub_config):
            sub_config[key] = dict()

        if len(path) == 1:
            sub_config[key].update(value)
        else:
            self._update_sub_config(sub_config[key], path[1:], value, create=create)

    def _set_fake(self, sub_config, path):
        self._update_sub_config(sub_config, path, {
            "IsFake": True,
        }, create=True)

    def _set_ydb(self, sub_config, path):
        self._update_sub_config(sub_config, path, {
            "Endpoint": self.storage_config.ydb_endpoint,
            "Database": self.storage_config.ydb_database,
            "ReadTimeoutSeconds": 5.0,
            "WriteTimeoutSeconds": 5.0,
        })

    def _make_config(self):
        config = {}

        config = deepupdate(config, json.loads(resource.find("/cachalot-activation.json")))
        config = deepupdate(config, json.loads(resource.find("/cachalot-context.json")))
        config = deepupdate(config, json.loads(resource.find("/cachalot-gdpr.json")))
        config = deepupdate(config, json.loads(resource.find("/cachalot-mm.json")))
        config = deepupdate(config, json.loads(resource.find("/cachalot-tts.json")))

        config["Server"]["Port"] = self.http_port
        config["Server"]["GrpcPort"] = self.grpc_port
        config["Log"]["Filename"] = self.log_path
        config["Log"]["RtLogFilename"] = self.rtlog_path
        config["LockMemory"] = False

        for path in self.storage_config.fakes:
            self._set_fake(config, path)

        for path in (
            ("Activation", "Ydb"),
            ("GDPR", "Ydb"),
            ("Takeout", "Ydb"),
            ("MegamindSession", "Storage", "Ydb"),
            ("Cache", "Storages", "Tts", "Ydb"),
            ("VinsContext", "Ydb"),
            ("YabioContext", "Storage", "YdbClient"),
        ):
            self._set_ydb(config, path)

        self._update_sub_config(config, ("Activation",), {
            "FreshnessDeltaMilliSeconds": 4000,
        })

        with open(self.config_path, 'w') as fout:
            json.dump(config, fout, indent=4)

    def _execute_bin(self):
        self._make_config()

        command = [
            self.binary_path, "run",
            "--config", self.config_path,
        ]
        return self._run_command(command)

    def get_client(self, use_grpc=False):
        grpc_port = self.grpc_port if use_grpc else None
        return CachalotClient("localhost", http_port=self.http_port, grpc_port=grpc_port)

    def get_sync_client(self, use_grpc=False):
        grpc_port = self.grpc_port if use_grpc else None
        return SyncCachalotClient("localhost", http_port=self.http_port, grpc_port=grpc_port)
