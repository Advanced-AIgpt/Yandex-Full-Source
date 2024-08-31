from tornado.process import Subprocess
import tornado.web

import yatest.common

from alice.uniproxy.library.auth.tvm_daemon_client import TvmDaemonClient

from .common import BaseServerMock

# TVM-daemon http api:
# https://wiki.yandex-team.ru/passport/tvm2/tvm-daemon/#httpapitvm-demona


class TvmKnifeError(Exception):
    def __init__(self, message):
        super(TvmKnifeError, self).__init__("TvmKnifeError({})".format(message))


class TvmKnife:
    @classmethod
    def _run_proc(cls, args):
        binary = yatest.common.binary_path("passport/infra/tools/tvmknife/bin/tvmknife")
        return Subprocess([binary] + args, stdout=Subprocess.STREAM)

    @classmethod
    async def _wait_proc(cls, proc):
        if await proc.wait_for_exit(raise_error=False):
            raise TvmKnifeError("Non-zero return code")

    @classmethod
    async def ticket(cls, src, dst, ticket_type="service"):
        proc = cls._run_proc([
            "unittest",
            ticket_type,
            "--src", str(src),
            "--dst", str(dst),
        ])

        ticket = (await proc.stdout.read_until_close()).decode("utf-8").strip()
        await cls._wait_proc(proc)
        return ticket

    @classmethod
    async def keys(cls):
        proc = cls._run_proc([
            "unittest",
            "public_keys",
        ])

        keys = (await proc.stdout.read_until_close()).decode("utf-8").strip()
        await cls._wait_proc(proc)
        return keys

    @classmethod
    async def parse_ticket(cls, ticket):
        proc = cls._run_proc([
            "parse_ticket",
            "--ticket", ticket,
        ])

        await proc.stdout.read_until(b"\n\nBody\n")
        body = (await proc.stdout.read_until_close()).decode("utf-8")

        await cls._wait_proc(proc)

        info = dict()
        for line in body.split("\n"):
            parts = line.split(": ", 2)
            if len(parts) == 2:
                key, value = parts
                info[key] = value

        return int(info["Src"]), int(info["Dst"])


class TvmServerMock(BaseServerMock):
    class PingHandler(tornado.web.RequestHandler):
        async def get(self):
            self.write("OK")
            self.set_status(200)

    class BaseHandler(tornado.web.RequestHandler):
        def initialize(self, auth_token=None):
            self.auth_token = auth_token

        async def prepare(self):
            if self.auth_token:
                if self.auth_token != self.request.headers.get("Authorization"):
                    self.set_status(403, reason="Invalid authorization token")
                    self.finish()

    class BaseHandlerWithConfig(BaseHandler):
        def initialize(self, config, auth_token=None):
            super(TvmServerMock.BaseHandlerWithConfig, self).initialize(auth_token)

            self.default_tvm_id = None
            self.tvm_alias_to_id = dict()
            for client_alias, client_config in config["clients"].items():
                self.tvm_alias_to_id[client_alias] = client_config["self_tvm_id"]
                for dst_alias, dst_config in client_config["dsts"].items():
                    self.tvm_alias_to_id[dst_alias] = dst_config["dst_id"]

                if len(config["clients"]) == 1:
                    self.default_tvm_id = client_config["self_tvm_id"]

            self.tvm_id_to_alias = dict((v, k) for k, v in self.tvm_alias_to_id.items())

        def get_tvm_id(self, alias):
            if isinstance(alias, str):
                return self.tvm_alias_to_id.get(alias) or int(alias)
            return alias

    class TicketsHandler(BaseHandlerWithConfig):
        async def get(self):
            if self.default_tvm_id:
                src = self.get_argument("src", self.default_tvm_id)
            else:
                src = self.get_argument("src")

            src = self.get_tvm_id(src)
            dsts = self.get_argument("dsts")

            response = dict()
            for dst in dsts.split(","):
                dst = self.get_tvm_id(dst)
                response[self.tvm_id_to_alias[dst]] = {
                    "ticket": await TvmKnife.ticket(src, dst),
                    "tvm_id": dst,
                }

            self.set_status(200)
            self.write(response)

    class ChecksrvHandler(BaseHandlerWithConfig):
        async def get(self):
            if self.default_tvm_id:
                self_id = self.get_argument("dst", self.default_tvm_id)
            else:
                self_id = self.get_argument("dst")

            self_id = self.get_tvm_id(self_id)
            ticket = self.request.headers.get("X-Ya-Service-Ticket")

            if not ticket:
                self.set_status(400)
                self.write({
                    "error": "Header X-Ya-Service-Ticket not found",
                })
            else:
                src = None
                dst = None
                try:
                    src, dst = await TvmKnife.parse_ticket(ticket)
                except TvmKnifeError:
                    pass

                if dst == self_id:
                    self.set_status(200)
                    self.write({
                        "src": src,
                        "dst": dst,
                    })
                elif dst:
                    self.set_status(400)
                    self.write({
                        "error": f"Wrong ticket dst, expected {self_id}, got {dst}",
                    })
                else:
                    self.set_status(400)
                    self.write({
                        "error": "Invalid tvm ticket",
                    })

    class KeysHandler(BaseHandler):
        def initialize(self, auth_token):
            super(TvmServerMock.KeysHandler, self).initialize(auth_token)

        async def get(self):
            self.write(await TvmKnife.keys())

    def __init__(self, port_manager=None, pubkey=None, config=None, auth_token=None):
        self.auth_token = auth_token

        handlers = [
            (r"/2/keys", self.KeysHandler, {"auth_token": auth_token}),
            (r"/tvm/keys", self.KeysHandler, {"auth_token": auth_token}),
        ]

        if config:
            handlers.extend([
                (r"/2/checksrv", self.ChecksrvHandler, {"config": config, "auth_token": auth_token}),
                (r"/tvm/checksrv", self.ChecksrvHandler, {"config": config, "auth_token": auth_token}),

                (r"/2/tickets", self.TicketsHandler, {"config": config, "auth_token": auth_token}),
                (r"/tvm/tickets", self.TicketsHandler, {"config": config, "auth_token": auth_token}),
            ])

        app = tornado.web.Application(handlers)
        self.init(app, port_manager=port_manager)

    def get_daemon_client(self, service_tvm_id):
        return TvmDaemonClient(
            auth_token=self.auth_token,
            port=self.port,
            self_alias=service_tvm_id,
        )
