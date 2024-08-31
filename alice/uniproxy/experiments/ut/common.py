import json
import functools
from copy import deepcopy

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.experiments import Experiments
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.utils import deepupdate
from alice.uniproxy.library.testing.checks import match

from library.python import resource


Logger.init("unittest", True)


class FakeUniSystem:
    def __init__(self, session_data=None):
        self.patcher = None
        self.uaas_flags = {}
        self.uaas_test_ids = []
        self.session_data = session_data or {'uuid': '123'}
        self.exps_check = False

    def set_event_patcher(self, patcher):
        self.patcher = patcher

    def log_experiment(self, *args, **kwargs):
        pass

    def patch(self, event):
        self.patcher.patch(event, self.session_data)


_macro_list = json.loads(resource.find("/vins_experiments.json"))
_experiments_list = json.loads(resource.find("/experiments_rtc_production.json"))
experiments = Experiments(_experiments_list, _macro_list, mutable_shares=True)


def with_shares(shares):
    def decorator(func):
        @functools.wraps(func)
        def wrap(*args, **kwargs):
            orig_shares = {}
            for exp_id, share in shares.items():
                orig_shares[exp_id] = experiments.get_share(exp_id)
                experiments.set_share(exp_id, share)

            try:
                return func(*args, **kwargs)
            finally:
                for exp_id, share in orig_shares.items():
                    experiments.set_share(exp_id, share)

        return wrap

    return decorator


EVENT = Event(
    {
        "header": {
            "messageId": "d2b24901-86b0-4f73-85d9-f4370f10c498",
            "name": "VoiceInput",
            "namespace": "Vins",
            "streamId": 1
        },
        "payload": {
            "key": "some_not_quasar_key",
            "advancedASROptions": {
                "manual_punctuation": False,
                "partial_results": True
            },
            "application": {
                "client_time": "20181011T195042",
                "device_id": "7bf3b1d736faed2463b6bae2e602fc93",
                "lang": "ru-RU",
                "timestamp": "1539276642",
                "timezone": "EuropegMoscow"
            },
            "disableAntimatNormalizer": True,
            "format": "audio/opus",
            "header": {
                "prev_req_id": "443793fb-b187-4eac-973b-216a2b1e7a37",
                "request_id": "5a233eb2-df1e-46fc-a347-7d60740d9920",
                "sequence_number": 164
            },
            "lang": "ru-RU",
            "punctuation": True,
            "request": {
                "additional_options": {
                },
                "event": {
                    "name": "",
                    "type": "voice_input"
                },
                "location": {
                    "accuracy": 23.63800048828125,
                    "lat": 55.7344229,
                    "lon": 37.5870865,
                    "recency": 478272
                },
                "reset_session": True,
                "voice_session": True,
            },
            "tags": (
                "PASS_AUDIO;exp_42248,0,3;exp_97850,0,79;exp_95265,0,33;exp_29057,0,96;exp_93893,0,97;"
                "exp_92848,0,7;exp_96668,0,31;exp_96585,0,48;exp_97140,0,56;exp_99145,0,97;exp_99155,0,90;"
                "exp_98427,0,51;exp_92994,0,80;exp_96081,0,81;exp_89003,0,71;exp_95822,0,70;exp_96066,0,35;"
                "exp_96476,0,93;exp_97473,0,55;exp_98800,0,53;exp_98305,0,91"),
            "topic": "dialog-general"
        }
    }
)


SESSION_DATA = {
    "accept_invalid_auth": True,
    "auth_token": "cc96633d-59d4-4724-94bd-f5db2f02ad13",
    "device": "xiaomi Redmi Note 4",
    "device_manufacturer": "xiaomi",
    "device_model": "Redmi Note 4",
    "emotion": "neutral",
    "network_type":
    "WIFI::CONNECTED",
    "oauth_token": "",
    "platform_info": "android",
    "ps_activation_model": "phrase-spotter/ru-RU-activation-alisa-plus-yandex-0.1.8:ru-RU-activation-alisa-plus-yandex-0.1.8",
    "seamlessBufferDurationMs": "1",
    "speechkitVersion": "3.20.3",
    "speed": "1",
    "uuid": "ec3ec1e0575df1be42ecccf2b6cfa152",
    "vins": {
        "application": {
            "app_id": "ru.yandex.searchplugin.beta",
            "app_version": "7.71",
            "device_id": "7bf3b1d736faed2463b6bae2e602fc93",
            "device_manufacturer": "xiaomi",
            "device_model": "Redmi Note 4",
            "os_version": "7.0",
            "platform": "android",
            "uuid": "ec3ec1e0575df1be42ecccf2b6cfa152"
        }
    },
    "voice": "shitova",
    "yandexuid": "1420271241539201426"
}


# -------------------------------------------------------------------------------------------------
def run_test_with_matcher(session_data_patch, event_patch, payload_matcher):
    us = FakeUniSystem(session_data=deepcopy(SESSION_DATA))
    if session_data_patch:
        deepupdate(us.session_data, session_data_patch, copy=False)

    event = deepcopy(EVENT)
    if event_patch:
        deepupdate(event.payload, event_patch, copy=False)

    experiments.try_use_experiment(us)
    us.patch(event)

    assert match(event.payload, payload_matcher)
