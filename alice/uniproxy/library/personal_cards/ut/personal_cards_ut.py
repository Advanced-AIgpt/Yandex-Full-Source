import alice.uniproxy.library.testing
import tornado.gen
import tornado.web
import json

from alice.uniproxy.library.personal_cards import PersonalCardsHelper
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter.uniproxy import GlobalCounter, UniproxyCounter

from rtlog import null_logger

from yatest.common.network import PortManager

_g_personal_cards_mock = None
_g_personal_cards_initialized = False
_g_personal_cards_initializing = False
_g_personal_cards_future = tornado.concurrent.Future()


class PersonalCardsApiHandler(tornado.web.RequestHandler):
    acceptable_request_payloads = [
        {
            'card': {
                'card': {
                    'card_id': 'station_billing_12345',
                    'data': {
                        'button_url': 'https://yandex.ru/quasar/id/kinopoisk/promoperiod',
                        'text': 'Активировать Яндекс.Плюс',
                        'min_price': 0
                    },
                'date_from': 1596398659,
                'date_to': 1596405859,
                'type': 'yandex.station_film'
                }
            },
            'uid': '123'
        },
        {
            'auth': {
                'uid': '123'
            },
            'card_id': '*'
        }
    ]

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def post(self):
        Logger.get().info('handling post request {self.request.body}')

        if json.loads(self.request.body) in self.acceptable_request_payloads:
            self.set_status(200)
            self.set_header('Content-Type', 'application/json')
            self.finish('{"result": "Operation success"}')
        else:
            self.set_status(400)
            self.finish('Invalid json')


class PersonalCardsServerMock(object):
    def __init__(self, host, port):
        super().__init__()
        self._app = tornado.web.Application([
            (r'/addPushCards', PersonalCardsApiHandler),
            (r'/dismiss', PersonalCardsApiHandler)
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(port)

    def start(self):
        self._srv.start(1)


@tornado.gen.coroutine
def wait_for_mock():
    Logger.init('personal_cards_test', True)
    log = Logger.get()
    global _g_personal_cards_mock, _g_personal_cards_initialized, _g_personal_cards_initializing

    if _g_personal_cards_initialized:
        return True

    if _g_personal_cards_initializing:
        yield _g_personal_cards_future
        return True

    _g_personal_cards_initializing = True

    with PortManager() as pm:
        port = pm.get_port()

        config.set_by_path('personal_cards.url', 'http://localhost')
        config.set_by_path('personal_cards.port', port)

        log.info(f'Starting personal cards mock server at port {port}')
        _g_personal_cards_mock = PersonalCardsServerMock('localhost', port)
        _g_personal_cards_mock.start()

        yield tornado.gen.sleep(0.2)

        log.info(f'Starting personal cards mock server at port {port} done')

    _g_personal_cards_initialized = True
    _g_personal_cards_future.set_result(True)

    UniproxyCounter.init()
    return True


@alice.uniproxy.library.testing.ioloop_run
def test_personal_cards():
    yield wait_for_mock()
    pdata = PersonalCardsHelper('123', null_logger())
    payload = {
        'card': {
            'card_id': 'station_billing_12345',
            'button_url': 'https://yandex.ru/quasar/id/kinopoisk/promoperiod',
            'text': 'Активировать Яндекс.Плюс',
            'date_from': 1596398659,
            'date_to': 1596405859,
            'yandex.station_film': {
                'min_price': 0
            }
        },
        'remove_existing_cards': True
    }
    res = yield pdata.send_personal_card(payload)
    assert res
    assert GlobalCounter.PERSONAL_CARDS_REQUEST_ADD_OK_SUMM.value() == 1
    assert GlobalCounter.PERSONAL_CARDS_REQUEST_DISMISS_OK_SUMM.value() == 1
