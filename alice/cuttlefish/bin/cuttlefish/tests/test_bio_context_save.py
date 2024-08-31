import json
import pytest
from .common import Cuttlefish, create_grpc_request
from alice.cuttlefish.library.protos.bio_context_save_pb2 import TBioContextSaveText, TBioContextSaveNewEnrolling
from alice.cachalot.api.protos.cachalot_pb2 import TYabioContextRequest, TYabioContextResponse, TYabioContextSuccess

# , TYabioContextError
from alice.cuttlefish.library.protos.context_save_pb2 import TContextSaveRequest
from alice.cuttlefish.library.protos.session_pb2 import TAudioOptions, TRequestContext
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TUserInfo, TDeviceInfo
from alice.cuttlefish.library.protos.store_audio_pb2 import TStoreAudioResponse
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.items import get_answer_item
from google.protobuf import json_format
from alice.megamind.protos.speechkit.directives_pb2 import TDirective as TSpeechkitDirective
from google.protobuf.struct_pb2 import Struct as google_protobuf_Struct
from voicetech.library.proto_api.yabio_pb2 import YabioContext, YabioUser, YabioVoiceprint


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def make_speechkit_directive(name, payload=None):
    directive = TSpeechkitDirective()
    directive.Name = name
    if payload:
        directive.Payload.MergeFrom(payload)
    return directive


class TestBioContextSavePre:

    DEFAULT_ITEMS = [
        {
            "type": ItemTypes.REQUEST_CONTEXT,
            "data": TRequestContext(
                AudioOptions=TAudioOptions(
                    Format="audio/opus",
                ),
                Header=TRequestContext.THeader(
                    SessionId="my-cool-session-id",
                    MessageId="my-cool-message-id",
                    StreamId=13,
                ),
            ),
        },
        {
            "type": ItemTypes.SESSION_CONTEXT,
            "data": TSessionContext(
                UserInfo=TUserInfo(
                    Uuid="my-cool-uuid",
                ),
                DeviceInfo=TDeviceInfo(
                    DeviceModel="my-cool-device-model",
                    DeviceManufacturer="my-cool-device-manuf",
                ),
            ),
        },
        {
            "type": ItemTypes.YABIO_CONTEXT,
            "data": YabioContext(
                group_id="my-cool-uuid",  # should be same as UUID
                users=[
                    YabioUser(
                        user_id="my-cool-user-id",
                    ),
                ],
                enrolling=[
                    YabioVoiceprint(
                        request_id="some-req-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="spotter",
                    ),
                ],
            ),
        },
    ]

    def test_nothing_changed(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.BIO_CONTEXT_SAVE_PRE, create_grpc_request(items=self.DEFAULT_ITEMS)
        )
        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.NOTHING)

    def test_mds_url_cache(self, cuttlefish: Cuttlefish):
        items = self.DEFAULT_ITEMS.copy()
        items[2] = {
            "type": ItemTypes.YABIO_CONTEXT,
            "data": YabioContext(
                group_id="my-cool-uuid",
                users=[
                    YabioUser(
                        user_id="my-cool-user-id",
                        voiceprints=[
                            YabioVoiceprint(
                                request_id="other-req-id",
                                compatibility_tag="some-tag",
                                format="some-format",
                                source="spotter",
                            ),
                            YabioVoiceprint(
                                request_id="my-cool-message-id",
                                compatibility_tag="some-tag",
                                format="some-format",
                                source="spotter",
                            ),
                        ],
                    ),
                ],
                enrolling=[
                    YabioVoiceprint(
                        request_id="other-req-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="my-cool-message-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="my-cool-message-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="request",
                        mds_url="old-mds-url",
                    ),
                    YabioVoiceprint(
                        request_id="other-other-req-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="spotter",
                    ),
                ],
            ),
        }

        items.append(
            {
                "type": ItemTypes.STORE_AUDIO_RESPONSE,
                "data": TStoreAudioResponse(
                    StatusCode=200,
                    IsSpotter=True,
                    Key="spotter.mp3",
                ),
            }
        )
        items.append(
            {
                "type": ItemTypes.STORE_AUDIO_RESPONSE,
                "data": TStoreAudioResponse(
                    StatusCode=200,
                    IsSpotter=False,
                    Key="stream.mp3",
                ),
            }
        )

        response = cuttlefish.make_grpc_request(ServiceHandles.BIO_CONTEXT_SAVE_PRE, create_grpc_request(items=items))
        assert len(response.Answers) == 1

        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT_REQUEST, proto=TYabioContextRequest)
        assert req.Save.Key.GroupId == "my-cool-uuid"
        assert req.Save.Key.DevModel == "my-cool-device-model"
        assert req.Save.Key.DevManuf == "my-cool-device-manuf"

        expected_context = items[2]["data"]
        expected_context.enrolling[1].mds_url = "http://storage-int.mds.yandex.net:80/get-speechbase/spotter.mp3"
        expected_context.enrolling[2].mds_url = "http://storage-int.mds.yandex.net:80/get-speechbase/stream.mp3"
        expected_context.users[0].voiceprints[
            1
        ].mds_url = "http://storage-int.mds.yandex.net:80/get-speechbase/spotter.mp3"
        assert req.Save.Context == expected_context.SerializeToString()

    def test_text_cache(self, cuttlefish: Cuttlefish):
        items = self.DEFAULT_ITEMS.copy()
        items[2] = {
            "type": ItemTypes.YABIO_CONTEXT,
            "data": YabioContext(
                group_id="my-cool-uuid",
                enrolling=[
                    YabioVoiceprint(
                        request_id="other-req-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="my-cool-message-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="my-cool-message-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="request",
                        text="old-text",
                    ),
                    YabioVoiceprint(
                        request_id="other-other-req-id",
                        compatibility_tag="some-tag",
                        format="some-format",
                        source="spotter",
                    ),
                ],
            ),
        }

        items.append(
            {
                "type": ItemTypes.YABIO_TEXT,
                "data": TBioContextSaveText(
                    Source="spotter",
                    Text="this is spotter text",
                ),
            }
        )
        items.append(
            {
                "type": ItemTypes.YABIO_TEXT,
                "data": TBioContextSaveText(
                    Source="request",
                    Text="this is stream text",
                ),
            }
        )

        response = cuttlefish.make_grpc_request(ServiceHandles.BIO_CONTEXT_SAVE_PRE, create_grpc_request(items=items))
        assert len(response.Answers) == 1

        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT_REQUEST, proto=TYabioContextRequest)
        assert req.Save.Key.GroupId == "my-cool-uuid"
        assert req.Save.Key.DevModel == "my-cool-device-model"
        assert req.Save.Key.DevManuf == "my-cool-device-manuf"

        expected_context = items[2]["data"]
        expected_context.enrolling[1].text = "this is spotter text"
        expected_context.enrolling[2].text = "this is stream text"
        assert req.Save.Context == expected_context.SerializeToString()

    def test_delete_incompatible_tags(self, cuttlefish: Cuttlefish):
        items = self.DEFAULT_ITEMS.copy()
        items[2] = {
            "type": ItemTypes.YABIO_CONTEXT,
            "data": YabioContext(
                group_id="my-cool-uuid",
                users=[
                    YabioUser(
                        user_id="my-cool-user-id",
                        voiceprints=[
                            YabioVoiceprint(
                                request_id="other-req-id",
                                compatibility_tag="tag-1",
                                format="some-format",
                                source="spotter",
                            ),
                            YabioVoiceprint(
                                request_id="other-req-id",
                                compatibility_tag="tag-2",
                                format="some-format",
                                source="spotter",
                            ),
                        ],
                    ),
                    YabioUser(
                        user_id="my-cool-user-id",
                        voiceprints=[
                            YabioVoiceprint(
                                request_id="other-req-id",
                                compatibility_tag="tag-5",
                                format="some-format",
                                source="spotter",
                            ),
                        ],
                    ),
                ],
                enrolling=[
                    YabioVoiceprint(
                        request_id="other-req-id",
                        compatibility_tag="tag-1",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="other-req-id",
                        compatibility_tag="tag-2",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="other-req-id",
                        compatibility_tag="tag-3",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="other-req-id",
                        compatibility_tag="tag-4",
                        format="some-format",
                        source="spotter",
                    ),
                ],
            ),
        }

        items.append(
            {
                "type": ItemTypes.YABIO_NEW_ENROLLING,
                "data": TBioContextSaveNewEnrolling(
                    SupportedTags=[
                        "tag-2",
                        "tag-4",
                    ],
                ),
            }
        )

        response = cuttlefish.make_grpc_request(ServiceHandles.BIO_CONTEXT_SAVE_PRE, create_grpc_request(items=items))
        assert len(response.Answers) == 1

        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT_REQUEST, proto=TYabioContextRequest)
        assert req.Save.Key.GroupId == "my-cool-uuid"
        assert req.Save.Key.DevModel == "my-cool-device-model"
        assert req.Save.Key.DevManuf == "my-cool-device-manuf"

        expected_context = items[2]["data"]
        del expected_context.users[0].voiceprints[0]
        del expected_context.users[1]
        del expected_context.enrolling[2]
        del expected_context.enrolling[0]
        assert req.Save.Context == expected_context.SerializeToString()

    def test_new_enrolling(self, cuttlefish: Cuttlefish):
        items = self.DEFAULT_ITEMS.copy()
        items[2] = {
            "type": ItemTypes.YABIO_CONTEXT,
            "data": YabioContext(
                group_id="my-cool-uuid",
                enrolling=[
                    YabioVoiceprint(
                        request_id="id-1",
                        compatibility_tag="tag",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="id-2",
                        compatibility_tag="tag",
                        format="some-format",
                        source="request",
                    ),
                    YabioVoiceprint(
                        request_id="id-3",
                        compatibility_tag="tag",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="id-4",
                        compatibility_tag="tag",
                        format="some-format",
                        source="request",
                    ),
                ],
            ),
        }

        items.append(
            {
                "type": ItemTypes.YABIO_NEW_ENROLLING,
                "data": TBioContextSaveNewEnrolling(
                    YabioEnrolling=[
                        YabioVoiceprint(
                            request_id="id-1",
                            compatibility_tag="tag",
                            format="new-format",
                            source="spotter",
                        ),
                        YabioVoiceprint(
                            request_id="id-2",
                            compatibility_tag="tag",
                            format="new-format",
                            source="request",
                        ),
                        YabioVoiceprint(
                            request_id="id-5",
                            compatibility_tag="tag",
                            format="new-format",
                            source="spotter",
                        ),
                    ],
                ),
            }
        )

        response = cuttlefish.make_grpc_request(ServiceHandles.BIO_CONTEXT_SAVE_PRE, create_grpc_request(items=items))
        assert len(response.Answers) == 1

        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT_REQUEST, proto=TYabioContextRequest)
        assert req.Save.Key.GroupId == "my-cool-uuid"
        assert req.Save.Key.DevModel == "my-cool-device-model"
        assert req.Save.Key.DevManuf == "my-cool-device-manuf"

        expected_context = items[2]["data"]
        del expected_context.enrolling[1]
        del expected_context.enrolling[0]
        expected_context.enrolling.extend(items[-1]["data"].YabioEnrolling)
        assert req.Save.Context == expected_context.SerializeToString()

    def test_delete_overlimit(self, cuttlefish: Cuttlefish):
        items = self.DEFAULT_ITEMS.copy()
        items[2] = {
            "type": ItemTypes.YABIO_CONTEXT,
            "data": YabioContext(
                group_id="my-cool-uuid",
                enrolling=[],
            ),
        }
        for i in range(30):
            items[2]["data"].enrolling.extend(
                [
                    YabioVoiceprint(
                        request_id="id-" + str(i),
                        compatibility_tag="tag",
                        format="some-format",
                        source="spotter",
                    )
                ]
            )

        new_enrolling = TBioContextSaveNewEnrolling(YabioEnrolling=[])
        for i in range(30):
            new_enrolling.YabioEnrolling.extend(
                [
                    YabioVoiceprint(
                        request_id="id-" + str(i + 30),
                        compatibility_tag="tag",
                        format="some-format",
                        source="spotter",
                    )
                ]
            )

        items.append(
            {
                "type": ItemTypes.YABIO_NEW_ENROLLING,
                "data": new_enrolling,
            }
        )

        response = cuttlefish.make_grpc_request(ServiceHandles.BIO_CONTEXT_SAVE_PRE, create_grpc_request(items=items))
        assert len(response.Answers) == 1

        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT_REQUEST, proto=TYabioContextRequest)
        assert req.Save.Key.GroupId == "my-cool-uuid"
        assert req.Save.Key.DevModel == "my-cool-device-model"
        assert req.Save.Key.DevManuf == "my-cool-device-manuf"

        expected_context = items[2]["data"]
        # the limit is 40 elements, 30 + 30 - 40 == 20
        for i in range(20):
            del expected_context.enrolling[0]
        expected_context.enrolling.extend(new_enrolling.YabioEnrolling)
        assert req.Save.Context == expected_context.SerializeToString()

    def test_save_voiceprint(self, cuttlefish: Cuttlefish):
        json_payload = {
            "user_id": "my-cool-user-id",
            "requests": ["req-4", "req-2", "req-1337"],
        }
        payload = json_format.Parse(json.dumps(json_payload), google_protobuf_Struct())

        items = self.DEFAULT_ITEMS.copy()
        items[2] = {
            "type": ItemTypes.YABIO_CONTEXT,
            "data": YabioContext(
                group_id="my-cool-uuid",
                users=[
                    YabioUser(
                        user_id="my-cool-user-id",
                        voiceprints=[
                            YabioVoiceprint(
                                request_id="other-req-id",
                                compatibility_tag="tag-2",
                                format="some-format",
                                source="spotter",
                            ),
                        ],
                    ),
                ],
                enrolling=[
                    YabioVoiceprint(
                        request_id="req-1",
                        compatibility_tag="tag-1",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="req-2",
                        compatibility_tag="tag-2",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="req-3",
                        compatibility_tag="tag-3",
                        format="some-format",
                        source="spotter",
                    ),
                    YabioVoiceprint(
                        request_id="req-4",
                        compatibility_tag="tag-4",
                        format="some-format",
                        source="spotter",
                    ),
                ],
            ),
        }

        items.append(
            {
                "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                "data": TContextSaveRequest(
                    Directives=[
                        make_speechkit_directive(name="save_voiceprint", payload=payload),
                    ]
                ),
            }
        )

        response = cuttlefish.make_grpc_request(ServiceHandles.BIO_CONTEXT_SAVE_PRE, create_grpc_request(items=items))
        assert len(response.Answers) == 1

        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT_REQUEST, proto=TYabioContextRequest)
        assert req.Save.Key.GroupId == "my-cool-uuid"
        assert req.Save.Key.DevModel == "my-cool-device-model"
        assert req.Save.Key.DevManuf == "my-cool-device-manuf"

        expected_context = items[2]["data"]
        expected_context.enrolling[3].reg_num = 0
        expected_context.enrolling[1].reg_num = 1
        del expected_context.users[0].voiceprints[:]
        expected_context.users[0].voiceprints.extend([expected_context.enrolling[1], expected_context.enrolling[3]])
        del expected_context.enrolling[3]
        del expected_context.enrolling[1]

        assert req.Save.Context == expected_context.SerializeToString()

    def test_remove_voiceprint(self, cuttlefish: Cuttlefish):
        json_payload = {
            "user_id": "my-cool-user-id",
        }
        payload = json_format.Parse(json.dumps(json_payload), google_protobuf_Struct())

        items = self.DEFAULT_ITEMS.copy()
        items[2] = {
            "type": ItemTypes.YABIO_CONTEXT,
            "data": YabioContext(
                group_id="my-cool-uuid",
                users=[
                    YabioUser(
                        user_id="some-user-id",
                    ),
                    YabioUser(
                        user_id="my-cool-user-id",
                    ),
                    YabioUser(
                        user_id="voovoovoo",
                    ),
                ],
            ),
        }

        items.append(
            {
                "type": ItemTypes.CONTEXT_SAVE_REQUEST,
                "data": TContextSaveRequest(
                    Directives=[
                        make_speechkit_directive(name="remove_voiceprint", payload=payload),
                    ]
                ),
            }
        )

        response = cuttlefish.make_grpc_request(ServiceHandles.BIO_CONTEXT_SAVE_PRE, create_grpc_request(items=items))
        assert len(response.Answers) == 1

        req = get_answer_item(response, ItemTypes.YABIO_CONTEXT_REQUEST, proto=TYabioContextRequest)
        assert req.Save.Key.GroupId == "my-cool-uuid"
        assert req.Save.Key.DevModel == "my-cool-device-model"
        assert req.Save.Key.DevManuf == "my-cool-device-manuf"

        expected_context = items[2]["data"]
        del expected_context.users[1]

        assert req.Save.Context == expected_context.SerializeToString()


class TestBioContextSavePost:
    def test_success(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.BIO_CONTEXT_SAVE_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.YABIO_CONTEXT_RESPONSE,
                        "data": TYabioContextResponse(
                            Success=TYabioContextSuccess(
                                Context=b"ololo trololo",
                            ),
                        ),
                    }
                ]
            ),
        )
        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.YABIO_CONTEXT_SAVED)

    """ disable for VOICESERV-4123
    def test_error(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(ServiceHandles.BIO_CONTEXT_SAVE_POST, create_grpc_request(items=[{
            "type": ItemTypes.YABIO_CONTEXT_RESPONSE,
            "data": TYabioContextResponse(
                Error=TYabioContextError(
                    Text="vzlom zhopi",
                ),
            ),
        }]))
        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.DIRECTIVE)  # event exception
    """
