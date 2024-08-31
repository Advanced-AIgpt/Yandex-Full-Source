from alice.cuttlefish.library.python.test_utils import deepupdate
import json
import urllib
import logging


class Blackbox:
    logger = logging.getLogger("mock.blackbox")
    backend_name = "BLACKBOX__VOICE"

    @classmethod
    def _standard_content(cls):
        return {
            "oauth": {
                "uid": "00000000",
                "token_id": "0000000000",
                "device_id": "FFFFFFFFFFFFFFFFFFFFFFFF",
                "device_name": "какое-то имя девайса",
                "scope": "login:email login:info music:read music:write yataxi:write yamb:all quasar:all quasar:pay",
                "ctime": "2020-01-01 00:00:00",
                "issue_time": "2020-01-01 00:00:00",
                "expire_time": "3020-01-01 00:00:00",
                "is_ttl_refreshable": True,
                "client_id": "00000000000000000000000000000000",
                "client_name": "какое-то имя клиента",
                "client_icon": "https://avatars.mds.yandex.net/get-oauth/path/to/some/icon",
                "client_homepage": "",
                "client_ctime": "2020-01-01 00:00:00",
                "client_is_yandex": False,
                "xtoken_id": "0000000000",
                "meta": "",
            },
            "uid": {"value": "00000000", "lite": False, "hosted": False},
            "login": "some-login",
            "have_password": True,
            "have_hint": True,
            "karma": {"value": 0},
            "karma_status": {"value": 6000},
            "user_ticket": "3:user:VALID-USER-TICKET",
            "status": {"value": "VALID", "id": 0},
            "error": "OK",
            "connection_id": "t:0000000000",
        }

    @classmethod
    def standard_response(cls):
        return {
            "code": 200,
            "reason": "OK",
            "headers": [["Server", "nginx"], ["Content-Type", "application/json"]],
            "body": json.dumps(cls._standard_content()),
        }

    @classmethod
    def response_for_staff(cls):
        content = cls._standard_content()
        deepupdate(
            content, {"client_is_yandex": True, "login": "some-yandex-login", "aliases": {"13": "some-yandex-login"}}
        )
        return {
            "code": 200,
            "reason": "OK",
            "headers": [["Server", "nginx"], ["Content-Type", "application/json"]],
            "body": json.dumps(content),
        }

    @classmethod
    def auto(cls, name, request):
        uri_parsed = urllib.parse.urlparse(request["uri"])
        args = urllib.parse.parse_qs(uri_parsed.query)

        oauth_token = args["oauth_token"][0]
        cls.logger.debug(f"Got request with OAuth token: {oauth_token}")

        if "STAFF" in oauth_token:
            return cls.response_for_staff()

        return cls.standard_response()
