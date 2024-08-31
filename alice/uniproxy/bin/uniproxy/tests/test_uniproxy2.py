from .common import UniProxyProcess, Events
from .run import run_async
import tornado.gen
import tornado.httpclient
import tornado.ioloop
import json
import datetime
import pytest
import logging
from uuid import uuid4

from alice.uniproxy.library.testing.checks import match
from alice.cuttlefish.library.protos.session_pb2 import (
    TConnectionInfo,
    TExperimentsContext,
    TFlagsJsonData,
    TSessionContext,
    TUserInfo,
)


@pytest.fixture(scope="module")
def uniproxy():
    with UniProxyProcess() as x:
        yield x


async def read_json(ws, timeout=3):
    resp = await tornado.gen.with_timeout(datetime.timedelta(seconds=timeout), ws.read_message())
    logging.debug(f"Got response: {resp}")
    return json.loads(resp)


async def write_json(ws, msg):
    msg = json.dumps(msg)
    await ws.write_message(msg)
    logging.debug(f"Sent message: {msg}")


@run_async
async def test_apply_session_context(uniproxy):
    prev_unistat = await uniproxy.get_unistat()

    AUTH_TOKEN = "developers-simple-key"

    ws_conn = await uniproxy.ws_connect()

    # System.SynchronizeState
    await ws_conn.write_message(json.dumps(Events.System.SynchronizeState(auth_token=AUTH_TOKEN, uniproxy2={})))

    response = await read_json(ws_conn)
    assert match(response, {
        "directive": {
            "header": {"namespace": "Uniproxy2", "name": "UpdateUserSession"},
            "payload": {}
        }
    })

    response = await read_json(ws_conn)
    assert match(response, {
        "directive": {
            "header": {
                "namespace": "Uniproxy2",
                "name": "EventProcessorFinished"
            },
            "payload": {
                "event_full_name": "system.synchronizestate",
            }
        }
    })

    # Uniproxy2.ApplySessionContext
    await ws_conn.write_message(json.dumps(Events.Uniproxy2.ApplySessionContext(payload={
        "AppToken": AUTH_TOKEN,
        "User": {
            "AuthToken": "OAuth MyOAuthToken"
        }
    })))

    response = await read_json(ws_conn)
    assert match(response, {
        "directive": {
            "header": {
                "namespace": "Uniproxy2",
                "name": "EventProcessorFinished"
            },
            "payload": {
                "event_full_name": "uniproxy2.applysessioncontext",
            }
        }
    })

    unistat = await uniproxy.get_unistat()

    def unistat_diff(key):
        return unistat[key] - prev_unistat[key]

    assert unistat_diff("u2_apply_session_context_summ") == 1
    assert unistat_diff("u2_asc_diff_app_token_summ") == 0
    assert unistat_diff("u2_asc_diff_oauth_token_summ") == 1
    assert unistat_diff("u2_asc_nodiff_summ") == 0


@run_async
async def test_double_synchronize_state(uniproxy):
    await uniproxy.get_unistat()

    AUTH_TOKEN = "developers-simple-key"

    ws_conn = await uniproxy.ws_connect()

    # send first System.SynchronizeState
    await ws_conn.write_message(json.dumps(Events.System.SynchronizeState(auth_token=AUTH_TOKEN, uniproxy2={})))

    response = await read_json(ws_conn)
    response = await read_json(ws_conn)
    assert match(response, {"directive": {
        "header": {"namespace": "Uniproxy2", "name": "EventProcessorFinished"},
        "payload": {"event_full_name": "system.synchronizestate"}
    }})

    # send one more System.SynchronizeState
    await ws_conn.write_message(json.dumps(Events.System.SynchronizeState(auth_token=AUTH_TOKEN, uniproxy2={})))

    response = await read_json(ws_conn)
    response = await read_json(ws_conn)
    assert match(response, {"directive": {
        "header": {"namespace": "Uniproxy2", "name": "EventProcessorFinished"},
        "payload": {"event_full_name": "system.synchronizestate"}
    }})


@run_async
async def test_bad_system_set_state(uniproxy):
    user = await uniproxy.ws_connect()

    message_id = str(uuid4())

    await write_json(user, Events.System.SetState(
        message_id=message_id,
        session_context=TSessionContext(),
        original_payload={
            "uniproxy2": {}
        }
    ))

    resp = await read_json(user)
    assert match(resp, {
        "directive": {
            "header": {
                "namespace": "System",
                "name": "EventException",
                "refMessageId": message_id
            },
            "payload": {
                "error": {
                    "type": "Error",
                    "message": "session context is empty"
                }
            }
        }
    })

    resp = await read_json(user)
    assert match(resp, {
        "directive": {
            "header": {
                "namespace": "Uniproxy2",
                "name": "EventProcessorFinished",
                "refMessageId": message_id
            },
            "payload": {
                "event_full_name": "system.setstate"
            }
        }
    })


@run_async
async def test_good_system_set_state(uniproxy):
    user = await uniproxy.ws_connect()

    message_id = str(uuid4())

    await write_json(user, Events.System.SetState(
        message_id=message_id,
        session_context=TSessionContext(
            SessionId="ffffffff-ffff-ffff-ffff-fffffffffffa",
            InitialMessageId=message_id,
            ConnectionInfo=TConnectionInfo(
                IpAddress="10.1.2.3"
            ),
            AppToken="ffffffff-ffff-ffff-ffff-fffffffffffc",
            AppId="ru.yandex.uniproxy.test",
            Language="ru-RU",
            UserInfo=TUserInfo(
                Uuid="12345678-1234-1234-1234-123456789abc",
                Yuid="123123123",
                Guid="87654321-1234-1234-1234-123456789abc",
                Puid="123000321",
                AuthToken="THIS-IS-TOKEN",
                AuthTokenType=TUserInfo.ETokenType.OAUTH,
                Cookie="yandexuid=123123123",
                ICookie="333222111",
                StaffLogin="robot-voicetechbugs",
                LaasRegion=json.dumps({"region_id": 104357, "precision": 3,
                                       "latitude": 60.166892, "longitude": 24.943592})
            ),
            Experiments=TExperimentsContext(
                FlagsJsonData=TFlagsJsonData(
                    Data=json.dumps({
                        "flags_json_version": "1623",
                        "all": {
                            "CONTEXT": {
                                "MAIN": {
                                    "ASR": {
                                        "flags": [
                                            "EouDelay"
                                        ]
                                    },
                                    "VOICE": {
                                        "flags": [
                                            "dump_sessions_to_logs"
                                        ]
                                    }
                                }
                            },
                            "TESTID": [
                                "283725"
                            ]
                        },
                        "reqid": "1630684567589567-3481691349177570160-sas2-0737-afd-sas-l7-balancer-8080-BAL-7173",
                        "exphandler": "uniproxy",
                        "exp_config_version": "1026211",
                        "is_prestable": 1,
                        "version_token": "CAIQwuakjro",
                        "ids": [
                            "283725"
                        ],
                        "exp_boxes": "283725,0,-1",
                        "exp_boxes_crypted": "8lO_PlEheM1SyMURRhUwGQ,,"
                    })
                )
            )
        ),
        original_payload={
            "uniproxy2": {}
        }
    ))

    resp = await read_json(user)
    assert match(resp, {
        "directive": {
            "header": {
                "namespace": "Uniproxy2",
                "name": "UpdateUserSession",
                "refMessageId": message_id
            },
            "payload": {
                "do_not_use_user_logs": False
            }
        }
    })

    resp = await read_json(user)
    assert match(resp, {
        "directive": {
            "header": {
                "namespace": "Uniproxy2",
                "name": "EventProcessorFinished",
                "refMessageId": message_id
            },
            "payload": {
                "event_full_name": "system.setstate"
            }
        }
    })
