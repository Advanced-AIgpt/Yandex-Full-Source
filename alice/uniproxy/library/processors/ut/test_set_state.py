from alice.uniproxy.library import testing
from alice.uniproxy.library.global_counter import GlobalCounter
import alice.uniproxy.library.global_state as global_state
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.testing.mocks import FakeUniWebSocket
from alice.uniproxy.library.testing.mocks import reset_mocks
from alice.uniproxy.library.testing.wrappers import reset_wrappers
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserInfo
import json
import base64


def make_session_context():
    context = TSessionContext()
    context.SessionId = "ffffffff-ffff-ffff-ffff-fffffffffffa"
    context.InitialMessageId = "ffffffff-ffff-ffff-ffff-ffffffffffff"
    context.ConnectionInfo.IpAddress = '10.1.2.3'
    context.AppToken = "ffffffff-ffff-ffff-ffff-ffffffffffff"
    context.AppId = "ru.yandex.uniproxy.test"
    context.Language = "ru-RU"
    context.UserInfo.Uuid = "12345678-1234-1234-1234-123456789abc"
    context.UserInfo.Yuid = "123123123"
    context.UserInfo.Guid = "87654321-1234-1234-1234-123456789abc"
    context.UserInfo.Puid = "123000321"
    context.UserInfo.AuthToken = "THIS-IS-TOKEN"
    context.UserInfo.AuthTokenType = TUserInfo.ETokenType.OAUTH
    context.UserInfo.Cookie = "yandexuid=123123123"
    context.UserInfo.ICookie = "333222111"
    context.UserInfo.StaffLogin = "robot-voicetechbugs"
    context.UserInfo.LaasRegion = json.dumps({
        "region_id": 104357,
        "precision": 3,
        "latitude": 60.166892,
        "longitude": 24.943592,
        "should_update_cookie": False,
        "is_user_choice": False,
        "suspected_region_id": 104357,
        "city_id": 10493,
        "region_by_ip": 104357,
        "suspected_region_city": 10493,
        "location_accuracy": 15000,
        "location_unixtime": 1600334750,
        "suspected_latitude": 60.166892,
        "suspected_longitude": 24.943592,
        "suspected_location_accuracy": 15000,
        "suspected_location_unixtime": 1600334750,
        "suspected_precision": 3,
        "region_home": 0,
        "probable_regions_reliability": 1.00,
        "probable_regions": [],
        "country_id_by_ip": 123,
        "is_anonymous_vpn": False,
        "is_public_proxy": False,
        "is_serp_trusted_net": False,
        "is_tor": False,
        "is_hosting": False,
        "is_gdpr": True,
        "is_mobile": False,
        "is_yandex_net": True,
        "is_yandex_staff": True
    })

    context.UserOptions.SaveToMds = False
    context.UserOptions.DisableLocalExperiments = False
    context.UserOptions.DisableUtteranceLogging = False
    context.UserOptions.DoNotUseLogs = True

    return base64.b64encode(context.SerializeToString()).decode('ascii')


@testing.ioloop_run
def ioloop_test_set_state():
    reset_mocks()
    reset_wrappers()

    web_socket = FakeUniWebSocket()
    system = web_socket.system
    system.process_json_message(json.dumps({
        "event": {
            "header": {
                "namespace": "System",
                "name": "SetState",
                "messageId": "ffffffff-ffff-ffff-ffff-ffffffffffff",
            },
            "payload": {
                "session_context": make_session_context(),
                "original_payload": {
                    "uniproxy2": {},
                    "settings_from_manager": {
                    }
                }
            }
        }
    }))

    assert system.yuid == "123123123"
    assert system.uuid() == "12345678-1234-1234-1234-123456789abc"
    assert system.guid == "87654321-1234-1234-1234-123456789abc"
    assert system.puid == "123000321"
    assert system.uid == system.puid
    assert system.uid != system.yuid
    assert system.staff_login == "robot-voicetechbugs"

    assert 'auth_token' in system.session_data
    assert system.session_data['auth_token'] == 'ffffffff-ffff-ffff-ffff-ffffffffffff'

    assert 'key' in system.session_data
    assert system.session_data['key'] == 'ffffffff-ffff-ffff-ffff-ffffffffffff'

    assert system.oauth_token == 'THIS-IS-TOKEN'

    assert 'request' in system.session_data

    assert 'laas_region' in system.session_data['request']
    assert isinstance(system.session_data['request']['laas_region'], dict)
    assert 'region_id' in system.session_data['request']['laas_region']

    assert system.icookie_for_uaas == "333222111"

    assert system.session_ready()


def test_set_state(monkeypatch):
    global_state.GlobalState.init()
    GlobalCounter.init()
    Logger.init("uniproxy", is_debug=True)

    ioloop_test_set_state()
