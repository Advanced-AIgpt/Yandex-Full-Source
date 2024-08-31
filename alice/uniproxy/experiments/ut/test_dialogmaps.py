from copy import deepcopy
import json
import pytest

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.experiments import Experiments
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.testing.checks import match
from library.python import resource


class FakeUniSystem:
    def __init__(self):
        self.patcher = None
        self.uaas_flags = {}
        self.uaas_test_ids = []
        self.session_data = {'uuid': '123'}
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
            "request": {
                "additional_options": {
                },
                "event": {
                    "name": "",
                    "type": "voice_input"
                },
                "reset_session": True,
                "voice_session": True
            },
            "topic": "dialog-maps"
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


@pytest.mark.parametrize("topic,lang,expected", [
    ("dialogmaps", "tr-TR", {
        "topic": "dialogmapsgpu",
        "advancedASROptions": {
            "enable_e2e_eou": True,
        },
        "request": {
            "experiments": {
                "vins_e2e_partials": "1"
            }
        }
    })
])
def test_rtc_production(topic, lang, expected):
    Logger.init("unittest", True)

    event = Event({
        "header": {
            "name": "SynchronizeState",
            "namespace": "System",
            "messageId": "azaza",
        },
        "payload": deepcopy(EVENT.payload)
    })

    us = FakeUniSystem()
    us.session_data = deepcopy(SESSION_DATA)

    if topic is not None:
        event.payload["topic"] = topic
    if lang is not None:
        event.payload["lang"] = lang

    experiments.try_use_experiment(us)
    us.patch(event)

    match(event.payload, expected)
