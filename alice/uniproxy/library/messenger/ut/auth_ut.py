import logging
import tornado.gen

import alice.uniproxy.library.testing
from alice.uniproxy.library.global_counter import GlobalCounter

from alice.uniproxy.library.messenger.auth import FanoutAuth, YambAuth, MessengerAuthError
from .mock_auth import wait_for_fanout_mock, wait_for_yamb_mock


logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')

GlobalCounter.init()


# ====================================================================================================================
class UnisystemMock(object):
    def __init__(self):
        super().__init__()
        self._logger = logging.getLogger('mock.unisystem')
        self.session_id = '1d2c0237-8c2c-4d6e-9d27-e82558da2bab'
        self.client_ip = '127.0.0.1'
        self.guid = None
        self.mssngr_auth_error = None

    def uuid(self):
        return '326fa655-9190-4206-9d86-ccd0ea438a12'

    def session_data(self):
        return {
            'uuid': self.uuid(),
        }

    def write_directive(self, directive):
        self._last_directive = directive.create_message(self)

    def write_message(self, message):
        self._last_message = message

    def on_close_event_processor(self, *args, **kwargs):
        pass

    def next_message_id(self):
        return 42

    @tornado.gen.coroutine
    def update_messenger_guid(self, token=None, token_type=None, cookie=None, origin='', fanout_auth=False):
        if fanout_auth:
            self._logger.info('will use fanout auth')
            client = FanoutAuth(timeout=1.0)
        else:
            self._logger.info('will use yamb auth')
            client = YambAuth(timeout=1.0)

        y_session_id = None
        y_yamb_session_id = None
        icookie = None
        if cookie:
            parsed = tornado.httputil.parse_cookie(cookie)
            y_session_id = parsed.get('Session_id')
            y_yamb_session_id = parsed.get('yamb_session_id')
            icookie = parsed.get('i')

        try:
            self.guid = yield client.auth_user(
                token=token,
                token_type=token_type,
                y_cookie=y_session_id,
                y_yamb_cookie=y_yamb_session_id,
                icookie=icookie,
                ip='127.0.0.1',
                port='3333',
                session_id=self.session_id,
                origin=origin,
            )
        except MessengerAuthError as ex:
            self._logger.exception(ex)
            self.mssngr_auth_error = ex.message
        except Exception as ex:
            self._logger.exception(ex)


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock():
    yield wait_for_yamb_mock()


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock():
    yield wait_for_fanout_mock()


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_oauth_valid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('oauth-valid-token', 'Oauth')

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'public-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_oauth_invalid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('oauth-error-token', 'Oauth')

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_oauthteam_valid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('oauth-valid-token', 'OauthTeam')

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'team-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_oauthteam_invalid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('oauth-error-token', 'OauthTeam')

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yambauth_valid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('yambauth-valid-token', 'YambAuth')

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'yamb-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yambauth_invalid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('yambauth-error-token', 'YambAuth')

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_session_id_valid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    # (token, token_type=None, cookie=None, origin='', fanout_auth=False):
    yield unisystem.update_messenger_guid(cookie='yandexuid=123; Session_id=valid-session-id', origin='https://yandex.ru')

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'public-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_session_id_invalid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    # (token, token_type=None, cookie=None, origin='', fanout_auth=False):
    yield unisystem.update_messenger_guid(cookie='yandexuid=123; Session_id=error-session-id', origin='https://yandex.ru')

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_session_id_team_valid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(cookie='yandexuid=123; Session_id=valid-session-id', origin='https://yandex-team.ru')

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'team-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_session_id_team_invalid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(cookie='yandexuid=123; Session_id=error-session-id', origin='https://yandex-team.ru')

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yamb_session_id_valid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(cookie='yandexuid=123; yamb_session_id=valid-session-id', origin='https://yandex.ru')

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'yamb-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yamb_session_id_and_i_cookie_valid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(cookie='yandexuid=123; yamb_session_id=valid-session-id; i=valid-i-cookie', origin='https://yandex.ru')

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'yamb-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_i_cookie_only_valid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(cookie='yandexuid=123; i=valid-i-cookie', origin='https://yandex.ru')

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'yamb-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yamb_session_id_invalid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(cookie='yandexuid=123; yamb_session_id=error-session-id; i=error-i-cookie', origin='https://yandex.ru')

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yamb_session_id_invalid_uid():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(cookie='yamb_session_id=error-session-id; i=error-i-cookie', origin='https://yandex.ru')

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_session_id_valid():
    yield wait_for_fanout_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(
        cookie='yandexuid=123; Session_id=valid-session-id',
        origin='https://yandex.ru',
        fanout_auth=True
    )

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'public-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_session_id_invalid():
    yield wait_for_fanout_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(
        cookie='yandexuid=123; Session_id=error-session-id',
        origin='https://yandex.ru',
        fanout_auth=True
    )

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_oauth_valid():
    yield wait_for_fanout_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('oauth-valid-token', 'Oauth', fanout_auth=True)

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'public-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_oauth_invalid():
    yield wait_for_fanout_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('oauth-error-token', 'Oauth', fanout_auth=True)

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_oauthteam_valid():
    yield wait_for_fanout_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('oauth-valid-token', 'OauthTeam', fanout_auth=True)

    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'team-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_fanout_mock_oauthteam_invalid():
    yield wait_for_fanout_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid('oauth-error-token', 'OauthTeam', fanout_auth=True)

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yamb_cookie_fallback_if_4xx():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(
        cookie='yandexuid=123; Session_id=invalid-session-id; yamb_session_id=valid-session-id; i=valid-i-cookie',
        origin='https://yandex.ru',
        fanout_auth=False
    )

    assert unisystem.guid is not None
    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'yamb-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yamb_cookie_fallback_if_expired():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(
        cookie='yandexuid=123; Session_id=expired-session-id; yamb_session_id=valid-session-id; i=valid-i-cookie',
        origin='https://yandex.ru',
        fanout_auth=False
    )

    assert unisystem.guid is not None
    assert unisystem.mssngr_auth_error is None
    assert unisystem.guid == 'yamb-guid'


# --------------------------------------------------------------------------------------------------------------------
@alice.uniproxy.library.testing.ioloop_run
def test_yamb_mock_yamb_cookie_no_fallback_if_expired_for_yateam():
    yield wait_for_yamb_mock()
    unisystem = UnisystemMock()

    yield unisystem.update_messenger_guid(
        cookie='yandexuid=123; Session_id=expired-session-id; yamb_session_id=valid-session-id; i=valid-i-cookie',
        origin='https://q.yandex-team.ru',
        fanout_auth=False
    )

    assert unisystem.guid is None
    assert unisystem.mssngr_auth_error is not None

# --------------------------------------------------------------------------------------------------------------------
# @pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
# @alice.uniproxy.library.testing.ioloop_run
# def test_synchronize_state_response_on_yamb_auth_failure():
#     yield wait_for_yamb_mock()
#
#     websocket = WebsocketMock()
#     websocket.send_event({
#         "header": {
#             "name": "SynchronizeState",
#             "namespace": "System"
#         }, "payload": {
#             "Messenger": {"version": 3},
#             "oauth_token": "yambauth-error-token",
#             "request": {"experiments": {"disregard_uaas": True}}  # to disable UaaS
#         }
#     })
#
#     response = yield websocket.pop_directive()
#     assert response["header"]["namespace"] == "Messenger"
#     assert response["header"]["name"] == "SynchronizeStateResponse"
#     assert response["payload"]["guid"] == None
#     assert response["payload"]["versions"] == {"current": 2, "minimal": 2}
#     assert response["payload"].get("error") == {
#         "scope": "yamb",
#         "code": "403",
#         "text": "invalid-token"
#     }
#
#
# # --------------------------------------------------------------------------------------------------------------------
# @pytest.mark.usefixtures("tvm_client_mock", "subway_client_mock", "no_lass")
# @alice.uniproxy.library.testing.ioloop_run
# def test_synchronize_state_response_on_blackbox_failure():
#     yield wait_for_fanout_mock()
#
#     websocket = WebsocketMock()
#     websocket.send_event({
#         "header": {
#             "name": "SynchronizeState",
#             "namespace": "System"
#         }, "payload": {
#             "Messenger": {"version": 3, "fanout_auth": True},
#             "oauth_token": "oauth-error-token",
#             "request": {"experiments": {"disregard_uaas": True}}  # to disable UaaS
#         }
#     })
#
#     response = yield websocket.pop_directive()
#     assert response["header"]["namespace"] == "Messenger"
#     assert response["header"]["name"] == "SynchronizeStateResponse"
#     assert response["payload"]["guid"] == None
#     assert response["payload"]["versions"] == {"current": 2, "minimal": 2}
#     assert response["payload"].get("error") == {
#         "scope": "blackbox",
#         "code": "INVALID_TOKEN",
#         "text": "INVALID_TOKEN"
#     }
