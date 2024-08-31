from alice.uniproxy.library.extlog import log_filter
from alice.uniproxy.library.extlog import SessionLogger
from alice.uniproxy.library.extlog.mocks import SessionLogMock
from alice.uniproxy.library.events import Event
from alice.uniproxy.library.testing import run_async
import copy


DIRECTIVE = {
    "Name": "Pretty Directive",
    "Body": {
        "Field1": {
            "A": 1,
            "B": ["one", "two", "three"],
            "C": {"Cx": True, "Cy": False}
        },
        "Field2": [{"A": 1}, {"B": 2}],
        "Field3": {"A": "a", "B": "b"}
    }
}


# -------------------------------------------------------------------------------------------------
def test_white_filter():
    # WHITELIST 1
    res = log_filter.WhiteFilter({
        "Name": True,
        "Body": {"Field2": True}
    }).filter(DIRECTIVE)
    assert res == {
        "Name": "Pretty Directive",
        "Body": {"Field2": [{"A": 1}, {"B": 2}]}
    }
    assert res["Body"]["Field2"][0] is not DIRECTIVE["Body"]["Field2"][0]  # the field is deepcopied

    # WHITELIST 2
    res = log_filter.WhiteFilter({
        "Body": {
            "Field1": {"C": {"Cy": True}},
            "Field3": {"B": "b"}
        }
    }).filter(DIRECTIVE)
    assert res == {
        "Body": {
            "Field1": {"C": {"Cy": False}},
            "Field3": {"B": "b"}
        }
    }

    # WHITELIST 3
    res = log_filter.WhiteFilter({
        "Name": True,
        "Body": True
    }).filter(DIRECTIVE)
    assert res == DIRECTIVE
    # fields is deepcopied
    assert res["Body"] is not DIRECTIVE["Body"]
    assert res["Body"]["Field1"] is not DIRECTIVE["Body"]["Field1"]
    assert res["Body"]["Field3"] is not DIRECTIVE["Body"]["Field3"]


# -------------------------------------------------------------------------------------------------
def test_black_filter():
    # BLACKLIST 1
    res = log_filter.BlackFilter({
        "Name": True,
        "Body": {"Field2": True}
    }).filter(DIRECTIVE)
    assert res == {
        "Body": {
            "Field1": DIRECTIVE["Body"]["Field1"],
            "Field3": DIRECTIVE["Body"]["Field3"]
        }
    }
    assert res["Body"]["Field1"] is not DIRECTIVE["Body"]["Field1"]
    assert res["Body"]["Field3"] is not DIRECTIVE["Body"]["Field3"]

    # BLACKLIST 2
    res = log_filter.BlackFilter({
        "Body": {
            "Field1": {"C": {"Cy": True}},
            "Field3": {"B": "b"}
        }
    }).filter(DIRECTIVE)
    assert res == {
        "Name": DIRECTIVE["Name"],
        "Body": {
            "Field1": {
                "A": DIRECTIVE["Body"]["Field1"]["A"],
                "B": DIRECTIVE["Body"]["Field1"]["B"],
                "C": {"Cx": True}
            },
            "Field2": DIRECTIVE["Body"]["Field2"],
            "Field3": {"A": DIRECTIVE["Body"]["Field3"]["A"]}
        }
    }

    # BLACKLIST 3
    res = log_filter.BlackFilter({
        "Name": True,
        "Body": True
    }).filter(DIRECTIVE)
    assert res == {}

    # BLACKLIST 4
    res = log_filter.BlackFilter({}).filter(DIRECTIVE)
    assert res == DIRECTIVE
    assert res is not DIRECTIVE


# -------------------------------------------------------------------------------------------------
VINS_REQUEST = {
    "type": "VinsRequest",
    "ForEvent": "2425d855-3201-4241-8920-be545396de1c",
    "Body": {
        "application": {
            "app_id": "ru.yandex.searchplugin",
            "app_version": "9.99",
            "platform": "android"
        },
        "header": {
            "prev_req_id": "f38f233f-7238-45d7-940b-a12702b91e42",
            "request_id": "316d6461-2fdd-4605-b556-8d38f3f9a1c0",
            "sequence_number": 230
        },
        "request": {
            "activation_type": "auto_listening",
            "additional_options": {},
            "device_state": {
                "is_default_assistant": False,
                "sound_level": 10,
                "sound_muted": False,
                "device_id": "4d7fcc5e87b25fd4af602f26c14d5beb"
            },
            "event": {
                "asr_result": [{}, {}, {}],
                "end_of_utterance": True,
                "name": "",
                "type": "voice_input",
                "biometry_classification": {
                    "scores": [
                        {
                            "classname": "adult",
                            "confidence": 0.25157397985458374,
                            "tag": "children"
                        }, {
                            "classname": "child",
                            "confidence": 0.7484260201454163,
                            "tag": "children"
                        }
                    ],
                    "status": "ok"
                },
                "biometry_scoring": {
                    "group_id": "35498ee27a3645c8f584c6af569df49c",
                    "scores_with_mode": [],
                    "status": "ok"
                },
            },
            "experiments": {
                "activation_search_redirect_experiment": "1",
                "afisha_poi_events": "1",
                "alice_voice_news": "1"
            },
            "laas_region": {},
            "location": {},
            "personal_data": {},
            "reset_session": False,
            "smart_home": {
                "payload": {},
                "request_id": "bf4c2e9a-295a-403b-a19b-9fc1d772de2d",
                "status": "ok"
            },
            "test_ids": [
                207280,
                206002,
                206750,
            ],
            "voice_session": True
        }
    }
}

VINS_RESPONSE = {
    "directive": {
        "header": {
            "messageId": "0c8dd750-6130-44e5-a7ac-b38d02337ad7",
            "name": "VinsResponse",
            "namespace": "Vins",
            "refMessageId": "23edec4c-8388-46c1-b9d7-09d14929da2d"
        },
        "payload": {
            "header": {
                "dialog_id": None,
                "prev_req_id": "994b52fd-6641-485b-ad65-b0b3e9c0d186",
                "request_id": "d3c1768f-b5c7-4854-89b8-4fbbff85bc25",
                "response_id": "5e1e3a14468845dd932c864094aef642",
                "sequence_number": 59
            },
            "response": {
                "card": {},
                "cards": [{}],
                "directives": [{}],
                "experiments": {
                    "analytics_info": "1",
                    "authorized_personal_playlists": "1",
                    "enable_ner_for_skills": "1",
                    "enable_reminders_todos": "1"
                },
                "features": {
                    "form_info": {}
                },
                "megamind_actions": None,
                "meta": [{}],
                "quality_storage": {}
            },
            "voice_response": {
                "directives": [],
                "output_speech": {
                    "text": "бла-бла-бла",
                    "type": "simple"
                },
                "should_listen": False
            }
        }
    }
}

TTS_GENERATE = Event({
    "header": {
        "messageId": "bbae1064-d463-4160-b213-69086b7ddba6",
        "name": "Generate",
        "namespace": "TTS",
    },
    "payload": {
        "advancedASROptions": {},
        "apiKey": "cc96633d-59d4-4724-94bd-f5db2f02ad13",
        "application": {},
        "asr_balancer": "yaldi.alice.yandex.net",
        "device": "HONOR DUA-L22",
        "device_manufacturer": "HONOR",
        "device_model": "DUA-L22",
        "emotion": "neutral",
        "firmware": "mt6739",
        "format": "audio/opus",
        "header": {},
        "key": "cc96633d-59d4-4724-94bd-f5db2f02ad13",
        "lang": "ru-RU",
        "network_type": "WIFI::CONNECTED",
        "oauth_token": "AgAAAAA7SKUZAAJ-ISN********************",
        "platform_info": "android",
        "ps_activation_model": "ru-RU-activation-mobile-alisa-1.1.1",
        "ps_interruption_model": "ru-RU-activation-mobile-alisa-1.1.1",
        "quality": "UltraHigh",
        "request": {
            "activation_type": "oknyx",
            "additional_options": {},
            "device_state": {},
            "event": {},
            "experiments": {
                "activation_search_redirect_experiment": "1",
                "afisha_poi_events": "1",
                "ambient_sound": "1",
            },
            "laas_region": {},
            "location": {},
        },
        "speechkitVersion": "4.2.0",
        "speed": "1",
        "text": "Я думаю, примерно тут: посёлок городского типа Усть-Уда, Юбилейная улица 3.",
        "topic": "dialogeneral+dialog-general-gpu",
        "uuid": "cfae432c106042dd90c91a30c4640f0e",
        "vins": {},
        "voice": "shitova.us",
        "yandexuid": "3995517211576052263"
    }
})


@run_async()
async def test_log_directive_without_filter():
    with SessionLogMock() as log:
        logger = SessionLogger(session_id="the-best-session-id", rt_log=None)
        await log.pop_record()  # skip InitEvent

        logger.log_directive(VINS_RESPONSE)
        rec = await log.pop_record()
        # record is equal but not the same (i.e. deepcopied)
        assert rec["Directive"] == VINS_RESPONSE
        assert rec["Directive"] is not VINS_RESPONSE
        assert rec["Directive"]["directive"]["header"] is not VINS_RESPONSE["directive"]["header"]
        assert rec["Directive"]["directive"]["payload"] is not VINS_RESPONSE["directive"]["payload"]

        logger.close()


@run_async()
async def test_log_messages_with_vins_sensitive_data_filter():
    with SessionLogMock() as log:
        logger = SessionLogger(session_id="the-worst-session-id", rt_log=None)
        await log.pop_record()  # skip InitEvent

        # --- enable filtering
        logger.set_message_filter(log_filter.VinsSensitiveDataFilter)

        # VinsRequest
        logger.log_directive(VINS_REQUEST)
        rec = await log.pop_record()
        assert rec["Directive"] == {
            "type": "VinsRequest",
            "ForEvent": VINS_REQUEST["ForEvent"],
            "Body": {
                "application": VINS_REQUEST["Body"]["application"],
                "header": VINS_REQUEST["Body"]["header"],
                "request": {
                    "event": {
                        "biometry_scoring": VINS_REQUEST["Body"]["request"]["event"]["biometry_scoring"]
                    },
                    "device_state": {
                        "device_id": VINS_REQUEST["Body"]["request"]["device_state"]["device_id"]
                    }
                }
            }
        }
        assert rec["Directive"]["Body"]["request"] is not VINS_REQUEST["Body"]["request"]

        # Vins.VinsResponse
        logger.log_directive(VINS_RESPONSE)
        rec = await log.pop_record()
        assert rec["Directive"] == {
            "directive": {
                "header": VINS_RESPONSE["directive"]["header"],
                "payload": {
                    "header": VINS_RESPONSE["directive"]["payload"]["header"]
                }
            }
        }
        assert rec["Directive"]["directive"]["payload"] is not VINS_RESPONSE["directive"]["payload"]

        # TTS.Generate
        logger.log_event(TTS_GENERATE)
        rec = await log.pop_record()
        ref = copy.deepcopy(TTS_GENERATE.create_message())
        del ref["event"]["payload"]["text"]
        assert rec["Event"] == ref

        # --- disable filtering
        logger.set_message_filter(None)

        # VinsRequest
        logger.log_directive(VINS_REQUEST)
        rec = await log.pop_record()
        assert rec["Directive"] == VINS_REQUEST
        assert rec["Directive"]["Body"]["request"] is not VINS_REQUEST["Body"]["request"]

        # VinsResponse
        logger.log_directive(VINS_RESPONSE)
        rec = await log.pop_record()
        assert rec["Directive"] == VINS_RESPONSE
        assert rec["Directive"]["directive"]["payload"] is not VINS_RESPONSE["directive"]["payload"]

        # TTS.Generate
        logger.log_event(TTS_GENERATE)
        rec = await log.pop_record()
        assert rec["Event"] == TTS_GENERATE.create_message()

        logger.close()
