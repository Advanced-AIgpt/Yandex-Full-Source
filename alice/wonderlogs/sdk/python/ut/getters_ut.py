import json

import google.protobuf.json_format
import google.protobuf.text_format
import pytest
from alice.library.client.protos.client_info_pb2 import TClientInfoProto
from alice.megamind.protos.analytics.megamind_analytics_info_pb2 import \
    TMegamindAnalyticsInfo
from alice.megamind.protos.common.frame_pb2 import TSemanticFrame
from alice.megamind.protos.speechkit.response_pb2 import \
    TSpeechKitResponseProto

from alice.wonderlogs.sdk.python.getters import (form_changed, get_app,
                                                 get_filters_genre, get_intent,
                                                 get_music_answer_type,
                                                 get_music_genre, get_path,
                                                 get_platform,
                                                 get_product_scenario_name,
                                                 get_slot_value, get_slots,
                                                 get_sound_level, get_version,
                                                 parse_cards, smart_home_user)

MEGAMIND_ANALYTICS_INFO_PROTO = '''
AnalyticsInfo {
    key: "Lolkek"
    value {
        ScenarioAnalyticsInfo {
            Intent: "keklol",
            ProductScenarioName: "lolkek1337"
            Events {
                MusicEvent {
                    AnswerType: Track
                }
            }
            Events {
                MusicEvent {
                    Id: "genre:hardbass"
                    AnswerType: Filters
                }
            }
            Objects {
                FirstTrack {
                    Genre: "phonk"
                }
            }
        }
    }
}
WinnerScenario {
    Name: "Lolkek"
}
'''

MEGAMIND_ANALYTICS_INFO_VINS_WITH_SLOTS_PROTO = '''
AnalyticsInfo {
    key: "Vins"
    value {
        SemanticFrame {
            Slots {
                Name: "lolkek"
            }
        }
    }
}
WinnerScenario {
    Name: "Vins"
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_PROTO = '''
AnalyticsInfo {
    key: "Vins"
    value {
        ScenarioAnalyticsInfo {
            Intent: "keklol1337",
            Objects {
                VinsErrorMeta {
                    Intent: "keklol"
                }
            }
        }
    }
}
WinnerScenario {
    Name: "Vins"
}
'''


MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_WITHOUT_INTENT_PROTO = '''
AnalyticsInfo {
    key: "Vins"
    value {
        ScenarioAnalyticsInfo {
            Intent: "keklol69",
            Objects {
                VinsErrorMeta {
                }
            }
        }
    }
}
WinnerScenario {
    Name: "Vins"
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_WITHOUT_ALL_INTENTS_PROTO = '''
AnalyticsInfo {
    key: "Vins"
    value {
        ScenarioAnalyticsInfo {
            Objects {
                VinsErrorMeta {
                }
            }
        }
    }
}
WinnerScenario {
    Name: "Vins"
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_MEDIA_DEVICE_PROTO = '''
IoTUserInfo {
    Devices {
        AnalyticsType: "devices.types.media_device.tv"
        SkillId: "Q"
    }
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_QUASAR_INFO_PROTO = '''
IoTUserInfo {
    Devices {
        QuasarInfo {
            Platform: "yandexstation"
        }
    }
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_LOLKEK_PROTO = '''
IoTUserInfo {
    Devices {
        AnalyticsType: "lolkek"
        SkillId: "Q"
    }
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_BANNED_ANALYTICS_TYPE_PROTO = '''
IoTUserInfo {
    Devices {
        AnalyticsType: "devices.types.hub"
    }
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_SMART_SPEAKER_PROTO = '''
IoTUserInfo {
    Devices {
        AnalyticsType: "devices.types.smart_speaker.flex"
    }
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_USERS_INFO_MEDIA_DEVICE_PROTO = '''
UsersInfo {
    key: "lolkek"
    value {
        ScenarioUserInfo {
            Properties {
                IotProfile {
                    Devices {
                        Type: "devices.types.media_device.tv"
                    }
                }
            }
        }
    }
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_USERS_INFO_LOLKEK_PROTO = '''
UsersInfo {
    key: "lolkek"
    value {
        ScenarioUserInfo {
            Properties {
                IotProfile {
                    Devices {
                        Type: "lolkek"
                    }
                }
            }
        }
    }
}
'''

MEGAMIND_ANALYTICS_INFO_WITH_USERS_INFO_BANNED_ANALYTICS_TYPE_PROTO = '''
UsersInfo {
    key: "lolkek"
    value {
        ScenarioUserInfo {
            Properties {
                IotProfile {
                    Devices {
                        Type: "devices.types.hub"
                    }
                }
            }
        }
    }
}
'''
MEGAMIND_ANALYTICS_INFO_WITH_USERS_INFO_SMART_SPEAKER_PROTO = '''
UsersInfo {
    key: "lolkek"
    value {
        ScenarioUserInfo {
            Properties {
                IotProfile {
                    Devices {
                        Type: "devices.types.smart_speaker.flex"
                    }
                }
            }
        }
    }
}
'''

MEGAMIND_ANALYTICS_INFO_REAL_JSON = '''
{
    "original_utterance":"жанулька кис кис",
    "analytics_info":{
        "HollywoodMusic":{
            "semantic_frame":{
                "slots":[
                    {
                        "type":"music_result",
                        "name":"answer",
                        "accepted_types":[
                            "music_result"
                        ]
                    }
                ],
                "name":"personal_assistant.scenarios.music_play"
            },
            "version":"vins/stable-159-4@8565965",
            "scenario_analytics_info":{
                "stage_timings":{
                    "timings":{
                        "continue":{
                            "start_timestamp":"1630184400083625"
                        },
                        "run":{
                            "start_timestamp":"1630184399689876"
                        },
                        "Continue":{
                            "source_response_durations":{
                                "HollywoodMusic-Continue":"53311"
                            }
                        }
                    }
                },
                "actions":[
                    {
                        "human_readable":"Первый трек, который включится",
                        "name":"music play",
                        "id":"music_play"
                    }
                ],
                "product_scenario_name":"music",
                "objects":[
                    {
                        "id":"music.first_track_id",
                        "human_readable":"zhanulka, кискис",
                        "first_track":{
                            "duration":"170450",
                            "genre":"local-indie",
                            "id":"79093911"
                        },
                        "name":"first_track_id"
                    }
                ],
                "intent":"personal_assistant.scenarios.music_play",
                "events":[
                    {
                        "timestamp":"1630184399629000",
                        "selected_web_document_event":{
                            "search_type":"music_web_search",
                            "answer_url":"https://music.yandex.ru/artist/8553938/?from=alice&mob=0",
                            "search_request_id":"1630184399706521-1562902640319451161200411-production-app-host-man-web-yp-374",
                            "document_url":"https://music.yandex.ru/artist/8553938",
                            "request_id":"bc8df369-fcd5-4b8f-adb5-5456ac4599a1"
                        }
                    },
                    {
                        "timestamp":"1630184399629000",
                        "request_source_event":{
                            "cgi":{
                                "text":"жанулька кис кис host:music.yandex.ru"
                            },
                            "response_success":true,
                            "response_code":200,
                            "source":"music_web_search",
                            "headers":{
                                "X-Yandex-Alice-Meta-Info":"CidwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLm11c2ljX3BsYXkSfwoUcnUueWFuZGV4LnF1YXNhci5hcHASAzEuMBoBOSIHYW5kcm9pZCokNGRhYTUwMjctZjlkMS04ZTAwLWQ5MWYtMDBhZjM4N2NiNzdiMgA6BXJ1LVJVQg8yMDIxMDgyOFQyMDU5NTVKAFIKMTYzMDE4NDM5NVoAYgZZYW5kZXgigQEKENC20LDQvdGD0LvRjNC60LAKBtC60LjRgQoO0LbQsNC90YPQvdC60LAKDtC20LDQu9GD0L3QutCwChDQtNC20LDQvdGD0L3QutCwChDQtNC20LDQvdGD0LvQutCwEgUKAwABARIFCgMCAQESBQoDAwEBEgUKAwQBARIFCgMFAQE,"
                            }
                        }
                    },
                    {
                        "timestamp":"1630184399629000",
                        "selected_source_event":{
                            "source":"web"
                        }
                    },
                    {
                        "timestamp":"1630184399629000",
                        "music_event":{
                            "uri":"https://music.yandex.ru/artist/8553938/?from=alice&mob=0",
                            "answer_type":"Artist",
                            "id":"8553938"
                        }
                    }
                ]
            },
            "parent_request_id":"bc8df369-fcd5-4b8f-adb5-5456ac4599a1",
            "parent_product_scenario_name":"music",
            "matched_semantic_frames":[
                {
                    "typed_semantic_frame":{
                        "music_play_semantic_frame":{
                            "search_text":{
                                "string_value":"жанулька кис кис"
                            }
                        }
                    },
                    "slots":[
                        {
                            "type":"string",
                            "name":"search_text",
                            "value":"жанулька кис кис",
                            "accepted_types":[
                                "hardcoded_music",
                                "custom.hardcoded_music",
                                "string"
                            ]
                        }
                    ],
                    "name":"personal_assistant.scenarios.music_play"
                }
            ]
        }
    },
    "user_profile":{
        "has_yandex_plus":true,
        "subscriptions":[
            "basic-kinopoisk",
            "basic-music",
            "basic-plus",
            "station-lease-plus"
        ]
    },
    "post_classify_duration":"5939",
    "shown_utterance":"Жанулька кис-кис.",
    "location":{
        "lat":57.739700319999997191,
        "recency":0,
        "lon":28.133806230000001136,
        "speed":0,
        "accuracy":140
    },
    "chosen_utterance":"жанулька кис кис",
    "winner_scenario":{
        "name":"HollywoodMusic"
    },
    "iot_user_info":{
        "devices":[
            {
                "quasar_info":{
                    "platform":"yandexstation_2"
                }
            },
            {
                "quasar_info":{
                    "platform":"yandexmicro"
                }
            },
            {
                "quasar_info":{
                    "platform":"yandexstation"
                }
            }
        ]
    }
}
'''

MEGAMIND_ANALYTICS_INFO_REAL_2_JSON = '''
{
    "modifiers_info":{
        "proactivity":{
            "semantic_frames_info":{
                "semantic_frames":[
                    {
                        "name":"personal_assistant.scenarios.player.continue",
                        "typed_semantic_frame":{
                            "player_continue_semantic_frame":{
                            }
                        }
                    }
                ],
                "source":"Begemot"
            },
            "source":"personal_assistant.scenarios.player_continue"
        }
    },
    "modifiers_analytics_info":{
        "proactivity":{
            "semantic_frames_info":{
                "semantic_frames":[
                    {
                        "name":"personal_assistant.scenarios.player.continue",
                        "typed_semantic_frame":{
                            "player_continue_semantic_frame":{
                            }
                        }
                    }
                ],
                "source":"Begemot"
            },
            "source":"personal_assistant.scenarios.player_continue"
        }
    },
    "location":{
        "speed":0,
        "lat":55.798702239999997232,
        "lon":37.771732329999998967,
        "recency":0,
        "accuracy":140
    },
    "scenario_timings":{
        "SideSpeech":{
            "timings":{
                "run":{
                    "start_timestamp":"1630184399743563"
                }
            }
        },
        "Vins":{
            "timings":{
                "run":{
                    "start_timestamp":"1630184399744412"
                }
            }
        },
        "HollywoodMusic":{
            "timings":{
                "run":{
                    "start_timestamp":"1630184399742942"
                }
            }
        },
        "HardcodedResponse":{
            "timings":{
                "run":{
                    "start_timestamp":"1630184399742273"
                }
            }
        }
    },
    "analytics_info":{
        "Vins":{
            "matched_semantic_frames":[
                {
                    "name":"personal_assistant.scenarios.player.continue",
                    "typed_semantic_frame":{
                        "player_continue_semantic_frame":{
                        }
                    }
                }
            ],
            "version":"vins/stable-159-4@8565965",
            "scenario_analytics_info":{
                "stage_timings":{
                    "timings":{
                        "run":{
                            "start_timestamp":"1630184399744412"
                        }
                    }
                },
                "product_scenario_name":"player_commands",
                "intent":"personal_assistant.scenarios.player_continue"
            },
            "semantic_frame":{
                "name":"personal_assistant.scenarios.player_continue",
                "slots":[
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"null"
                        },
                        "name":"player_type"
                    }
                ]
            }
        }
    },
    "winner_scenario":{
        "name":"Vins"
    },
    "user_profile":{
        "subscriptions":[
            "basic-kinopoisk",
            "basic-music",
            "basic-plus",
            "station-lease-plus"
        ],
        "has_yandex_plus":true
    },
    "iot_user_info":{
        "households":[
            {
                "name":"Мой дом",
                "id":"4725907a-267c-4a6d-81ce-976fae37e8fe"
            }
        ],
        "devices":[
            {
                "name":"Лампа 1",
                "updated":1623868646,
                "analytics_type":"devices.types.light",
                "status_updated":1630183359,
                "analytics_name":"Осветительный прибор",
                "id":"184ec051-57f7-46ba-83c1-522df0dac493",
                "custom_data":"eyJjbG91ZElkIjoxMCwiaXNTcGxpdCI6ZmFsc2UsInJlZ2lvbiI6InJ1c3NpYSIsInR5cGUiOiJ1cm46bWlvdC1zcGVjLXYyOmRldmljZTpsaWdodDowMDAwQTAwMTp5ZWVsaW5rLWNvbG9yNToxIn0=",
                "household_id":"4725907a-267c-4a6d-81ce-976fae37e8fe",
                "device_info":{
                    "manufacturer":"yeelink",
                    "model":"yeelink.light.color5",
                    "sw_version":"2.0.8_0019"
                },
                "icon_url":"http://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.svg",
                "status":"OfflineDeviceState",
                "type":"LightDeviceType",
                "external_name":"Осветительный прибор",
                "external_id":"M1GAxtaW9A0LXNwZWMtdjIVgoAFGA55ZWVsaW5rLWNvbG9AyNRUUGAkyNDg4MDY5NDEVxpQIAA",
                "room_id":"e5a088a7-c65d-4a6a-8265-f5a6136be1a5",
                "created":1623868646,
                "capabilities":[
                    {
                        "type":"OnOffCapabilityType",
                        "on_off_capability_parameters":{
                        },
                        "analytics_type":"devices.capabilities.on_off",
                        "reportable":true,
                        "analytics_name":"включение/выключение",
                        "last_updated":1630182244,
                        "retrievable":true,
                        "on_off_capability_state":{
                            "instance":"on",
                            "value":true
                        }
                    },
                    {
                        "range_capability_state":{
                            "instance":"brightness",
                            "value":50
                        },
                        "type":"RangeCapabilityType",
                        "analytics_type":"devices.capabilities.range",
                        "range_capability_parameters":{
                            "random_access":true,
                            "instance":"brightness",
                            "range":{
                                "max":100,
                                "min":1,
                                "precision":1
                            },
                            "unit":"unit.percent"
                        },
                        "reportable":true,
                        "analytics_name":"изменение яркости",
                        "last_updated":1630182244,
                        "retrievable":true
                    },
                    {
                        "color_setting_capability_state":{
                            "instance":"rgb",
                            "rgb":16727040
                        },
                        "color_setting_capability_parameters":{
                            "temperature_k":{
                                "Max":6500,
                                "analytics_name":"изменение цветовой температуры",
                                "Min":1700
                            },
                            "color_model":{
                                "type":"RgbColorModel",
                                "analytics_name":"изменение цвета"
                            }
                        },
                        "type":"ColorSettingCapabilityType",
                        "analytics_type":"devices.capabilities.color_setting",
                        "reportable":true,
                        "analytics_name":"изменение цвета",
                        "last_updated":1630182244,
                        "retrievable":true
                    }
                ],
                "original_type":"LightDeviceType",
                "skill_id":"ad26f8c2-fc31-4928-a653-d829fda7e6c2"
            },
            {
                "name":"Лампа 2",
                "updated":1623868555,
                "analytics_type":"devices.types.light",
                "status_updated":1630183359,
                "analytics_name":"Осветительный прибор",
                "id":"dbd71118-a0c0-4c20-8a95-3ba2e02a7bd5",
                "custom_data":"eyJjbG91ZElkIjoxMCwiaXNTcGxpdCI6ZmFsc2UsInJlZ2lvbiI6InJ1c3NpYSIsInR5cGUiOiJ1cm46bWlvdC1zcGVjLXYyOmRldmljZTpsaWdodDowMDAwQTAwMTp5ZWVsaW5rLWNvbG9yNToxIn0=",
                "household_id":"4725907a-267c-4a6d-81ce-976fae37e8fe",
                "device_info":{
                    "manufacturer":"yeelink",
                    "model":"yeelink.light.color5",
                    "sw_version":"2.0.8_0019"
                },
                "icon_url":"http://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.svg",
                "status":"OfflineDeviceState",
                "type":"LightDeviceType",
                "external_name":"Осветительный прибор",
                "external_id":"M1GAxtaW9A0LXNwZWMtdjIVgoAFGA55ZWVsaW5rLWNvbG9AyNRUUGAkyNDg3OTU2NjcVxpQIAA",
                "room_id":"e5a088a7-c65d-4a6a-8265-f5a6136be1a5",
                "created":1623868555,
                "capabilities":[
                    {
                        "type":"OnOffCapabilityType",
                        "on_off_capability_parameters":{
                        },
                        "analytics_type":"devices.capabilities.on_off",
                        "reportable":true,
                        "analytics_name":"включение/выключение",
                        "last_updated":1630148130,
                        "retrievable":true,
                        "on_off_capability_state":{
                            "instance":"on"
                        }
                    },
                    {
                        "range_capability_state":{
                            "instance":"brightness",
                            "value":50
                        },
                        "type":"RangeCapabilityType",
                        "analytics_type":"devices.capabilities.range",
                        "range_capability_parameters":{
                            "random_access":true,
                            "instance":"brightness",
                            "range":{
                                "max":100,
                                "min":1,
                                "precision":1
                            },
                            "unit":"unit.percent"
                        },
                        "reportable":true,
                        "analytics_name":"изменение яркости",
                        "last_updated":1630182244,
                        "retrievable":true
                    },
                    {
                        "color_setting_capability_state":{
                            "instance":"rgb",
                            "rgb":16727040
                        },
                        "color_setting_capability_parameters":{
                            "temperature_k":{
                                "Max":6500,
                                "analytics_name":"изменение цветовой температуры",
                                "Min":1700
                            },
                            "color_model":{
                                "type":"RgbColorModel",
                                "analytics_name":"изменение цвета"
                            }
                        },
                        "type":"ColorSettingCapabilityType",
                        "analytics_type":"devices.capabilities.color_setting",
                        "reportable":true,
                        "analytics_name":"изменение цвета",
                        "last_updated":1630182242,
                        "retrievable":true
                    }
                ],
                "original_type":"LightDeviceType",
                "skill_id":"ad26f8c2-fc31-4928-a653-d829fda7e6c2"
            },
            {
                "name":"Яндекс Станция",
                "updated":1601839134,
                "quasar_info":{
                    "device_id":"543078968308301509d0",
                    "platform":"yandexstation"
                },
                "analytics_type":"devices.types.smart_speaker.yandex.station",
                "status_updated":1623869390,
                "analytics_name":"Умное устройство",
                "id":"f448eac7-995f-485c-ae6f-dff890bdca0c",
                "custom_data":"eyJkZXZpY2VfaWQiOiI1NDMwNzg5NjgzMDgzMDE1MDlkMCIsInBsYXRmb3JtIjoieWFuZGV4c3RhdGlvbiJ9",
                "household_id":"4725907a-267c-4a6d-81ce-976fae37e8fe",
                "device_info":{
                    "manufacturer":"Yandex Services AG",
                    "model":"YNDX-0001"
                },
                "icon_url":"http://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.png",
                "status":"OnlineDeviceState",
                "type":"YandexStationDeviceType",
                "external_name":"Яндекс Станция",
                "external_id":"543078968308301509d0.yandexstation",
                "created":1601839132,
                "original_type":"YandexStationDeviceType",
                "skill_id":"Q"
            }
        ],
        "colors":[
            {
                "name":"Бирюзовый",
                "id":"turquoise"
            },
            {
                "name":"Мягкий белый",
                "id":"soft_white"
            },
            {
                "name":"Обычный",
                "id":"white"
            },
            {
                "name":"Желтый",
                "id":"yellow"
            },
            {
                "name":"Салатовый",
                "id":"lime"
            },
            {
                "name":"Сиреневый",
                "id":"lavender"
            },
            {
                "name":"Орхидея",
                "id":"orchid"
            },
            {
                "name":"Малина",
                "id":"raspberry"
            },
            {
                "name":"Теплый белый",
                "id":"warm_white"
            },
            {
                "name":"Белый",
                "id":"white"
            },
            {
                "name":"Холодный белый",
                "id":"cold_white"
            },
            {
                "name":"Красный",
                "id":"red"
            },
            {
                "name":"Оранжевый",
                "id":"orange"
            },
            {
                "name":"Зеленый",
                "id":"green"
            },
            {
                "name":"Голубой",
                "id":"cyan"
            },
            {
                "name":"Пурпурный",
                "id":"purple"
            },
            {
                "name":"Лиловый",
                "id":"mauve"
            },
            {
                "name":"Нормальный",
                "id":"white"
            },
            {
                "name":"Дневной белый",
                "id":"daylight"
            },
            {
                "name":"Коралловый",
                "id":"coral"
            },
            {
                "name":"Изумрудный",
                "id":"emerald"
            },
            {
                "name":"Синий",
                "id":"blue"
            },
            {
                "name":"Лунный",
                "id":"moonlight"
            },
            {
                "name":"Фиолетовый",
                "id":"violet"
            },
            {
                "name":"Розовый",
                "id":"orchid"
            },
            {
                "name":"Малиновый",
                "id":"raspberry"
            }
        ],
        "rooms":[
            {
                "name":"Спальня",
                "household_id":"4725907a-267c-4a6d-81ce-976fae37e8fe",
                "id":"e5a088a7-c65d-4a6a-8265-f5a6136be1a5"
            }
        ],
        "current_household_id":"4725907a-267c-4a6d-81ce-976fae37e8fe"
    },
    "post_classify_duration":"170906",
    "original_utterance":"включи",
    "shown_utterance":"включи.",
    "chosen_utterance":"включи"
}
'''


MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_PROTO = '''
AnalyticsInfo {
    key: "HollywoodMusic"
    value {
        ScenarioAnalyticsInfo {
            Intent: "keklol",
            ProductScenarioName: "lolkek1337"
            Events {
                MusicEvent {
                    AnswerType: Track
                }
            }
            Events {
                MusicEvent {
                    Id: "genre:hardbass"
                    AnswerType: Filters
                }
            }
            Objects {
                FirstTrack {
                    Genre: "phonk"
                }
            }
        }
    }
}
WinnerScenario {
    Name: "HollywoodMusic"
}
'''

MEGAMIND_ANALYTICS_INFO_REAL_3_JSON = '''{
    "modifiers_info":{
        "proactivity":{
            "semantic_frames_info":{
                "semantic_frames":[
                    {
                        "name":"personal_assistant.scenarios.convert__ellipsis",
                        "slots":[
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"\\"RUR\\""
                                },
                                "name":"type_from"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"null"
                                },
                                "name":"amount_from"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"100"
                                },
                                "name":"amount_base"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"\\"USD\\""
                                },
                                "name":"type_to"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"1.85986"
                                },
                                "name":"amount_to"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"null"
                                },
                                "name":"where"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"null"
                                },
                                "name":"when"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"\\"RUS\\""
                                },
                                "name":"source"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"2022-07-03"
                                },
                                "name":"source_date"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"Europe/Moscow"
                                },
                                "name":"source_timezone"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"{\\"timezone\\": \\"Europe/Minsk\\", \\"city_cases\\": {\\"preposition\\": \\"в\\", \\"nominative\\": \\"Величковичи\\", ''' \
                                    '''\\"prepositional\\": \\"Величковичах\\", \\"genitive\\": \\"Величковичей\\", \\"dative\\": \\"Величковичам\\"}}"
                                },
                                "name":"resolved_where"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"{\\"status\\": \\"ok\\", \\"from\\": {\\"amount\\": \\"100\\", \\"id\\": \\"RUR\\", \\"name\\": \\"российских рублей\\"}, ''' \
                                    '''\\"source\\": {\\"id\\": \\"RUS\\", \\"name\\": \\"ЦБ РФ\\"}, \\"to\\": {\\"amount\\": \\"1.85986\\", \\"id\\": \\"USD\\", \\"name\\": ''' \
                                    '''\\"американского доллара\\"}, \\"voice_text\\": \\"100 российских рублей - это 1 американский доллар 86 центов\\", \\"time\\": null, \\"date\\": ''' \
                                    '''\\"2022-07-03\\", \\"timezone\\": {\\"hours\\": 3, \\"minutes\\": 0, \\"name\\": \\"Europe/Moscow\\"}}"
                                },
                                "name":"search_response"
                            }
                        ]
                    }
                ],
                "source":"Response"
            },
            "source":"personal_assistant.scenarios.convert__ellipsis"
        }
    },
    "modifiers_analytics_info":{
        "proactivity":{
            "semantic_frames_info":{
                "semantic_frames":[
                    {
                        "name":"personal_assistant.scenarios.convert__ellipsis",
                        "slots":[
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"\\"RUR\\""
                                },
                                "name":"type_from"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"null"
                                },
                                "name":"amount_from"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"100"
                                },
                                "name":"amount_base"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"\\"USD\\""
                                },
                                "name":"type_to"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"1.85986"
                                },
                                "name":"amount_to"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"null"
                                },
                                "name":"where"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"null"
                                },
                                "name":"when"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"\\"RUS\\""
                                },
                                "name":"source"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"2022-07-03"
                                },
                                "name":"source_date"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"Europe/Moscow"
                                },
                                "name":"source_timezone"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"{\\"timezone\\": \\"Europe/Minsk\\", \\"city_cases\\": ''' \
                                    '''{\\"preposition\\": \\"в\\", \\"nominative\\": \\"Величковичи\\", \\"prepositional\\": \\"Величковичах\\", \\"genitive\\": \\"Величковичей\\", \\"dative\\": \\"Величковичам\\"}}"
                                },
                                "name":"resolved_where"
                            },
                            {
                                "typed_value":{
                                    "type":"string",
                                    "string":"{\\"status\\": \\"ok\\", \\"from\\": {\\"amount\\": \\"100\\", \\"id\\": \\"RUR\\", \\"name\\": \\"российских рублей\\"}, \\"source\\": ''' \
                                    '''{\\"id\\": \\"RUS\\", \\"name\\": \\"ЦБ РФ\\"}, \\"to\\": {\\"amount\\": \\"1.85986\\", \\"id\\": \\"USD\\", \\"name\\": \\"американского доллара\\"}, ''' \
                                    '''\\"voice_text\\": \\"100 российских рублей - это 1 американский доллар 86 центов\\", \\"time\\": null, \\"date\\": \\"2022-07-03\\", \\"timezone\\": ''' \
                                    '''{\\"hours\\": 3, \\"minutes\\": 0, \\"name\\": \\"Europe/Moscow\\"}}"
                                },
                                "name":"search_response"
                            }
                        ]
                    }
                ],
                "source":"Response"
            },
            "source":"personal_assistant.scenarios.convert__ellipsis"
        }
    },
    "location":{
        "speed":0,
        "lat":52.62972700000000259,
        "lon":27.254391999999999285,
        "recency":5000,
        "accuracy":15000
    },
    "analytics_info":{
        "Vins":{
            "version":"vins/stable-241-8@9656369",
            "scenario_analytics_info":{
                "product_scenario_name":"convert",
                "objects":[
                    {
                        "vins_gc_meta":{
                            "intent":"personal_assistant.scenarios.repeat"
                        }
                    }
                ],
                "intent":"personal_assistant.scenarios.convert__ellipsis"
            },
            "frame_actions":{
                "0343e053704442b0a3d22c5af19332ff":{
                    "directives":{
                        "list":[
                            {
                                "type_text_directive":{
                                    "name":"render_buttons_type",
                                    "text":"Что ты умеешь?"
                                }
                            }
                        ]
                    }
                },
                "e7ef584d9e3647518fcce07beae18b94":{
                    "directives":{
                        "list":[
                            {
                                "type_text_directive":{
                                    "name":"render_buttons_type",
                                    "text":"Курс доллара к доллару"
                                }
                            }
                        ]
                    }
                },
                "f553a92e6e604d899923e6a0b190abdd":{
                    "directives":{
                        "list":[
                            {
                                "type_text_directive":{
                                    "name":"render_buttons_type",
                                    "text":"Что это за курс?"
                                }
                            }
                        ]
                    }
                },
                "e76d9e680290456f983744730b0b0fda":{
                    "directives":{
                        "list":[
                            {
                                "type_text_directive":{
                                    "name":"render_buttons_type",
                                    "text":"Курс евро к доллару"
                                }
                            }
                        ]
                    }
                },
                "e77f5654c4ba44f8ae14f09eea152da9":{
                    "directives":{
                        "list":[
                            {
                                "type_text_silent_directive":{
                                    "name":"render_buttons_type_silent",
                                    "text":"👍"
                                }
                            },
                            {
                                "callback_directive":{
                                    "name":"update_form",
                                    "payload":{
                                        "form_update":{
                                            "name":"personal_assistant.feedback.feedback_positive"
                                        },
                                        "resubmit":false
                                    }
                                }
                            }
                        ]
                    }
                },
                "89b5864ae80e4b5288da3e23ef1f90d1":{
                    "directives":{
                        "list":[
                            {
                                "type_text_silent_directive":{
                                    "name":"render_buttons_type_silent",
                                    "text":"👎"
                                }
                            },
                            {
                                "callback_directive":{
                                    "name":"update_form",
                                    "payload":{
                                        "form_update":{
                                            "name":"personal_assistant.feedback.feedback_negative"
                                        },
                                        "resubmit":false
                                    }
                                }
                            }
                        ]
                    }
                }
            },
            "semantic_frame":{
                "name":"personal_assistant.scenarios.convert__ellipsis",
                "slots":[
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"\\"RUR\\""
                        },
                        "name":"type_from"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"null"
                        },
                        "name":"amount_from"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"100"
                        },
                        "name":"amount_base"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"\\"USD\\""
                        },
                        "name":"type_to"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"1.85986"
                        },
                        "name":"amount_to"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"null"
                        },
                        "name":"where"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"null"
                        },
                        "name":"when"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"\\"RUS\\""
                        },
                        "name":"source"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"2022-07-03"
                        },
                        "name":"source_date"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"Europe/Moscow"
                        },
                        "name":"source_timezone"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"{\\"timezone\\": \\"Europe/Minsk\\", \\"city_cases\\": {\\"preposition\\": \\"в\\", \\"nominative\\": \\"Величковичи\\", \\"prepositional\\": ''' \
                            '''\\"Величковичах\\", \\"genitive\\": \\"Величковичей\\", \\"dative\\": \\"Величковичам\\"}}"
                        },
                        "name":"resolved_where"
                    },
                    {
                        "typed_value":{
                            "type":"string",
                            "string":"{\\"status\\": \\"ok\\", \\"from\\": {\\"amount\\": \\"100\\", \\"id\\": \\"RUR\\", \\"name\\": \\"российских рублей\\"}, ''' \
                            '''\\"source\\": {\\"id\\": \\"RUS\\", \\"name\\": \\"ЦБ РФ\\"}, \\"to\\": {\\"amount\\": \\"1.85986\\", \\"id\\": \\"USD\\", \\"name\\": \\"американского доллара\\"}''' \
                            ''', \\"voice_text\\": \\"100 российских рублей - это 1 американский доллар 86 центов\\", \\"time\\": null, \\"date\\": \\"2022-07-03\\", \\"timezone\\": ''' \
                            '''{\\"hours\\": 3, \\"minutes\\": 0, \\"name\\": \\"Europe/Moscow\\"}}"
                        },
                        "name":"search_response"
                    }
                ]
            }
        }
    },
    "pre_classify_duration":"1670",
    "winner_scenario":{
        "name":"Vins"
    },
    "modifier_analytics_info":{
    },
    "original_utterance":"чего",
    "shown_utterance":"Чего?",
    "chosen_utterance":"чего"
}'''

MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_NULL_GENRE_PROTO = '''
AnalyticsInfo {
    key: "HollywoodMusic"
    value {
        ScenarioAnalyticsInfo {
            Intent: "keklol",
            ProductScenarioName: "lolkek1337"
            Events {
                MusicEvent {
                    AnswerType: Track
                }
            }
            Events {
                MusicEvent {
                    Id: "genre:hardbass"
                    AnswerType: Filters
                }
            }
            Objects {
            }
        }
    }
}
WinnerScenario {
    Name: "HollywoodMusic"
}
'''

MEGAMIND_ANALYTICS_INFO_WITHOUT_SCENARIO_ANALYTICS_INFO_PROTO = '''
AnalyticsInfo {
    key: "Lolkek"
    value {
    }
}
WinnerScenario {
    Name: "Lolkek"
}
'''


MEGAMIND_ANALYTICS_INFO_WITHOUT_ANALYTICS_INFO_PROTO = '''
WinnerScenario {
    Name: "Lolkek"
}
'''

TR_NAVI_APP_PROTO = '''
AppId: "ru.yandex.mobile.navigator"
Lang: "tr"
'''

TR_NAVI_WITH_VERSION_APP_PROTO = '''
AppId: "ru.yandex.mobile.navigator"
Lang: "tr"
AppVersion: "69"
'''

TR_NAVI_WITH_PLATFORM_APP_PROTO = '''
AppId: "ru.yandex.mobile.navigator"
Lang: "tr"
Platform: "lolkek"
'''

RU_NAVI_APP_PROTO = '''
AppId: "ru.yandex.mobile.navigator"
Lang: "ru"
'''

RU_NAVI_WITH_PLATFORM_APP_PROTO = '''
AppId: "ru.yandex.mobile.navigator"
Lang: "ru"
Platform: "lolkek"
'''

RU_NAVI_WITH_EMPTY_PLATFORM_APP_PROTO = '''
AppId: "ru.yandex.mobile.navigator"
Lang: "ru"
Platform: ""
'''

RU_NAVI_WITH_VERSION_APP_PROTO = '''
AppId: "ru.yandex.mobile.navigator"
Lang: "ru"
AppVersion: "69"
'''

TR_NAVI_2_APP_PROTO = '''
AppId: "ru.yandex.mobile.navigator"
Lang: "tr-TR"
'''

YANDEX_PHONE_APP_PROTO = '''
AppId: "com.yandex.launcher"
DeviceManufacturer: "Yandex"
'''

NOT_YANDEX_PHONE_APP_PROTO = '''
AppId: "com.yandex.launcher"
'''

NOT_YANDEX_PHONE_2_APP_PROTO = '''
AppId: "lolkek"
DeviceManufacturer: "Yandex"
'''

OTHER_APP_PROTO = '''
AppId: "lolkek"
'''


QUASAR_APP_PROTO = '''
AppId: "ru.yandex.quasar.app"
'''


CALLBACK_ARGS_WITH_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON = '''
{
    "suggest_block": {
        "form_update": {
            "name": "lolkek"
        }
    }
}
'''

CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON = '''
{
    "suggest_block": {
        "form_update": {
        }
    }
}
'''


META_PURE_GC_FIRST_JSON = '''
[
    {
        "pure_gc": {},
    },
    {
        "type": "form_restored",
        "overriden_form": "lolkek1337"
    }
]
'''


STRING_SLOT_PROTO = '''
TypedValue {
    Type: "string"
    String: "lolkek"
}
'''

JSON_SLOT_PROTO = '''
TypedValue {
    Type: "string"
    String: "{\\"lol\\": \\"kek\\"}"
}
'''


CARDS_1 = '''[
    {
        "text":"Видео-контент по запросу «супер котэ»",
        "type":"simple_text"
    }
]'''


CARDS_EXPECTED_1 = '''[
    {
        "text":"Видео-контент по запросу «супер котэ»",
        "type":"simple_text"
    }
]'''


CARDS_2 = '''[
    {
        "body":{
            "background":[
                {
                    "color":"#FFFFFF",
                    "type":"div-solid-background"
                }
            ],
            "states":[
                {
                    "blocks":[
                        {
                            "size":"xs",
                            "type":"div-separator-block"
                        },
                        {
                            "action":{
                                "log_id":"skill_recommendation__get_greetings__editorial#__show_traffic",
                                "url":"dialog-action://?directives=%5B%0A%20%20%20%20%7B%22type%22%3A%22client_action%22%2C%22sub_name%22%3A%22skill_recommendation__get_greetings__editorial%23__show_traffic%22%2C%22name%22%3A%22type%22%2C%22payload%22%3A%7B%22text%22%3A%22%5Cu041a%5Cu0430%5Cu043a%5Cu0438%5Cu0435%20%5Cu0441%5Cu0435%5Cu0439%5Cu0447%5Cu0430%5Cu0441%20%5Cu043f%5Cu0440%5Cu043e%5Cu0431%5Cu043a%5Cu0438%3F%22%7D%7D%2C%7B%22type%22%3A%22server_action%22%2C%22name%22%3A%22external_source_action%22%2C%22ignore_answer%22%3Atrue%2C%22payload%22%3A%7B%22utm_campaign%22%3A%22%22%2C%22utm_term%22%3A%22%22%2C%22utm_content%22%3A%22textlink%22%2C%22utm_source%22%3A%22Yandex_Alisa%22%2C%22utm_medium%22%3A%22get_greetings%22%2C%22request_id%22%3A%2280f47636-10df-4ae9-85ba-61d557361675%22%7D%7D%2C%7B%22type%22%3A%22server_action%22%2C%22name%22%3A%22on_card_action%22%2C%22ignore_answer%22%3Atrue%2C%22payload%22%3A%7B%22item_number%22%3A1%2C%22request_id%22%3A%2280f47636-10df-4ae9-85ba-61d557361675%22%2C%22intent_name%22%3A%22personal_assistant.scenarios.skill_recommendation%22%2C%22card_id%22%3A%22skill_recommendation%22%2C%22case_name%22%3A%22skill_recommendation__get_greetings__editorial%23__show_traffic%22%7D%7D%0A%20%20%20%20%5D"
                            },
                            "side_element":{
                                "element":{
                                    "image_url":"https://avatars.mds.yandex.net/get-dialogs/758954/onboard_Traffic/mobile-logo-x2",
                                    "ratio":1,
                                    "type":"div-image-element"
                                },
                                "position":"left",
                                "size":"s"
                            },
                            "text":"<font color=\\"#7f7f7f\\">Расскажу о движении в городе.</font>",
                            "text_max_lines":2,
                            "title":"Какие сейчас пробки",
                            "title_max_lines":2,
                            "type":"div-universal-block"
                        },
                        {
                            "action":{
                                "log_id":"skill_recommendation__get_greetings__editorial#__onboarding_weather2",
                                "url":"dialog-action://?directives=%5B%0A%20%20%20%20%7B%22type%22%3A%22client_action%22%2C%22sub_name%22%3A%22skill_recommendation__get_greetings__editorial%23__onboarding_weather2%22%2C%22name%22%3A%22type%22%2C%22payload%22%3A%7B%22text%22%3A%22%5Cu041f%5Cu043e%5Cu0433%5Cu043e%5Cu0434%5Cu0430%20%5Cu043d%5Cu0430%20%5Cu0432%5Cu044b%5Cu0445%5Cu043e%5Cu0434%5Cu043d%5Cu044b%5Cu0445%22%7D%7D%2C%7B%22type%22%3A%22server_action%22%2C%22name%22%3A%22external_source_action%22%2C%22ignore_answer%22%3Atrue%2C%22payload%22%3A%7B%22utm_campaign%22%3A%22%22%2C%22utm_term%22%3A%22%22%2C%22utm_content%22%3A%22textlink%22%2C%22utm_source%22%3A%22Yandex_Alisa%22%2C%22utm_medium%22%3A%22get_greetings%22%2C%22request_id%22%3A%2280f47636-10df-4ae9-85ba-61d557361675%22%7D%7D%2C%7B%22type%22%3A%22server_action%22%2C%22name%22%3A%22on_card_action%22%2C%22ignore_answer%22%3Atrue%2C%22payload%22%3A%7B%22item_number%22%3A2%2C%22request_id%22%3A%2280f47636-10df-4ae9-85ba-61d557361675%22%2C%22intent_name%22%3A%22personal_assistant.scenarios.skill_recommendation%22%2C%22card_id%22%3A%22skill_recommendation%22%2C%22case_name%22%3A%22skill_recommendation__get_greetings__editorial%23__onboarding_weather2%22%7D%7D%0A%20%20%20%20%5D"
                            },
                            "side_element":{
                                "element":{
                                    "image_url":"https://avatars.mds.yandex.net/get-dialogs/1535439/onboard_Wheather/mobile-logo-x2",
                                    "ratio":1,
                                    "type":"div-image-element"
                                },
                                "position":"left",
                                "size":"s"
                            },
                            "text":"<font color=\\"#7f7f7f\\">Одевайтесь по погоде.</font>",
                            "text_max_lines":2,
                            "title":"Погода на выходных",
                            "title_max_lines":2,
                            "type":"div-universal-block"
                        },
                        {
                            "action":{
                                "log_id":"skill_recommendation__get_greetings__editorial#__a557c651-94d8-48fd-9c43-d2b644615050",
                                "url":"dialog-action://?directives=%5B%0A%20%20%20%20%7B%22type%22%3A%22client_action%22%2C%22sub_name%22%3A%22skill_recommendation__get_greetings__editorial%23__a557c651-94d8-48fd-9c43-d2b644615050%22%2C%22name%22%3A%22type%22%2C%22payload%22%3A%7B%22text%22%3A%22%5Cu0417%5Cu0430%5Cu043f%5Cu0443%5Cu0441%5Cu0442%5Cu0438%20%5Cu043d%5Cu0430%5Cu0432%5Cu044b%5Cu043a%20%5Cu0417%5Cu0430%5Cu043d%5Cu0438%5Cu043c%5Cu0430%5Cu0442%5Cu0435%5Cu043b%5Cu044c%5Cu043d%5Cu044b%5Cu0435%20%5Cu0438%5Cu0441%5Cu0442%5Cu043e%5Cu0440%5Cu0438%5Cu0438%22%7D%7D%2C%7B%22type%22%3A%22server_action%22%2C%22name%22%3A%22external_source_action%22%2C%22ignore_answer%22%3Atrue%2C%22payload%22%3A%7B%22utm_campaign%22%3A%22%22%2C%22utm_term%22%3A%22%22%2C%22utm_content%22%3A%22textlink%22%2C%22utm_source%22%3A%22Yandex_Alisa%22%2C%22utm_medium%22%3A%22get_greetings%22%2C%22request_id%22%3A%2280f47636-10df-4ae9-85ba-61d557361675%22%7D%7D%2C%7B%22type%22%3A%22server_action%22%2C%22name%22%3A%22on_card_action%22%2C%22ignore_answer%22%3Atrue%2C%22payload%22%3A%7B%22item_number%22%3A3%2C%22request_id%22%3A%2280f47636-10df-4ae9-85ba-61d557361675%22%2C%22intent_name%22%3A%22personal_assistant.scenarios.skill_recommendation%22%2C%22card_id%22%3A%22skill_recommendation%22%2C%22case_name%22%3A%22skill_recommendation__get_greetings__editorial%23__a557c651-94d8-48fd-9c43-d2b644615050%22%7D%7D%0A%20%20%20%20%5D"
                            },
                            "side_element":{
                                "element":{
                                    "image_url":"https://avatars.mds.yandex.net/get-dialogs/1530877/556dd0add524860ba900/mobile-logo-x2",
                                    "ratio":1,
                                    "type":"div-image-element"
                                },
                                "position":"left",
                                "size":"s"
                            },
                            "text":"<font color=\\"#7f7f7f\\">Сочини забавные истории вместе с Алисой! Она попросит по очереди называть разные слова (имена, прилагательные, и другие). ''' \
                            '''Потом она вставит эти слова в нужные места в тексте и прочитает получившуюся занимательную историю 😉<br/><br/>Подписывайся на сообщество ВК &quot;Занимательные ''' \
                            '''истории голосовых помощников&quot;, чтобы получить ответ на свою оценку, предложить новый текст, и раньше всех узнавать новости игры!<br/>https://vk.com/fillinthetextbot.</font>",
                            "text_max_lines":2,
                            "title":"Запусти навык Занимательные истории",
                            "title_max_lines":2,
                            "type":"div-universal-block"
                        },
                        {
                            "size":"xs",
                            "type":"div-separator-block"
                        }
                    ],
                    "state_id":1
                }
            ]
        },
        "text":"...",
        "type":"div_card"
    }
]'''


CARDS_EXPECTED_2 = '''[
    {
        "actions":[
            "skill_recommendation__get_greetings__editorial#__show_traffic",
            "skill_recommendation__get_greetings__editorial#__onboarding_weather2",
            "skill_recommendation__get_greetings__editorial#__a557c651-94d8-48fd-9c43-d2b644615050"
        ],
        "card_id":"skill_recommendation",
        "intent_name":"personal_assistant\\tscenarios\\tskill_recommendation",
        "text":"...",
        "type":"div_card"
    }
]'''


CARDS_3 = '''[
    {
        "body":{
            "background":[
                {
                    "color":"#FFFFFF",
                    "type":"div-solid-background"
                }
            ],
            "states":[
                {
                    "action":{
                        "log_id":"whole_card",
                        "url":"https://translate.yandex.ru/?lang=ru-en&text=%D0%B4%D0%BB%D1%8F%20%D1%81%D0%BE%D0%B1%D0%B0%D0%BA&searchapp_from_source=alice"
                    },
                    "blocks":[
                        {
                            "children":[
                                {
                                    "size":"xs",
                                    "type":"div-separator-block"
                                },
                                {
                                    "title":"<font color=\\"#7F7F7F\\">РУССКИЙ — АНГЛИЙСКИЙ</font>",
                                    "title_style":"card_header",
                                    "type":"div-universal-block"
                                },
                                {
                                    "columns":[
                                        {
                                            "left_padding":"zero",
                                            "right_padding":"zero"
                                        }
                                    ],
                                    "rows":[
                                        {
                                            "cells":[
                                                {
                                                    "horizontal_alignment":"left",
                                                    "text":"для собак",
                                                    "text_style":"text_m_medium",
                                                    "vertical_alignment":"bottom"
                                                }
                                            ],
                                            "type":"row_element"
                                        }
                                    ],
                                    "type":"div-table-block"
                                },
                                {
                                    "columns":[
                                        {
                                            "left_padding":"zero"
                                        },
                                        {
                                            "left_padding":"zero"
                                        }
                                    ],
                                    "rows":[
                                        {
                                            "cells":[
                                                {
                                                    "horizontal_alignment":"left",
                                                    "text":"<font color=\\"#7F7F7F\\">нареч</font>",
                                                    "text_style":"text_s",
                                                    "vertical_alignment":"bottom"
                                                },
                                                {
                                                    "horizontal_alignment":"left",
                                                    "text":"for dogs",
                                                    "text_style":"title_m",
                                                    "vertical_alignment":"bottom"
                                                }
                                            ],
                                            "type":"row_element"
                                        },
                                        {
                                            "cells":[
                                                {
                                                    "horizontal_alignment":"left",
                                                    "text":"<font color=\\"#7F7F7F\\">нареч</font>",
                                                    "text_style":"text_s",
                                                    "vertical_alignment":"bottom"
                                                },
                                                {
                                                    "horizontal_alignment":"left",
                                                    "text":"for the dog",
                                                    "text_style":"title_m",
                                                    "vertical_alignment":"bottom"
                                                }
                                            ],
                                            "type":"row_element"
                                        }
                                    ],
                                    "type":"div-table-block"
                                },
                                {
                                    "items":[
                                        {
                                            "action":{
                                                "log_id":"translate_pronounce_repeat",
                                                "url":"dialog-action://?directives=%5B%7B%22ignore_answer%22%3Afalse%2C%22is_led_silent%22%3Afalse%2C%22name%22%3A%22repeat%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%227e46f754-ffe9-40a6-a154-5f0d699754d2%22%2C%22%40scenario_name%22%3A%22Translation%22%2C%22voice%22%3A%22%3Cspeaker%20voice%3D%5C%22oksana%5C%22%20effect%3D%5C%22translate_oksana_en%5C%22%20lang%3D%5C%22en%5C%22%20speed%3D%5C%220.9%5C%22%3Efor%20dogs%22%7D%2C%22type%22%3A%22server_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%227e46f754-ffe9-40a6-a154-5f0d699754d2%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22c67f9051-edb4e2ed-601458f-a819200a%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%227e46f754-ffe9-40a6-a154-5f0d699754d2%22%2C%22scenario_name%22%3A%22Translation%22%7D%2C%22type%22%3A%22server_action%22%7D%5D"
                                            },
                                            "image":{
                                                "image_url":"https://avatars.mds.yandex.net/get-bass/1593034/serp_gallery_48x48_97b0f8433ebd73be04900d48d17e44bb39d97f35731b3211bf7e44bb581daeed.png/orig",
                                                "type":"div-image-element"
                                            }
                                        }
                                    ],
                                    "type":"div-buttons-block"
                                },
                                {
                                    "has_delimiter":1,
                                    "size":"xs",
                                    "type":"div-separator-block"
                                },
                                {
                                    "text":"<font color=\\"#0A4CD3\\">ОТКРЫТЬ В ПЕРЕВОДЧИКЕ</font>",
                                    "type":"div-footer-block"
                                }
                            ],
                            "direction":"vertical",
                            "height":{
                                "type":"predefined",
                                "value":"wrap_content"
                            },
                            "type":"div-container-block",
                            "width":{
                                "type":"predefined",
                                "value":"match_parent"
                            }
                        }
                    ],
                    "state_id":1
                }
            ]
        },
        "text":"...",
        "type":"div_card"
    }
]'''


CARDS_EXPECTED_3 = '''[
    {
        "actions":[
            "whole_card",
            null
        ],
        "card_id":null,
        "intent_name":null,
        "text":"...",
        "type":"div_card"
    }
]'''


CARDS_4 = '''[
    {
        "buttons":[
            {
                "directives":[
                    {
                        "name":"open_uri",
                        "payload":{
                            "uri":"https://yandex.ru/search/touch/?l10n=ru-RU&lr=213&query_source=alice&text=%D1%84%D1%83%D0%BD%D0%B3%D0%BE%D0%B4%D0%B5%D1%80%D0%B8%D0%BB%20%D1%86%D0%B5%D0%BD%D0%B0%20%D0%B2%20%D0%B0%D0%BF%D1%82%D0%B5%D0%BA%D0%B0%D1%85%20%D0%BA%D0%B8%D1%82%20%D1%84%D0%B0%D1%80%D0%BC"
                        },
                        "sub_name":"open_uri",
                        "type":"client_action"
                    },
                    {
                        "ignore_answer":true,
                        "is_led_silent":true,
                        "name":"on_suggest",
                        "payload":{
                            "@request_id":"e8925e45-8ab9-486e-8215-5521267dc590",
                            "@scenario_name":"Vins",
                            "button_id":"686575b8-5f1f1d2c-c6e3aca2-9e83225c",
                            "caption":"Поискать в Яндексе",
                            "request_id":"e8925e45-8ab9-486e-8215-5521267dc590",
                            "scenario_name":"Search"
                        },
                        "type":"server_action"
                    }
                ],
                "title":"Поискать в Яндексе",
                "type":"action"
            }
        ],
        "text":"Сейчас найдём",
        "type":"text_with_button"
    }
]'''

CARDS_EXPECTED_4 = '''[
    {
        "text":"Сейчас найдём",
        "type":"text_with_button"
    }
]'''


@pytest.mark.parametrize('analytics_info_text,expected', [
    (MEGAMIND_ANALYTICS_INFO_PROTO, 'keklol'),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_SCENARIO_ANALYTICS_INFO_PROTO, None),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_ANALYTICS_INFO_PROTO, None)
])
def test_get_intent(analytics_info_text, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = get_intent(serialized_analytics_info)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('analytics_info_text,expected', [
    (MEGAMIND_ANALYTICS_INFO_PROTO, 'lolkek1337'),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_SCENARIO_ANALYTICS_INFO_PROTO, None),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_ANALYTICS_INFO_PROTO, None)
])
def test_get_product_scenario_name(analytics_info_text, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = get_product_scenario_name(serialized_analytics_info)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


# TODO(ran1s) add tests in case of unknown enum value
@pytest.mark.parametrize('analytics_info_text,expected', [
    (MEGAMIND_ANALYTICS_INFO_PROTO, 'Track'),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_SCENARIO_ANALYTICS_INFO_PROTO, None),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_ANALYTICS_INFO_PROTO, None)
])
def test_get_music_answer_type(analytics_info_text, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = get_music_answer_type(serialized_analytics_info)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('analytics_info_text,expected', [
    (MEGAMIND_ANALYTICS_INFO_PROTO, None),
    (MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_PROTO, 'phonk'),
    (MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_NULL_GENRE_PROTO, None),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_SCENARIO_ANALYTICS_INFO_PROTO, None),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_ANALYTICS_INFO_PROTO, None)
])
def test_get_music_genre(analytics_info_text, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = get_music_genre(serialized_analytics_info)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('analytics_info_text,expected', [
    (MEGAMIND_ANALYTICS_INFO_PROTO, None),
    (MEGAMIND_ANALYTICS_INFO_HOLLYWOOD_MUSIC_PROTO, 'hardbass'),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_SCENARIO_ANALYTICS_INFO_PROTO, None),
    (MEGAMIND_ANALYTICS_INFO_WITHOUT_ANALYTICS_INFO_PROTO, None)
])
def test_get_filters_genre(analytics_info_text, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = get_filters_genre(serialized_analytics_info)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('app_info_text,expected', [
    (TR_NAVI_APP_PROTO, 'tr_navigator'),
    (RU_NAVI_APP_PROTO, 'navigator'),
    (TR_NAVI_2_APP_PROTO, 'tr_navigator'),
    (YANDEX_PHONE_APP_PROTO, 'yandex_phone'),
    (NOT_YANDEX_PHONE_APP_PROTO, 'launcher'),
    (NOT_YANDEX_PHONE_2_APP_PROTO, 'other'),
    (OTHER_APP_PROTO, 'other'),
    (QUASAR_APP_PROTO, 'quasar'),
])
def test_get_app(app_info_text, expected):
    serialized_app_info = google.protobuf.text_format.Parse(
        app_info_text, TClientInfoProto()).SerializeToString()

    actual = get_app(serialized_app_info)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('app_info_text,expected', [
    (TR_NAVI_APP_PROTO, 'iphone'),
    (RU_NAVI_APP_PROTO, 'android'),
    (TR_NAVI_WITH_PLATFORM_APP_PROTO, 'lolkek'),
    (RU_NAVI_WITH_PLATFORM_APP_PROTO, 'lolkek'),
    (RU_NAVI_WITH_EMPTY_PLATFORM_APP_PROTO, 'android')
])
def test_get_platform(app_info_text, expected):
    serialized_app_info = google.protobuf.text_format.Parse(
        app_info_text, TClientInfoProto()).SerializeToString()

    actual = get_platform(serialized_app_info)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('app_info_text,expected', [
    (TR_NAVI_APP_PROTO, '400'),
    (TR_NAVI_WITH_PLATFORM_APP_PROTO, ''),
    (TR_NAVI_WITH_VERSION_APP_PROTO, '400'),
    (RU_NAVI_WITH_VERSION_APP_PROTO, '69'),
])
def test_get_version(app_info_text, expected):
    serialized_app_info = google.protobuf.text_format.Parse(
        app_info_text, TClientInfoProto()).SerializeToString()

    actual = get_version(serialized_app_info)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('sound_level,app_id,expected', [
    (100., 'com.yandex.tv.alice', 10.),
    (50., 'com.yandex.tv.alice', 5.),
    (10., 'com.yandex.alice', 10.),
    (5., 'com.yandex.alice', 5.),
    (None, 'com.yandex.alice', None),
    (None, 'com.yandex.tv.alice', None),
])
def test_get_sound_level(sound_level, app_id, expected):
    actual = get_sound_level(sound_level, app_id)
    if expected is None:
        assert actual is None
    else:
        assert isinstance(actual, float)
        assert expected == pytest.approx(actual)


@pytest.mark.parametrize('analytics_info_text,expected', [
    (MEGAMIND_ANALYTICS_INFO_PROTO, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_MEDIA_DEVICE_PROTO, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_BANNED_ANALYTICS_TYPE_PROTO, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_SMART_SPEAKER_PROTO, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_LOLKEK_PROTO, True),
    (MEGAMIND_ANALYTICS_INFO_WITH_USERS_INFO_MEDIA_DEVICE_PROTO, True),
    (MEGAMIND_ANALYTICS_INFO_WITH_USERS_INFO_LOLKEK_PROTO, True),
    (MEGAMIND_ANALYTICS_INFO_WITH_USERS_INFO_BANNED_ANALYTICS_TYPE_PROTO, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_USERS_INFO_SMART_SPEAKER_PROTO, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_IOT_USER_INFO_QUASAR_INFO_PROTO, False),
])
def test_smart_home_user(analytics_info_text, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = smart_home_user(serialized_analytics_info)
    assert expected is actual


@pytest.mark.parametrize('analytics_info_json,expected', [
    (MEGAMIND_ANALYTICS_INFO_REAL_JSON, False),
])
def test_smart_home_user_json(analytics_info_json, expected):
    serialized_analytics_info = google.protobuf.json_format.ParseDict(
        json.loads(analytics_info_json), TMegamindAnalyticsInfo()).SerializeToString()

    actual = smart_home_user(serialized_analytics_info)
    assert expected is actual


@pytest.mark.parametrize('analytics_info_text,callback_args,expected', [
    ('', CALLBACK_ARGS_WITH_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, 'lolkek'),
    (MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, 'keklol1337'),
    (MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_WITHOUT_INTENT_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, 'keklol69'),
    (MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_WITHOUT_ALL_INTENTS_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, ''),
    (MEGAMIND_ANALYTICS_INFO_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, 'Lolkek'),
])
def test_get_path(analytics_info_text, callback_args, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = get_path(serialized_analytics_info, callback_args)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('analytics_info_json,callback_args,expected', [
    (MEGAMIND_ANALYTICS_INFO_REAL_2_JSON, '{}',
     'personal_assistant.scenarios.player_continue'),
    (MEGAMIND_ANALYTICS_INFO_REAL_3_JSON, '{}',
     'personal_assistant.scenarios.repeat')
])
def test_get_path_json(analytics_info_json, callback_args, expected):
    serialized_analytics_info = google.protobuf.json_format.ParseDict(
        json.loads(analytics_info_json), TMegamindAnalyticsInfo()).SerializeToString()

    actual = get_path(serialized_analytics_info, callback_args)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('analytics_info_text,callback_args,expected', [
    ('', CALLBACK_ARGS_WITH_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_WITHOUT_INTENT_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, False),
    (MEGAMIND_ANALYTICS_INFO_WITH_VINS_ERROR_META_WITHOUT_ALL_INTENTS_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, False),
    (MEGAMIND_ANALYTICS_INFO_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, False),
    (MEGAMIND_ANALYTICS_INFO_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, False),
    (MEGAMIND_ANALYTICS_INFO_PROTO, CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON,
     False),
    (MEGAMIND_ANALYTICS_INFO_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, False),
    (MEGAMIND_ANALYTICS_INFO_PROTO,
     CALLBACK_ARGS_WITHOUT_SUGGEST_FORM_UPDATE_NAME_BLOCK_JSON, False),
])
def test_form_changed(analytics_info_text, callback_args, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = form_changed(serialized_analytics_info, callback_args)
    assert expected is actual


@pytest.mark.parametrize('analytics_info_text,expected', [
    (MEGAMIND_ANALYTICS_INFO_VINS_WITH_SLOTS_PROTO, [{'name': 'lolkek'}]),
    (MEGAMIND_ANALYTICS_INFO_PROTO, []),
])
def test_get_slots(analytics_info_text, expected):
    serialized_analytics_info = google.protobuf.text_format.Parse(
        analytics_info_text, TMegamindAnalyticsInfo()).SerializeToString()

    actual = get_slots(serialized_analytics_info)
    assert expected == actual


@pytest.mark.parametrize('slot_text,expected', [
    (STRING_SLOT_PROTO, 'lolkek'),
    (JSON_SLOT_PROTO, {'lol': 'kek'}),
    ('', None),
])
def test_get_slot_value(slot_text, expected):
    serialized_slot = google.protobuf.text_format.Parse(
        slot_text, TSemanticFrame.TSlot()).SerializeToString()

    actual = get_slot_value(serialized_slot)
    if expected is None:
        assert actual is None
    else:
        assert expected == actual


@pytest.mark.parametrize('cards_str,expected_str', [
    (CARDS_1, CARDS_EXPECTED_1),
    (CARDS_2, CARDS_EXPECTED_2),
    (CARDS_3, CARDS_EXPECTED_3),
    (CARDS_4, CARDS_EXPECTED_4),
])
def test_parse_cards(cards_str, expected_str):
    cards = []
    for card in json.loads(cards_str):
        cards.append(google.protobuf.json_format.ParseDict(
            card, TSpeechKitResponseProto.TResponse.TCard()).SerializeToString())

    actual = parse_cards(cards)
    expected = json.loads(expected_str)
    assert expected == actual
