from alice.uniproxy.library.uaas import FlagsJsonClient

import json


class FakeHTTPClient2:
    def __init__(self, rsp):
        self.rsp = rsp
        self.request = None

    def fetch(self, request, callback):
        self.request = request

        class FakeHttpResponse:
            def __init__(self, body):
                self.body = body
                self.code = 200
                self.request = request
                self.headers = {}
                self.error = None

        callback(FakeHttpResponse(json.dumps(self.rsp).encode('utf-8')))

    def close(self):
        pass


class FakeFlagsJsonClient(FlagsJsonClient):
    CLIENTS = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        FakeFlagsJsonClient.CLIENTS.append(self)

    def create_client(self, url):
        self.client = FakeHTTPClient2({
            "flags_json_version": "1417",
            "all": {
                "CONTEXT": {
                    "MAIN": {
                        "VOICE": {
                            "flags": [
                                "disable_biometry_scoring",
                            ]
                        }
                    }
                },
                "TESTID": [
                    "2808",
                    "2207"
                ]
            },
            "reqid": "1624551755958756-1289704714697201917800113-production-app-host-sas-faas-10",
            "exphandler": "uniproxy",
            "exp_config_version": "16127",
            "is_prestable": 1,
            "version_token": "blabla",
            "ids": [
                "2808",
                "2207",
            ],
            "exp_boxes": "2808,0,8;2207,0,7",
        })
        self.fake_client = self.client
