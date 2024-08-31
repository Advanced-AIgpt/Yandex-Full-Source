from alice.uniproxy.library.unisystem.unisystem import UniSystem
from alice.uniproxy.library.utils import Srcrwr, GraphOverrides
from tornado.httpclient import HTTPRequest
import tornado


@tornado.gen.coroutine
def fake_foo():
    return 'smth'


class FakeUniWebSocket:
    unistat_handler_name = 'uni_ws'

    def __init__(self):
        self.output_messages = []
        self.request = HTTPRequest('http://localhost', 'GET', headers={'X-RTLog-Token': 'fake-rtlog-token'})
        self.system = UniSystem(self)
        self.system.get_bb_user_ticket = fake_foo
        self.client_ip = '127.0.0.1'
        self.system.origin = self.request.headers.get('X-Forwarded-Host', self.request.headers.get('Host'))
        self.system.x_yamb_token = self.request.headers.get(
            'x-yamb-token',
            self.request.headers.get('X-Yamb-Token'),
        )
        self.system.x_yamb_token_type = self.request.headers.get(
            'x-yamb-token-type',
            self.request.headers.get('X-Yamb-Token-Type'),
        )
        self.system.user_agent = self.request.headers.get('User-Agent')
        self.system.srcrwr = Srcrwr(self.request.headers)
        self.system.graph_overrides = GraphOverrides([])

    def write_message(self, msg):
        self.output_messages.append(msg)
