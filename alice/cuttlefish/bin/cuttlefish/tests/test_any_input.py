import pytest
from .common import Cuttlefish, create_grpc_request
from alice.cachalot.api.protos.cachalot_pb2 import TMegamindSessionRequest
from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadSmarthomeUid
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TRequestContext, TUserInfo
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.items import get_answer_item


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
class TestAnyInputPre:
    def test(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.ANY_INPUT_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            Header=TRequestContext.THeader(
                                SessionId="cool-session-id",
                                MessageId="cool-message-id",
                                DialogId="cool-dialog-id",
                                PrevReqId="cool-prev-req-id",
                            ),
                            AdditionalOptions=TRequestContext.TAdditionalOptions(
                                SmarthomeUid="cool-smarthome-uid",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.SESSION_CONTEXT,
                        "data": TSessionContext(
                            UserInfo=TUserInfo(
                                Uuid="bad-uuid",
                                VinsApplicationUuid="good-uuid",
                            ),
                        ),
                    },
                ],
            ),
        )
        assert len(response.Answers) == 2

        mm_session_req = get_answer_item(response, ItemTypes.MM_SESSION_REQUEST, proto=TMegamindSessionRequest)
        assert mm_session_req.LoadRequest.Uuid == "good-uuid"
        assert mm_session_req.LoadRequest.DialogId == "cool-dialog-id"
        assert mm_session_req.LoadRequest.RequestId == "cool-prev-req-id"

        smarthome_uid = get_answer_item(response, ItemTypes.SMARTHOME_UID, proto=TContextLoadSmarthomeUid)
        assert smarthome_uid.Value == "cool-smarthome-uid"
