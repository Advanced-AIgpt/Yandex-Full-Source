import logging
import os
import json
import base64
from uuid import uuid4
import yatest.common
import yatest.common.network

import tornado.httpclient
import tornado.httpserver
import tornado.websocket
import tornado.web
import tornado.gen
import tornado.ioloop

from alice.uniproxy.library.testing.mocks import TvmServerMock
from alice.uniproxy.library.utils.deepupdate import deepupdate


UNIPROXY_BIN_PATH = yatest.common.binary_path("alice/uniproxy/bin/uniproxy/uniproxy")
UNIPROXY_SETTINGS_PATH = yatest.common.source_path("alice/uniproxy/bin/uniproxy/tests/data/settings.json")


logging.basicConfig(level=logging.DEBUG)


def _prepare_settings(dst_path, src_path, patch=None):
    from alice.uniproxy.library.utils.deepupdate import deepupdate

    with open(src_path, "r") as fin:
        settings = json.load(fin)

    if patch is not None:
        deepupdate(settings, patch, copy=False)

    with open(dst_path, "w") as fout:
        json.dump(settings, fout, indent=4)


class Environment:
    def __init__(self, settings_path, port_manager=None):
        self.pm = port_manager or yatest.common.network.PortManager()

        self._orig_settings_path = settings_path
        self.settings_path = os.path.join(yatest.common.work_path(), "settings.json")

        self.tvm_mock = TvmServerMock(port_manager=self.pm)

    def start(self):
        self.tvm_mock.start()

        patch = {
            "tvm": {"url": self.tvm_mock.url},
            "experiments": {
                "cfg_folder": yatest.common.source_path("alice/uniproxy/bin/uniproxy/tests/data/experiments")
            }
        }
        _prepare_settings(self.settings_path, self._orig_settings_path, patch)

    def stop(self):
        self.tvm_mock.stop()


class UniProxyProcess:
    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()

    def __init__(self, bin_path=UNIPROXY_BIN_PATH, settings_path=UNIPROXY_SETTINGS_PATH, nproc=2):
        self._exec = None
        self.env = Environment(settings_path)
        self.bin_path = bin_path
        self.port = None
        self.nproc = nproc

    def start(self):
        self.env.start()

        self.port = self.env.pm.get_port()
        command = [self.bin_path, "--port", str(self.port), "--nproc", str(self.nproc)]
        self._exec = yatest.common.execute(
            command,
            env={
                "UNIPROXY_CUSTOM_ENVIRONMENT_TYPE": "development",
                "UNIPROXY_CUSTOM_SETTINGS_FILE": self.env.settings_path
            },
            wait=False
        )
        tornado.ioloop.IOLoop.current().run_sync(self.wait_ready)

    def stop(self):
        import signal
        try:
            self.process.send_signal(signal.SIGINT)
            self._exec.wait(timeout=10)
        finally:
            self.env.pm.release_port(self.port)
            self.port = None
        self.env.stop()

    @property
    def process(self):
        return self._exec.process

    @property
    def url(self):
        if self.port is None:
            return None
        return "http://localhost:{}".format(self.port)

    @property
    def ws_url(self):
        if self.port is None:
            return None
        return "ws://localhost:{}/uni.ws".format(self.port)

    async def wait_ready(self):
        pinger = tornado.httpclient.AsyncHTTPClient()
        while self._exec.running:
            resp = await pinger.fetch(self.url + "/ping", raise_error=False)
            if resp.code == 200:
                logging.debug("Uniproxy is ready; listening on {} port".format(self.port))
                return
            await tornado.gen.sleep(0.1)
        raise RuntimeError("Uniproxy process unexpectendly terminated")

    async def ws_connect(self) -> tornado.websocket.WebSocketClientConnection:
        url = self.ws_url
        logging.debug("Connect to Uniproxy={}".format(url))
        return await tornado.websocket.websocket_connect(url)

    async def get_unistat(self):
        resp = await tornado.httpclient.AsyncHTTPClient().fetch(self.url + "/unistat")
        metrics = json.loads(resp.body.decode("utf-8"))
        res = {}
        for m in metrics:
            res[m[0]] = m[1]
        return res


class Events:
    class System:
        @staticmethod
        def SynchronizeState(message_id=None, auth_token=None, oauth_token=None, uuid=None, speechkit_version=None, is_mssngr=False, uniproxy2=None):
            ret = {
                "event": {
                    "header": {
                        "namespace": "System",
                        "name": "SynchronizeState",
                        "messageId": message_id or str(uuid4())
                    },
                    "payload": {
                        "auth_token": auth_token or "developers-simple-key",
                        "uuid": uuid or str(uuid4()),
                        "speechkitVersion": speechkit_version or "1.0.0"
                    }
                }
            }

            if oauth_token is not None:
                ret["event"]["payload"]["oauth_token"] = oauth_token

            if is_mssngr:
                mssngr = {
                    "event": {
                        "payload": {
                            "Messenger": {
                                "anonymous": True,
                                "version": 3,
                            },
                        }
                    }
                }

                ret = deepupdate(ret, mssngr)

            if uniproxy2 is not None:
                ret["event"]["payload"]["uniproxy2"] = uniproxy2

            return ret

        @staticmethod
        def EchoRequest(message_id=None, payload=None):
            return {
                "event": {
                    "header": {
                        "namespace": "System",
                        "name": "EchoRequest",
                        "messageId": message_id or str(uuid4()),
                    },
                    "payload": payload or {"lang": "ru-RU", "topic": "queries", "application": "test"}
                }
            }

        @staticmethod
        def SetState(original_payload, session_context, message_id=None):
            ret = {
                "event": {
                    "header": {
                        "namespace": "System",
                        "name": "SetState",
                        "messageId": message_id or str(uuid4())
                    },
                    "payload": {
                        "original_payload": original_payload,
                        "session_context": base64.encodebytes(session_context.SerializeToString()).decode("ascii").replace("\n", "")
                    }
                }
            }
            return ret

    class Uniproxy2:
        def ApplySessionContext(message_id=None, payload=None):
            return {
                "event": {
                    "header": {
                        "namespace": "Uniproxy2",
                        "name": "ApplySessionContext",
                        "messageId": message_id or str(uuid4()),
                    },
                    "payload": {} if payload is None else payload
                }
            }
