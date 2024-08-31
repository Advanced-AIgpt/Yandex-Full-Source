from copy import deepcopy
import json
import pytest

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.experiments import Experiments
from alice.uniproxy.library.logging import Logger
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
                "voice_session": True
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

QUASAR_KEY = "51ae06cc-5c8f-48dc-93ae-7214517679e6"
QUASAR_APP_ID = ["aliced", "ru.yandex.quasar.app", "ru.yandex.quasar.services", "ru.yandex.iosdk.elariwatch"]

# ---------------------- Demo sample
DEMO_EXPECTED = [
    "market",
    "market_beru_disable",
    "market_orders_status_disable",
]

DEMO_NOT_EXPECTED = [
    "market_disable",
    "market_beru",
    "market_orders_status",
    "use_yandexstation_instead_of_station",
    "disable_biometry_scoring",
]

# ----------------------- ANDROID BROWSER
ANDROID_BRO_PROD_EXPECTED = [
    "market_disable",
    "market_beru",
]

ANDROID_BRO_PROD_NOT_EXPECTED = [
    "market",
    "market_beru_disable",
    "market_mds_phrases",
    "use_yandexstation_instead_of_station",
    "disable_biometry_scoring",
]

ANDROID_BRO_DEV_EXPECTED = ANDROID_BRO_PROD_EXPECTED + [
]

ANDROID_BRO_DEV_NOT_EXPECTED = ANDROID_BRO_PROD_NOT_EXPECTED + [
]


# ----------------------- IOS BROWSER
IOS_BRO_PROD_EXPECTED = [
    "market_disable",
    "market_beru",
]

IOS_BRO_PROD_NOT_EXPECTED = [
    "market",
    "market_beru_disable",
    "market_mds_phrases",
    "use_yandexstation_instead_of_station",
    "disable_biometry_scoring",
]

IOS_BRO_DEV_EXPECTED = IOS_BRO_PROD_EXPECTED + [
]

IOS_BRO_DEV_NOT_EXPECTED = IOS_BRO_PROD_NOT_EXPECTED + [
]


# ----------------------- ANDROID PP
ANDROID_PP_PROD_EXPECTED = [
    "market",
    "market_beru",
    "market_mds_phrases",
]

ANDROID_PP_PROD_NOT_EXPECTED = [
    "market_disable",
    "market_beru_disable",
    "use_yandexstation_instead_of_station",
    "disable_biometry_scoring",
]

ANDROID_PP_BETA_EXPECTED = ANDROID_PP_PROD_EXPECTED
ANDROID_PP_BETA_NOT_EXPECTED = ANDROID_PP_PROD_NOT_EXPECTED

ANDROID_PP_DEV_EXPECTED = [e for e in ANDROID_PP_BETA_EXPECTED if e not in ['market']]
ANDROID_PP_DEV_NOT_EXPECTED = [e for e in ANDROID_PP_BETA_NOT_EXPECTED if e not in ['market_disable']]


# ----------------------- IOS PP
IOS_PP_PROD_EXPECTED = [
    "market",
    "market_beru",
    "market_mds_phrases",
]

IOS_PP_PROD_NOT_EXPECTED = [
    "market_disable",
    "market_beru_disable",
    "use_yandexstation_instead_of_station",
    "disable_biometry_scoring",
]

IOS_PP_DEV_EXPECTED = [e for e in IOS_PP_PROD_EXPECTED if e not in ['market']]

IOS_PP_DEV_NOT_EXPECTED = [e for e in IOS_PP_PROD_NOT_EXPECTED if e not in ['market_disable']]


# ----------------------- QUASAR
QUASAR_PROD_EXPECTED = [
    "market_disable",
    "market_beru_disable",
    "market_orders_status_disable",
]

QUASAR_PROD_NOT_EXPECTED = [
    "market",
    "market_beru",
    "market_orders_status",
    "market_mds_phrases",
    "use_yandexstation_instead_of_station",
    "disable_biometry_scoring",
]

# ---------------------- ELARI
# the main experiments from QUASAR (see below)
ELARI_SPECIAL_EXPECTED = [
    "how_much_disable",
    "recurring_purchase_disable",
]

ELARI_SPECIAL_NOT_EXPECTED = [
    "how_much",
    "recurring_purchase",
    "market_mds_phrases"
]

ELARI_EXPECTED = [e for e in QUASAR_PROD_EXPECTED if e not in ELARI_SPECIAL_NOT_EXPECTED] + ELARI_SPECIAL_EXPECTED
ELARI_NOT_EXPECTED = [e for e in QUASAR_PROD_NOT_EXPECTED if e not in ELARI_SPECIAL_EXPECTED] + ELARI_SPECIAL_NOT_EXPECTED

# ---------------------- DESKTOP BROWSER
BRO_EXPECTED = [
    "market_disable",
    "market_beru",
]

BRO_NOT_EXPECTED = [
    "market",
    "market_beru_disable",
    "how_much_disable",
    "recurring_purchase_disable",
    "market_orders_status_disable",
    "market_mds_phrases",
    "use_yandexstation_instead_of_station",
    "disable_biometry_scoring",
]


@pytest.mark.parametrize("app_id,expected,not_expected", [
    ("com.yandex.browser", ANDROID_BRO_PROD_EXPECTED, ANDROID_BRO_PROD_NOT_EXPECTED),

    ("com.yandex.browser.alpha", ANDROID_BRO_DEV_EXPECTED, ANDROID_BRO_DEV_NOT_EXPECTED),
    ("com.yandex.browser.beta", ANDROID_BRO_DEV_EXPECTED, ANDROID_BRO_DEV_NOT_EXPECTED),
    ("com.yandex.browser.inhouse", ANDROID_BRO_DEV_EXPECTED, ANDROID_BRO_DEV_NOT_EXPECTED),
    ("com.yandex.browser.dev", ANDROID_BRO_DEV_EXPECTED, ANDROID_BRO_DEV_NOT_EXPECTED),
    ("com.yandex.browser.canary", ANDROID_BRO_DEV_EXPECTED, ANDROID_BRO_DEV_NOT_EXPECTED),
    ("com.yandex.browser.broteam", ANDROID_BRO_DEV_EXPECTED, ANDROID_BRO_DEV_NOT_EXPECTED),

    ("ru.yandex.mobile.search", IOS_BRO_PROD_EXPECTED, IOS_BRO_PROD_NOT_EXPECTED),
    ("ru.yandex.mobile.search.ipad", IOS_BRO_PROD_EXPECTED, IOS_BRO_PROD_NOT_EXPECTED),

    ("ru.yandex.mobile.search.dev", IOS_BRO_DEV_EXPECTED, IOS_BRO_DEV_NOT_EXPECTED),
    ("ru.yandex.mobile.search.test", IOS_BRO_DEV_EXPECTED, IOS_BRO_DEV_NOT_EXPECTED),
    ("ru.yandex.mobile.search.inhouse", IOS_BRO_DEV_EXPECTED, IOS_BRO_DEV_NOT_EXPECTED),
    ("ru.yandex.mobile.search.ipad.dev", IOS_BRO_DEV_EXPECTED, IOS_BRO_DEV_NOT_EXPECTED),
    ("ru.yandex.mobile.search.ipad.test", IOS_BRO_DEV_EXPECTED, IOS_BRO_DEV_NOT_EXPECTED),
    ("ru.yandex.mobile.search.ipad.inhouse", IOS_BRO_DEV_EXPECTED, IOS_BRO_DEV_NOT_EXPECTED),

    ("aliced", QUASAR_PROD_EXPECTED, QUASAR_PROD_NOT_EXPECTED),
    ("ru.yandex.quasar.app", QUASAR_PROD_EXPECTED, QUASAR_PROD_NOT_EXPECTED),
    ("ru.yandex.quasar.services", QUASAR_PROD_EXPECTED, QUASAR_PROD_NOT_EXPECTED),

    ("ru.yandex.iosdk.elariwatch", ELARI_EXPECTED, ELARI_NOT_EXPECTED),

    ("ru.yandex.searchplugin", ANDROID_PP_PROD_EXPECTED, ANDROID_PP_PROD_NOT_EXPECTED),
    ("ru.yandex.searchplugin.beta", ANDROID_PP_BETA_EXPECTED, ANDROID_PP_BETA_NOT_EXPECTED),
    ("ru.yandex.searchplugin.dev", ANDROID_PP_DEV_EXPECTED, ANDROID_PP_DEV_NOT_EXPECTED),
    ("ru.yandex.searchplugin.nightly", ANDROID_PP_DEV_EXPECTED, ANDROID_PP_DEV_NOT_EXPECTED),

    ("ru.yandex.mobile", IOS_PP_PROD_EXPECTED, IOS_PP_PROD_NOT_EXPECTED),
    ("ru.yandex.mobile.dev", IOS_PP_DEV_EXPECTED, IOS_PP_DEV_NOT_EXPECTED),
    ("ru.yandex.mobile.inhouse", IOS_PP_DEV_EXPECTED, IOS_PP_DEV_NOT_EXPECTED),

    # desktop browser
    ("winsearchbar",  BRO_EXPECTED, BRO_NOT_EXPECTED),
    ("YaBro",         BRO_EXPECTED, BRO_NOT_EXPECTED),
    ("YaBro.dev",     BRO_EXPECTED, BRO_NOT_EXPECTED),
    ("YaBro.canary",  BRO_EXPECTED, BRO_NOT_EXPECTED),
    ("YaBro.broteam", BRO_EXPECTED, BRO_NOT_EXPECTED),
    ("YaBro.beta",    BRO_EXPECTED, BRO_NOT_EXPECTED),

    ("com.yandex.alicekit.demo", DEMO_EXPECTED, DEMO_NOT_EXPECTED),
    ("ru.yandex.mobile.alice.inhouse", DEMO_EXPECTED, DEMO_NOT_EXPECTED),

    (
        "com.yandex.launcher",
        [
            "market_disable",
        ],
        [
            "market",
        ]
    ),
])
def test_rtc_production(app_id, expected, not_expected):
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

    # patch session data
    us.session_data["vins"]["application"]["app_id"] = app_id
    if app_id in QUASAR_APP_ID:
        us.session_data["key"] = QUASAR_KEY

    experiments.try_use_experiment(us)
    us.patch(event)

    for e in expected:
        assert e in event.payload['request']['experiments']

    for e in not_expected:
        assert e not in event.payload['request']['experiments']
