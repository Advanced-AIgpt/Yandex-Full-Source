from copy import deepcopy

import pytest

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.event_patcher import EventPatcher
from alice.uniproxy.library.logging import Logger
from library.python import resource


def test_event_patcher():
    Logger.init("unittest", True)

    macros = {
        "staff": [
            "enable_reminders_todos",
            "market"
        ],
        "beta": [
            "find_poi_gallery",
            "translate",
            "ambient_sound",
            "external_skills_discovery",
            "external_skills_web_discovery",
            "external_skills_web_discovery_use_relevance_boundary",
            "market",
            "skill_recommendation_experiment",
            "skill_recommendation_service_experiment",
            "phone_call_contact",
            "image_recognizer_clothes",
            "personalization",
            "username_auto_insert",
            "market_beru",
            "show_route_gallery"
        ],
        "test-all": [
        ],
        "prod-all": [
            "enable_reminders_todos",
            "enable_ner_for_skills",
            "taxi_nlu",
            "taxi",
            "music_sing_song",
            "enable_timers_alarms",
            "enable_timers",
            "enable_alarms",
            "how_much",
            "games_onboarding_monsters_on_vacation",
            "music_use_websearch",
            "authorized_personal_playlists"
        ],
        "quasar": [
            "taxi",
            "taxi_nlu",
            "music_sing_song",
            "music_recognizer",
            "enable_timers_alarms",
            "debug_mode",
            "debug_mode_old",
            "enable_reminders_todos",
            "tts_domain_music",
            "music_show_first_track",
            "music_use_websearch",
            "how_much",
            "new_special_playlists"
        ]
    }

    event_patcher = EventPatcher([[
        "import_macro", ".request.experiments", "prod-all"
    ], [
        "if_session_data_in", ".vins.application.app_id",
        [
            "ru.yandex.searchplugin.dev",
            "ru.yandex.searchplugin.beta",
            "ru.yandex.searchplugin.nightly",
            "com.yandex.browser.alpha",
            "com.yandex.browser.beta",
            "com.yandex.browser.canary",
            "com.yandex.browser.broteam"
        ],
        "import_macro", ".request.experiments", "beta"
    ], [
        "if_session_data_eq", ".vins.application.app_id", "ru.yandex.searchplugin.beta",
        "set", ".abc", True,
        "if_session_data_ne", ".vins.application.app_id", "ru.yandex.searchplugin.beta",
        "set", ".abc", False,
    ]], macros
    )

    event1 = Event(
        {
            "header": {
                "messageId": "d2b24901-86b0-4f73-85d9-f4370f10c498",
                "name": "VoiceInput",
                "namespace": "Vins",
                "streamId": 1
            },
            "payload": {
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
    assert(event_patcher.useful())
    session_data = {
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
    event = Event({
        "header": {
            "name": "SynchronizeState",
            "namespace": "System",
            "messageId": "azaza",
        },
        "payload": deepcopy(event1.payload)
    })
    session_data2 = deepcopy(session_data)

    assert("abc" not in event.payload)
    event_patcher.patch(event, session_data, staff_login='vasya')
    for e in macros['prod-all']:
        assert e in event.payload['request']['experiments']
    assert(event.payload["abc"] is True)

    # assert(event.payload["request"]["experiments"] is None)
    session_data2["vins"]["application"]["app_id"] = "ru.yandex.searchplugin"

    event_patcher.patch(event1, session_data2, staff_login='vasya')
    event_patcher.patch(event1, session_data2, staff_login='vasya')
    assert(len(macros["prod-all"]) == 12)


@pytest.mark.parametrize("pfile", ["experiments_rtc_production.json", "experiments_ycloud.json"])
def test_real_experiments(pfile):
    import json

    macro = json.loads(resource.find('/vins_experiments.json'))
    full_exps = json.loads(resource.find('/' + pfile))

    exp_list = [f for e in full_exps for f in e['flags']]

    class FakePatcher(EventPatcher):
        def __init__(self, *args):
            super().__init__(*args)

    def init():
        import inspect
        for name, method in inspect.getmembers(EventPatcher, predicate=inspect.isfunction):
            if name.startswith('cmd_'):
                def _f(method):
                    def _func(self, event, args):
                        print("    ", method)
                        r = method(self, event, args)
                        if r is False:
                            src = inspect.getsource(method)
                            x = int(src[src.find("@check_args_num") + len("@check_args_num(")])
                            self.apply_flag(event, args[x:])
                        return True
                    return _func
                setattr(FakePatcher, name, _f(method))

    init()
    event_patcher = FakePatcher(exp_list, macro)

    assert(event_patcher.useful())
    session_data = {
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
    event = Event({
        "header": {
            "name": "SynchronizeState",
            "namespace": "System",
            "messageId": "azaza",
        },
        "payload": {}
    })

    event_patcher.patch(event, session_data, staff_login='vasya', hide_exceptions=False)
