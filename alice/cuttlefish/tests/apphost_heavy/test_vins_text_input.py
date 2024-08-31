import pytest
import uuid
import time
import asyncio
from .common import initialized_connection

# from .utils.tts import process_tts_generate_in_uniproxy
# from google.protobuf.json_format import ParseDict
from alice.cuttlefish.library.python.test_utils import checks, deepupdate


MSGID = str(uuid.uuid4())
UUID = str(uuid.uuid4())
SYNCHRONIZE_STATE_PAYLOAD = {
    "uuid": UUID,
    "vins": {
        "application": {
            "uuid": UUID,
            "device_manufacturer": "Ivan Inc",
            "device_model": "ivan-ivanovich",
            "device_id": "001",
            "platform": "android",
            "os_version": "4.2",
            "app_id": "my-shiny-application",
            "app_version": "0.1",
        }
    },
}


def make_text_input(smart_home_uid=None, apply=False, voice_session=None, message_id=None):
    payload = {
        "header": {"dialog_id": "my-dialog-id", "request_id": "my-request-id", "prev_req_id": "my-previous-request-id"},
        "request": {"voice_session": False, "event": {"name": "", "type": "text_input", "text": "NO-APPLY"}},
        "vins": {"application": {"uuid": UUID}},
    }

    if apply:
        deepupdate(payload, {"request": {"event": {"text": "APPLY"}}})

    if smart_home_uid is not None:
        deepupdate(
            payload,
            {
                "request": {
                    "additional_options": {
                        "quasar_auxiliary_config": {"alice4business": {"smart_home_uid": smart_home_uid}}
                    }
                }
            },
        )

    if voice_session is not None:
        deepupdate(payload, {"request": {"voice_session": voice_session}})

    return {
        "event": {
            "header": {
                "namespace": "Vins",
                "name": "TextInput",
                "messageId": MSGID if (message_id is None) else message_id,
            },
            "payload": payload,
        }
    }


EXPECTED_APPHOST_LOGS = [
    # -----------------------------------------------------------------------------
    # /context_load
    (
        "TSourceOutputMetadata",
        {"Source": "INIT", "ItemMetas": checks.Contains(":session_context", ":mm_session_request")},
    ),
    # CONTEXT_LOAD_POST
    (
        "TSourceInputMetadata",
        {
            "Source": "CONTEXT_LOAD_POST",
            "ItemMetas": checks.Contains("INIT:session_context", "CACHALOT_MM_SESSION:mm_session_response"),
        },
    ),
    ("TSourceOutputMetadata", {"Source": "CONTEXT_LOAD_POST", "ItemMetas": [":context_load_response"]}),
    ("TSourceSuccess", {"Source": "CONTEXT_LOAD_POST"}),
    # -----------------------------------------------------------------------------
    # /text_input
    # CONTEXT_LOAD
    (
        "TSourceInputMetadata",
        {"Source": "CONTEXT_LOAD", "ItemMetas": checks.Contains("INIT:session_context", "INIT:mm_session_request")},
    ),
    ("TSourceOutputMetadata", {"Source": "CONTEXT_LOAD", "ItemMetas": [":context_load_response"]}),
    ("TSourceSuccess", {"Source": "CONTEXT_LOAD"}),
    # MEGAMIND_RUN
    (
        "TSourceInputMetadata",
        {
            "Source": "MEGAMIND_RUN",
            "ItemMetas": checks.Contains(
                "MEGAMIND_RUN_FIRST_CHUNK:request_context", "MEGAMIND_RUN_FIRST_CHUNK:session_context"
            ),
        },
    ),
    ("TSourceInputMetadata", {"Source": "MEGAMIND_RUN", "ItemMetas": ["CONTEXT_LOAD:context_load_response"]}),
    (
        "TSourceOutputMetadata",
        {"Source": "MEGAMIND_RUN", "ItemMetas": checks.Contains(":mm_session_request", ":mm_response")},
    ),
    ("TSourceSuccess", {"Source": "MEGAMIND_RUN"}),
    # CONTEXT_SAVE
    (
        "TSourceInputMetadata",
        {
            "Source": "CONTEXT_SAVE",
            "ItemMetas": checks.Contains(
                "INIT:session_context", "INIT:request_context", "INIT:mm_session_request", "INIT:tvm_user_ticket"
            ),
        },
    ),
    ("TSourceOutputMetadata", {"Source": "CONTEXT_SAVE", "ItemMetas": checks.Contains(":context_save_response")}),
    ("TSourceSuccess", {"Source": "CONTEXT_SAVE"}),
    # WS_ADAPTER_OUT
    (
        "TSourceInputMetadata",
        {
            "Source": "WS_ADAPTER_OUT",
            "ItemMetas": checks.Contains("WS_ADAPTER_IN:request_context", "INIT:session_context"),
        },
    ),
    (
        "TSourceInputMetadata",
        {
            "Source": "WS_ADAPTER_OUT",
            "ItemMetas": checks.Contains(
                "WAIT_FINAL_RESPONSE_PARTS:mm_response", "WAIT_FINAL_RESPONSE_PARTS:context_save_response"
            ),
        },
    ),
    (
        "TSourceOutputMetadata",
        {"Source": "WS_ADAPTER_OUT", "ItemMetas": checks.Contains(":ws_message", ":uniproxy2_directive")},
    ),
    ("TSourceSuccess", {"Source": "WS_ADAPTER_OUT"}),
]


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
async def test_vins_text_input_without_smarthome_uid(mocks, uniproxy2, uniproxy_mock, apphost_evlogdump):
    start_time_us = int(time.time_ns() / 1000)

    async with uniproxy2.settings_patch(apphosted_vins_text_input=True):
        async with initialized_connection(uniproxy2, uniproxy_mock, payload=SYNCHRONIZE_STATE_PAYLOAD) as (
            user,
            uniproxy,
        ):
            await user.write(make_text_input())
            assert checks.match(
                await user.read(),
                {
                    "directive": {
                        "header": {"namespace": "Vins", "name": "VinsResponse", "refMessageId": MSGID},
                        "payload": {},
                    }
                },
            )

            await asyncio.sleep(1)

            # check that the request is really finished
            assert checks.match(
                await uniproxy2.get_unistat(),
                {
                    "event_processor_count_ammx": 0,
                    "request_count_ammx": 0,
                    "ah_stream_count_ammx": 0,
                },
            )

            # -------------------------------------------------------------------------------------
            # CHECK LOGS
            apphost_evlogs = await apphost_evlogdump.get_json_all(start_time=start_time_us)
            assert checks.match(apphost_evlogs, checks.EvlogRecords(EXPECTED_APPHOST_LOGS, ignore_order=True))

    assert checks.match(
        mocks.records,
        {
            "CUTTLEFISH_MEGAMIND_URL": checks.ListMatcher(
                [
                    {
                        "request": {
                            "method": "POST",
                            "uri": "/speechkit/app/pa/",
                            "body": checks.ParseJson(
                                {
                                    "application": {
                                        "app_id": "my-shiny-application",
                                        "app_version": "0.1",
                                        "device_id": "001",
                                        "device_manufacturer": "Ivan Inc",
                                        "device_model": "ivan-ivanovich",
                                        "os_version": "4.2",
                                        "platform": "android",
                                        "uuid": UUID,
                                    }
                                }
                            ),
                        }
                    }
                ],
                exact_length=True,
            ),
            "VOICE__DATASYNC": checks.ListMatcher(
                [
                    # context load
                    {
                        "request": {
                            "method": "POST",
                            "uri": "/v1/batch/request",
                            "headers": [["x-uid", f"device_id:{UUID}"]],
                            "body": checks.ParseJson(
                                {
                                    "items": [
                                        {"method": "GET", "relative_url": "/v2/personality/profile/addresses"},
                                        {"method": "GET", "relative_url": "/v1/personality/profile/alisa/kv"},
                                    ]
                                }
                            ),
                        }
                    },
                    {
                        "request": {
                            "method": "POST",
                            "uri": "/v1/batch/request",
                            "headers": [["x-uid", f"uuid:{UUID}"]],
                            "body": checks.ParseJson(
                                {
                                    "items": [
                                        {"method": "GET", "relative_url": "/v2/personality/profile/addresses"},
                                        {"method": "GET", "relative_url": "/v1/personality/profile/alisa/kv"},
                                    ]
                                }
                            ),
                        }
                    },
                    # context save
                    {
                        "request": {
                            "method": "POST",
                            "uri": "/v1/batch/request",
                            "headers": [["x-uid", f"device_id:{UUID}"]],
                            "body": checks.ParseJson(
                                {
                                    "items": [
                                        {
                                            "method": "PUT",
                                            "relative_url": "/v1/personality/profile/alisa/kv/proactivity_history",
                                        }
                                    ]
                                }
                            ),
                        }
                    },
                ],
                ignore_order=True,
            ),
        },
    )


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
async def test_vins_text_input_with_smarthome_uid(mocks, uniproxy2, uniproxy_mock, apphost_evlogdump):
    start_time_us = int(time.time_ns() / 1000)

    async with uniproxy2.settings_patch(apphosted_vins_text_input=True):
        ss_payload = {"uuid": UUID, "oauth_token": "PERFECT-OAUTH-TOKEN"}
        async with initialized_connection(uniproxy2, uniproxy_mock, payload=ss_payload) as (user, uniproxy):
            await user.write(make_text_input(smart_home_uid="100500"))
            assert checks.match(
                await user.read(),
                {
                    "directive": {
                        "header": {"namespace": "Vins", "name": "VinsResponse", "refMessageId": MSGID},
                        "payload": {},
                    }
                },
            )

            await asyncio.sleep(1)

            # check that the request is really finished
            assert checks.match(
                await uniproxy2.get_unistat(),
                {
                    "event_processor_count_ammx": 0,
                    "request_count_ammx": 0,
                    "ah_stream_count_ammx": 0,
                },
            )

            # -------------------------------------------------------------------------------------
            # CHECK LOGS
            apphost_evlogs = await apphost_evlogdump.get_json_all(start_time=start_time_us)
            assert checks.match(apphost_evlogs, checks.EvlogRecords(EXPECTED_APPHOST_LOGS, ignore_order=True))

    assert checks.match(
        mocks.records,
        {
            "CUTTLEFISH_MEGAMIND_URL": checks.ListMatcher(
                [
                    {
                        "request": {
                            "method": "POST",
                            "uri": "/speechkit/app/pa/",
                            "headers": [
                                ["content-type", "application/json"],
                                ["x-ya-user-ticket", "3:user:VALID-USER-TICKET"],
                            ],
                            "body": checks.ParseJson(
                                {
                                    "request": {
                                        "additional_options": {
                                            "oauth_token": "PERFECT-OAUTH-TOKEN",
                                            "puid": "00000000",
                                            "quasar_auxiliary_config": {"alice4business": {"smart_home_uid": "100500"}},
                                        },
                                        "laas_region": {},
                                        "smart_home": {  # content of `RawUserInfo` field from QUASAR_IOT's response
                                            "payload": {
                                                "rooms": [
                                                    {"name": "kitchen", "id": "1"},
                                                    {"name": "bedroom", "id": "2"},
                                                    {"name": "toilet", "id": "3"},
                                                ]
                                            }
                                        },
                                    }
                                }
                            ),
                        }
                    }
                ],
                exact_length=True,
            ),
            "VOICE__DATASYNC": checks.ListMatcher(
                [
                    # context load
                    {
                        "request": {
                            "method": "POST",
                            "uri": "/v1/batch/request",
                            "headers": [["x-ya-user-ticket", "3:user:VALID-USER-TICKET"]],
                            "body": checks.ParseJson(
                                {
                                    "items": [
                                        {"method": "GET", "relative_url": "/v2/personality/profile/addresses"},
                                        {"method": "GET", "relative_url": "/v1/personality/profile/alisa/kv"},
                                        {"method": "GET", "relative_url": "/v1/personality/profile/alisa/settings"},
                                    ]
                                }
                            ),
                        }
                    },
                    # context save
                    {
                        "request": {
                            "method": "POST",
                            "uri": "/v1/batch/request",
                            "headers": [["x-ya-user-ticket", "3:user:VALID-USER-TICKET"]],
                            "body": checks.ParseJson(
                                {
                                    "items": [
                                        {
                                            "method": "PUT",
                                            "relative_url": "/v1/personality/profile/alisa/kv/proactivity_history",
                                        }
                                    ]
                                }
                            ),
                        }
                    },
                ],
                ignore_order=True,
            ),
        },
    )


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
async def test_vins_text_input_with_pythonic_tts(mocks, uniproxy2, uniproxy_mock, apphost_evlogdump):
    # TODO: start_time_us = int(time.time_ns() / 1000)
    async with uniproxy2.settings_patch(apphosted_vins_text_input=True):
        async with initialized_connection(uniproxy2, uniproxy_mock) as (user, uniproxy):

            await user.write(make_text_input(voice_session=True))

            # User recevies Vins.VinsResponse (from AppHost)
            assert checks.match(
                await user.read(),
                {
                    "directive": {
                        "header": {"namespace": "Vins", "name": "VinsResponse", "refMessageId": MSGID},
                        "payload": {},
                    }
                },
            )

            # TODO: restore test (after make tts graph normally work)
            """
            # User receives TTS.Speak and audio stream (from pythonic uniproxy)
            # async with process_tts_generate_in_uniproxy(uniproxy, wait_nextchunk=False):
            directive = await user.read()  # TODO
            assert checks.match(directive, {
                "directive": {
                    "header": {
                        "namespace": "TTS",
                        "name": "Speak",
                        "refMessageId": MSGID
                    },
                    "payload": {}
                }
            })

            while True:
                msg = await user.read()
                if isinstance(msg, bytes):
                    continue
                assert checks.match(msg, {"streamcontrol": {}})  # TODO
                break

            # User receives nothing else...
            assert not (await user.read_some())

            # ...and request is finished
            assert checks.match(await uniproxy2.get_unistat(), {
                "event_processor_count_ammx": 0,
                "request_count_ammx": 0,
                "ah_stream_count_ammx": 0
            })

            # -------------------------------------------------------------------------------------
            # CHECK LOGS
            apphost_evlogs = await apphost_evlogdump.get_json_all(start_time=start_time_us)
            assert checks.match(apphost_evlogs, checks.EvlogRecords(EXPECTED_APPHOST_LOGS, ignore_order=True))
            """

    assert checks.match(
        mocks.records,
        {
            "CUTTLEFISH_MEGAMIND_URL": checks.ListMatcher(
                [{"request": {"method": "POST", "uri": "/speechkit/app/pa/"}}], exact_length=True
            )
        },
    )
