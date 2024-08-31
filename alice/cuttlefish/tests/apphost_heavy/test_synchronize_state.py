import pytest
from alice.cuttlefish.library.python.test_utils import messages, checks
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext
from .common import connection


@pytest.mark.asyncio
async def test_simple_synchronize_state(uniproxy2, uniproxy_mock):
    async with connection(uniproxy2, uniproxy_mock) as (user, uniproxy):
        await user.write(messages.SynchronizeState())

        msg = await uniproxy.read(timeout=2)
        assert checks.match(
            msg, {"event": {"header": {"namespace": "System", "name": "SetState"}, "payload": {}}}  # TODO: check more
        )

        msg_id = msg["event"]["header"]["messageId"]
        await uniproxy.write(
            {
                "directive": {
                    "header": {"namespace": "Uniproxy2", "name": "UpdateUserSession", "refMessageId": msg_id},
                    "payload": {"do_not_use_user_logs": False},
                }
            }
        )
        await uniproxy.write(messages.EventProcessorFinishedFor(msg))

        assert not (await user.read_some())


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
async def test_synchronize_state_with_oauth(uniproxy2, uniproxy_mock, mocks):
    async with connection(uniproxy2, uniproxy_mock) as (user, uniproxy):
        await user.write(
            messages.SynchronizeState(
                payload={"oauth_token": "PERFECT-OAUTH-TOKEN-FOR-STAFF", "uuid": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"}
            )
        )

        msg = await uniproxy.read(timeout=2)
        assert checks.match(
            msg,
            {
                "event": {
                    "header": {"namespace": "System", "name": "SetState"},
                    "payload": {
                        "original_payload": {},
                        "session_context": checks.Base64Decode(
                            checks.ParseProtobufToDict(
                                TSessionContext,
                                {
                                    "UserInfo": {
                                        "Puid": "00000000",
                                    }
                                },
                            )
                        ),
                    },
                }
            },
        )

        msg_id = msg["event"]["header"]["messageId"]
        await uniproxy.write(
            {
                "directive": {
                    "header": {"namespace": "Uniproxy2", "name": "UpdateUserSession", "refMessageId": msg_id},
                    "payload": {"do_not_use_user_logs": False},
                }
            }
        )
        await uniproxy.write(messages.EventProcessorFinishedFor(msg))

        assert not (await user.read_some())

    # check preformed requests
    assert checks.match(
        mocks.records,
        {
            "BLACKBOX__VOICE": checks.ListMatcher(
                [
                    {
                        "request": {
                            "method": "GET",
                            "uri": "/blackbox?method=oauth&format=json&get_user_ticket=yes&oauth_token=PERFECT-OAUTH-TOKEN-FOR-STAFF&userip=127.0.0.1&attributes=8&aliases=13",
                        }
                    }
                ],
                exact_length=True,
            ),
        },
    )


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
async def test_synchronize_state_without_oauth(uniproxy2, uniproxy_mock, mocks):
    uuid = "01234567-89ab-cdef-0123-456789abcdef"

    async with connection(uniproxy2, uniproxy_mock) as (user, uniproxy):
        await user.write(messages.SynchronizeState(payload={"uuid": uuid}))

        msg = await uniproxy.read(timeout=2)
        assert checks.match(
            msg,
            {
                "event": {
                    "header": {"namespace": "System", "name": "SetState"},
                    "payload": {
                        "original_payload": {},
                        "session_context": checks.Base64Decode(
                            checks.ParseProtobufToDict(
                                TSessionContext,
                                {
                                    "UserInfo": {
                                        "Puid": None,
                                    }
                                },
                            )
                        ),
                    },
                }
            },
        )

        msg_id = msg["event"]["header"]["messageId"]
        await uniproxy.write(
            {
                "directive": {
                    "header": {"namespace": "Uniproxy2", "name": "UpdateUserSession", "refMessageId": msg_id},
                    "payload": {"do_not_use_user_logs": False},
                }
            }
        )
        await uniproxy.write(messages.EventProcessorFinishedFor(msg))

        assert not (await user.read_some())

    # check preformed requests
    assert checks.match(
        mocks.records,
        {
            "BLACKBOX__VOICE": None,
        },
    )
