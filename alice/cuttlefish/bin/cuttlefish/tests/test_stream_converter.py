import logging
import json
from uuid import uuid4
import pytest
from .common import Cuttlefish
from google.protobuf.json_format import MessageToDict
from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient
from alice.cachalot.api.protos.cachalot_pb2 import TMegamindSessionRequest
from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadSmarthomeUid
from alice.cuttlefish.library.protos.megamind_pb2 import TMegamindRequest
from alice.cuttlefish.library.protos.session_pb2 import (
    TAbFlagsProviderOptions,
    TRequestContext,
    TSessionContext,
    TUserInfo,
)
from alice.cuttlefish.library.python.testing import checks
from alice.cuttlefish.library.python.testing.constants import ItemTypes
from alice.cuttlefish.library.protos.wsevent_pb2 import TEventHeader, TWsEvent
from alice.megamind.protos.guest.enrollment_headers_pb2 import TEnrollmentHeaders
from library.python import resource


@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def make_text_input(message_id=None, uuid=None):
    evt = {
        "event": {
            "header": {
                "namespace": "Vins",
                "name": "TextInput",
                "messageId": str(uuid4()) if (message_id is None) else message_id,
            },
            "payload": {
                "header": {
                    "dialog_id": "my-dialog-id",
                    "request_id": "my-request-id",
                    "prev_req_id": "my-previous-request-id",
                },
                "vins": {"application": {"uuid": str(uuid4()) if (uuid is None) else uuid}},
                "request": {
                    "additional_options": {"quasar_auxiliary_config": {"alice4business": {"smart_home_uid": "100500"}}},
                    "experiments": {
                        "only_100_percent_flags": "1",
                    },
                    "megamind_cookies": json.dumps({"uaas_tests": [333]}),
                    "uaas_tests": [111, "222"],
                },
            },
        }
    }

    return evt


def make_voice_input(message_id=None, enrollment_headers=None):
    voice_input = json.loads(resource.find("voice_input").decode("utf-8"))

    voice_input['event']['header']['message_id'] = message_id
    voice_input['event']['payload']['request']['enrollment_headers'] = enrollment_headers

    return voice_input


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
async def test_ws_stream_to_proto__text_input(cuttlefish: Cuttlefish):
    msgid = str(uuid4())
    uuid = str(uuid4())
    client = AppHostGrpcClient(cuttlefish.grpc_endpoint)
    response = None

    async with client.create_stream("/stream_raw_to_protobuf") as stream:
        stream.write_items(
            items={
                "session_context": [
                    TSessionContext(
                        AppToken="51ae06cc-5c8f-48dc-93ae-7214517679e6",  # quasar's auth_token
                        UserInfo=TUserInfo(
                            Uuid=uuid,
                        ),
                    ),
                ],
                "ws_message": [
                    TWsEvent(
                        Header=TEventHeader(
                            Namespace=TEventHeader.EMessageNamespace.VINS,
                            Name=TEventHeader.EMessageName.TEXT_INPUT,
                            MessageId=msgid,
                        ),
                        Text=json.dumps(make_text_input(message_id=msgid, uuid=uuid)),
                    ),
                ],
            }
        )
        response = await stream.read(timeout=1)

    request_context = response.get_only_item(item_type="request_context", proto_type=TRequestContext)
    logging.debug(f"request_context: {request_context.data}")
    assert checks.match(
        MessageToDict(request_context.data), {"Header": {"SessionId": "", "MessageId": msgid}, "ExpFlags": {}}
    )

    mm_request = response.get_only_item(item_type=ItemTypes.MEGAMIND_REQUEST, proto_type=TMegamindRequest)
    logging.debug(f"mm_request: {mm_request}")
    logging.debug("mm_request:json: {}".format(MessageToDict(mm_request.data)))
    assert checks.match(
        MessageToDict(mm_request.data),
        {
            "RequestBase": {
                "header": {
                    "dialog_id": "my-dialog-id",
                    "request_id": "my-request-id",
                    "prev_req_id": "my-previous-request-id",
                },
                "request": {
                    "additional_options": {"quasar_auxiliary_config": {"alice4business": {"smart_home_uid": '100500'}}}
                },
            },
        },
    )

    mm_session_request = response.get_only_item(item_type="mm_session_request", proto_type=TMegamindSessionRequest)
    logging.debug(f"mm_session_request: {mm_session_request.data}")
    assert checks.match(
        MessageToDict(mm_session_request.data),
        {"LoadRequest": {"Uuid": uuid, "DialogId": "my-dialog-id", "RequestId": "my-previous-request-id"}},
    )

    smarthome_uid = response.get_only_item(item_type="smarthome_uid", proto_type=TContextLoadSmarthomeUid)
    logging.debug(f"smarthome_uid: {smarthome_uid.data}")
    assert checks.match(MessageToDict(smarthome_uid.data), {"Value": "100500"})

    ab_experiments_options = response.get_only_item_data(
        item_type="ab_experiments_options", proto_type=TAbFlagsProviderOptions
    )
    logging.debug(f"ab_experiments_options: {ab_experiments_options}")
    assert not ab_experiments_options.DisregardUaas
    assert ab_experiments_options.Only100PercentFlags
    assert ab_experiments_options.TestIds == ["111", "222", "333"]


@pytest.mark.asyncio
async def test_ws_stream_to_proto__text_input_with_guest_options(cuttlefish: Cuttlefish):
    msgid = str(uuid4())
    uuid = str(uuid4())
    client = AppHostGrpcClient(cuttlefish.grpc_endpoint)
    response = None

    message = make_text_input(message_id=msgid, uuid=uuid)
    message['event']['payload']['request']['additional_options']['guest_user_options'] = {
        "pers_id": "12345",
        "status": "Match",
        "oauth_token": "my_token",
    }

    async with client.create_stream("/stream_raw_to_protobuf") as stream:
        stream.write_items(
            items={
                "session_context": [
                    TSessionContext(
                        AppToken="51ae06cc-5c8f-48dc-93ae-7214517679e6",  # quasar's auth_token
                        UserInfo=TUserInfo(
                            Uuid=uuid,
                        ),
                    ),
                ],
                "ws_message": [
                    TWsEvent(
                        Header=TEventHeader(
                            Namespace=TEventHeader.EMessageNamespace.VINS,
                            Name=TEventHeader.EMessageName.TEXT_INPUT,
                            MessageId=msgid,
                        ),
                        Text=json.dumps(message),
                    ),
                ],
            }
        )
        response = await stream.read(timeout=1)

    mm_request = response.get_only_item(item_type=ItemTypes.MEGAMIND_REQUEST, proto_type=TMegamindRequest)
    logging.debug(f"mm_request: {mm_request}")
    logging.debug("mm_request:json: {}".format(MessageToDict(mm_request.data)))
    assert checks.match(
        MessageToDict(mm_request.data)["RequestBase"]["request"]["additional_options"]["guest_user_options"],
        {
            "pers_id": "12345",
            "status": "Match",
            "oauth_token": "my_token",
        },
    )

    assert len(list(response.get_items(item_type="guest_blackbox_http_request"))) == 1


@pytest.mark.asyncio
async def test_ws_stream_to_proto__text_input_without_guest_oauth_token(cuttlefish: Cuttlefish):
    msgid = str(uuid4())
    uuid = str(uuid4())
    client = AppHostGrpcClient(cuttlefish.grpc_endpoint)
    response = None

    message = make_text_input(message_id=msgid, uuid=uuid)
    message['event']['payload']['request']['additional_options']['guest_user_options'] = {
        "pers_id": "12345",
        "status": "Match",
    }

    async with client.create_stream("/stream_raw_to_protobuf") as stream:
        stream.write_items(
            items={
                "session_context": [
                    TSessionContext(
                        AppToken="51ae06cc-5c8f-48dc-93ae-7214517679e6",  # quasar's auth_token
                        UserInfo=TUserInfo(
                            Uuid=uuid,
                        ),
                    ),
                ],
                "ws_message": [
                    TWsEvent(
                        Header=TEventHeader(
                            Namespace=TEventHeader.EMessageNamespace.VINS,
                            Name=TEventHeader.EMessageName.TEXT_INPUT,
                            MessageId=msgid,
                        ),
                        Text=json.dumps(message),
                    ),
                ],
            }
        )
        response = await stream.read(timeout=1)

    mm_request = response.get_only_item(item_type=ItemTypes.MEGAMIND_REQUEST, proto_type=TMegamindRequest)
    logging.debug(f"mm_request: {mm_request}")
    logging.debug("mm_request:json: {}".format(MessageToDict(mm_request.data)))
    assert checks.match(
        MessageToDict(mm_request.data)["RequestBase"]["request"]["additional_options"]["guest_user_options"],
        {
            "pers_id": "12345",
            "status": "Match",
        },
    )

    assert len(list(response.get_items(item_type="guest_blackbox_http_request"))) == 0


# TODO @aradzevich (VOICESERV-4454): Remove test when guest_user_options moved into additional_options
@pytest.mark.asyncio
async def test_ws_stream_to_proto__text_input_with_guest_options_2(cuttlefish: Cuttlefish):
    msgid = str(uuid4())
    uuid = str(uuid4())
    client = AppHostGrpcClient(cuttlefish.grpc_endpoint)
    response = None

    message = make_text_input(message_id=msgid, uuid=uuid)
    message['event']['payload']['request']['guest_user_options'] = {
        "pers_id": "12345",
        "status": "Match",
        "oauth_token": "my_token",
    }

    async with client.create_stream("/stream_raw_to_protobuf") as stream:
        stream.write_items(
            items={
                "session_context": [
                    TSessionContext(
                        AppToken="51ae06cc-5c8f-48dc-93ae-7214517679e6",  # quasar's auth_token
                        UserInfo=TUserInfo(
                            Uuid=uuid,
                        ),
                    ),
                ],
                "ws_message": [
                    TWsEvent(
                        Header=TEventHeader(
                            Namespace=TEventHeader.EMessageNamespace.VINS,
                            Name=TEventHeader.EMessageName.TEXT_INPUT,
                            MessageId=msgid,
                        ),
                        Text=json.dumps(message),
                    ),
                ],
            }
        )
        response = await stream.read(timeout=1)

    mm_request = response.get_only_item(item_type=ItemTypes.MEGAMIND_REQUEST, proto_type=TMegamindRequest)
    logging.debug(f"mm_request: {mm_request}")
    logging.debug("mm_request:json: {}".format(MessageToDict(mm_request.data)))
    assert checks.match(
        MessageToDict(mm_request.data)["RequestBase"]["request"]["additional_options"]["guest_user_options"],
        {
            "pers_id": "12345",
            "status": "Match",
            "oauth_token": "my_token",
        },
    )

    assert len(list(response.get_items(item_type="guest_blackbox_http_request"))) == 1


# -------------------------------------------------------------------------------------------------
@pytest.mark.asyncio
@pytest.mark.parametrize(
    "enrollment_headers_from_client, expected_enrollment_headers",
    [
        (None, None),
        ([], None),
        (
            {
                "headers": [
                    {
                        "person_id": "PersId-6821a013-3e50338d-8e23bff5-249ac1e3",
                        "version": "speaker-0.1.3",
                        "user_type": "OWNER",
                    }
                ]
            },
            {
                "headers": [
                    {
                        "person_id": "PersId-6821a013-3e50338d-8e23bff5-249ac1e3",
                        "version": "speaker-0.1.3",
                        "user_type": "OWNER",
                    }
                ]
            },
        ),
        (
            {
                "headers": [
                    {
                        "person_id": "PersId-6821a013-3e50338d-8e23bff5-249ac1e3",
                        "version": "speaker-0.1.3",
                        "user_type": "GUEST",
                    }
                ]
            },
            {
                "headers": [
                    {
                        "person_id": "PersId-6821a013-3e50338d-8e23bff5-249ac1e3",
                        "version": "speaker-0.1.3",
                        "user_type": "GUEST",
                    }
                ]
            },
        ),
        (
            {"headers": [{"person_id": "", "version": "", "user_type": "GUEST"}]},
            {"headers": [{"person_id": "", "version": "", "user_type": "GUEST"}]},
        ),
    ],
)
async def test_ws_stream_to_proto__voice_input_enrollment_headers(
    cuttlefish: Cuttlefish, enrollment_headers_from_client, expected_enrollment_headers
):
    msgid = str(uuid4())
    uuid = str(uuid4())
    client = AppHostGrpcClient(cuttlefish.grpc_endpoint)
    response = None

    async with client.create_stream("/stream_raw_to_protobuf") as stream:
        stream.write_items(
            items={
                "session_context": [
                    TSessionContext(
                        AppToken="51ae06cc-5c8f-48dc-93ae-7214517679e6",  # quasar's auth_token
                        UserInfo=TUserInfo(
                            Uuid=uuid,
                        ),
                    ),
                ],
                "ws_message": [
                    TWsEvent(
                        Header=TEventHeader(
                            Namespace=TEventHeader.EMessageNamespace.VINS,
                            Name=TEventHeader.EMessageName.VOICE_INPUT,
                            MessageId=msgid,
                        ),
                        Text=json.dumps(make_voice_input(msgid, enrollment_headers=enrollment_headers_from_client)),
                    ),
                ],
            }
        )
        response = await stream.read(timeout=1)

    if expected_enrollment_headers:
        enrollment_headers = response.get_only_item(
            item_type=ItemTypes.ENROLLMENT_HEADERS, proto_type=TEnrollmentHeaders
        )
        logging.debug(f"enrollment_headers: {enrollment_headers}")
        logging.debug(f"enrollment_headers:json: {enrollment_headers.data}")
        assert checks.match(
            MessageToDict(enrollment_headers.data, including_default_value_fields=True), expected_enrollment_headers
        )
    else:
        assert len(list(response.get_items(item_type="enrollment_headers"))) == 0
