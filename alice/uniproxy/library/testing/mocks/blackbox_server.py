import tornado.web

from alice.uniproxy.library.auth.mocks import BlackboxMock, BlackboxError

from .common import BaseServerMock


class BlackboxServerMock(BaseServerMock):
    class GetHandler(tornado.web.RequestHandler):
        def initialize(self, mock):
            self.mock = mock

        async def get_oauth(self):
            token = self.get_argument("oauth_token")
            client_ip = self.get_argument("userip")
            ticket = await self.mock.ticket4oauth(token, client_ip, 0)
            return {
                "status": {
                    "value": "VALID",
                    "id": 0,
                },
                "oauth": {
                    "uid": "1234",
                },
                "login": "mock-passport-user",
                "aliases":
                {
                    "13": "mock-passport-user"
                },
                "user_ticket": ticket,
            }

        async def get_sessionid(self):
            sessionid = self.get_argument("sessionid")
            client_ip = self.get_argument("userip")
            client_host = self.get_argument("host")
            ticket = await self.mock.ticket4sessionid(sessionid, client_ip, client_host, '0')
            return {
                "status": {
                    "value": "VALID",
                    "id": 0,
                },
                "oauth": {
                    "uid": "1234",
                },
                "login": "mock-passport-user",
                "user_ticket": ticket,
            }

        async def get(self):
            try:
                method = self.get_argument("method")
                if method == "oauth":
                    self.write(await self.get_oauth())
                elif method == "sessionid":
                    self.write(await self.get_sessionid())
                else:
                    raise BlackboxError("INVALID_PARAMS", 400)
            except BlackboxError as ex:
                self.set_status(400, reason=str(ex))
                self.write({
                    "status": {
                        "value": "INVALID",
                        "id": 5,
                    },
                    "error": str(ex),
                })
            except Exception as ex:
                self.set_status(500, reason=str(ex))

    def __init__(self, port_manager=None, pubkey=None):
        app = tornado.web.Application([
            (r"/", self.GetHandler, {"mock": BlackboxMock()})
        ])
        self.init(app, port_manager=port_manager)
