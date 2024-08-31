from .common import Cuttlefish
from alice.cachalot.api.protos.cachalot_pb2 import (
    EResponseStatus,
    TMegamindSessionLoadResponse,
    TResponse as TCachalotResponse,
)
from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse
from alice.cuttlefish.library.protos.context_save_pb2 import TContextSaveRequest
from alice.cuttlefish.library.protos.megamind_pb2 import TMegamindRequest, TMegamindResponse
from alice.cuttlefish.library.protos.session_pb2 import (
    TConnectionInfo,
    TFlagsInfo,
    TRequestContext,
    TSessionContext,
    TUserInfo,
    TUserOptions,
)
from alice.cuttlefish.library.protos.uniproxy2_pb2 import TUniproxyDirective
from alice.cuttlefish.library.python.apphost_grpc_client import AppHostGrpcClient
from alice.cuttlefish.library.python.testing.constants import ItemTypes
from alice.library.client.protos.client_info_pb2 import TClientInfoProto
from alice.megamind.protos.common.experiments_pb2 import TExperimentsProto
from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.megamind.protos.scenarios.notification_state_pb2 import TNotificationState, TSubscription
from alice.megamind.protos.speechkit.request_pb2 import TSpeechKitRequestProto
from alice.megamind.protos.speechkit.response_pb2 import TSpeechKitResponseProto

from library.python import resource

from apphost.lib.proto_answers.http_pb2 import THttpResponse
from google.protobuf import json_format
import yatest.common.network

from tornado import httputil
from tornado.queues import Queue
from tornado.tcpserver import TCPServer, IOStream
import tornado.concurrent

from functools import partial
import base64
import json
import logging
import pytest
import socket


# -------------------------------------------------------------------------------------------------
async def read_request_line(stream: IOStream):
    data = await stream.read_until(b"\r\n")
    return httputil.parse_request_start_line(data.decode("utf-8").strip())


async def read_request_headers(stream: IOStream):
    data = await stream.read_until(b"\r\n\r\n")
    return httputil.HTTPHeaders.parse(data.decode("utf-8").strip())


async def read_http_request(stream: IOStream, with_body=False) -> httputil.HTTPServerRequest:
    start_line = await read_request_line(stream)
    headers = await read_request_headers(stream)
    body = None
    if with_body:
        body = await stream.read_bytes(int(headers["Content-Length"]))
    return httputil.HTTPServerRequest(start_line=start_line, headers=headers, body=body)


class QueuedTcpServer(TCPServer):
    class __Stop:
        pass

    def __enter__(self):
        self.listen()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__incoming = Queue()
        self.__port_manager = None
        self.__runing = False
        self.port = None

    def stop(self):
        self.__runing = False

        super().stop()
        self.__incoming.put_nowait(self.__Stop)
        self.__port_manager.release_port(self.port)

    def listen(self, address="", port_manager=None):
        self.__port_manager = port_manager or yatest.common.network.PortManager()
        self.port = self.__port_manager.get_port()
        super().listen(self.port, address)

        self.__runing = True

    async def pop_stream(self, accept=True, timeout=None) -> IOStream:
        if not self.__runing:
            return None

        stream = await self.__incoming.get(timeout)
        self.__incoming.task_done()
        if stream == self.__Stop:
            return None

        return stream

    async def handle_stream(self, stream: IOStream, address):
        self.__incoming.put_nowait(stream)


def run_async(func, *args, **kwargs):
    import tornado.ioloop

    async def wrapper():
        return await func(*args, **kwargs)

    return tornado.ioloop.IOLoop.current().run_sync(wrapper)


@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
class TestMegamind:
    @pytest.mark.asyncio
    @pytest.mark.parametrize(
        "vins_http_code, additional_vins_headers",
        [
            (b"200 OK", b""),
            (b"512 NEOK", b"x-yandex-vins-ok: true\r\n"),
        ],
    )
    async def test_simple_http_request(self, vins_http_code, additional_vins_headers, cuttlefish: Cuttlefish):
        loop = tornado.ioloop.IOLoop.current()
        finished = tornado.concurrent.Future()

        async def run(port):
            try:
                client = AppHostGrpcClient(cuttlefish.grpc_endpoint.split(":"))
                async with client.create_stream(path="/megamind_run") as stream:
                    # first pack of items
                    stream.write_items(
                        {
                            ItemTypes.APP_HOST_PARAMS: [
                                {"reqid": "my_lovely_reqid"},
                            ],
                            ItemTypes.SESSION_CONTEXT: [
                                TSessionContext(  # mostly ignored (used TMegamindRequest)
                                    AppType="MEGADEVICE",
                                    AppId="STALIN-3000",
                                    SessionId="my-session-id",
                                    RequestBase=TSpeechKitRequestProto(
                                        Application=TClientInfoProto(
                                            AppId="ignored-test-app-id",
                                            Platform="ignored-test-platform",
                                            DeviceColor="ignored-test-device-color",
                                        ),
                                    ),
                                ),
                            ],
                            ItemTypes.MEGAMIND_REQUEST: [
                                TMegamindRequest(
                                    RequestBase=TSpeechKitRequestProto(
                                        Application=TClientInfoProto(
                                            AppId="test-app-id",
                                            Platform="test-platform",
                                            DeviceColor="test-device-color",
                                        ),
                                        Header=TSpeechKitRequestProto.THeader(
                                            RequestId="my_lovely_reqid",
                                        ),
                                        Session="my_session",
                                        MementoData="memchiki)))0)0",
                                    ),
                                ),
                            ],
                            ItemTypes.REQUEST_CONTEXT: [
                                TRequestContext(
                                    VinsUrl=f"http://localhost:{port}/speechkit/app/pa",
                                    Header=TRequestContext.THeader(
                                        SessionId="my-session-id-2",  # synthetic desync with session_id in session_context
                                        MessageId="test-message-id",
                                    ),
                                ),
                            ],
                        }
                    )

                    # second pack of items
                    stream.write_items(
                        {
                            ItemTypes.CONTEXT_LOAD_RESPONSE: [
                                TContextLoadResponse(
                                    UserTicket="my-lovely-user-ticket",
                                ),
                            ],
                        }
                    )

                    resp = await stream.read(timeout=1.0)

                    sessionLogVinsRequest = resp.get_only_item(
                        item_type=ItemTypes.UNIPROXY2_DIRECTIVE_SESSION_LOG, proto_type=TUniproxyDirective
                    )
                    assert sessionLogVinsRequest is not None
                    try:
                        item = resp.get_only_item(item_type=ItemTypes.MEGAMIND_RESPONSE, proto_type=TMegamindResponse)
                    except RuntimeError:
                        logging.exception("not find MEGAMIND_RESPONSE, try next read")
                        resp = await stream.read(timeout=1.0)
                        item = resp.get_only_item(item_type=ItemTypes.MEGAMIND_RESPONSE, proto_type=TMegamindResponse)

                assert item.data.ProtoResponse == TSpeechKitResponseProto(
                    Version="1.2.3",
                    Header=TSpeechKitResponseProto.THeader(
                        SequenceNumber=228,
                    ),
                )
                assert item.data.RawJsonResponse == '{"version": "1.2.3", "header": {"sequence_number": 228}}'

                finished.set_result(None)
            except Exception as exc:
                finished.set_exception(exc)
                assert False, "Error threw"

        try:
            # mocked Megamind server
            with QueuedTcpServer() as srv:
                loop.add_callback(partial(run, port=srv.port))

                stream = await srv.pop_stream()
                request = await read_http_request(stream, with_body=True)

                assert request.method == "POST"
                assert request.uri == "/speechkit/app/pa"  # run suffix
                headers = dict(request.headers)
                assert 'my_lovely_reqid-0' in headers.pop('X-Rtlog-Token')
                assert 'my_lovely_reqid-0' in headers.pop('X-Yandex-Req-Id')
                assert headers == {
                    "Host": f"localhost:{srv.port}",
                    "Content-Type": "application/json",
                    "X-Ya-User-Ticket": "my-lovely-user-ticket",
                    "X-Yandex-Internal-Request": "1",
                    "X-Alice-Appid": "STALIN-3000",
                    "X-Alice-Apptype": "MEGADEVICE",
                    "X-Alice-Client-Reqid": "my_lovely_reqid",
                    "X-Alice-Ispartial": "0",
                    'X-Ya-Servant-Hostname': socket.gethostname(),
                    'X-Yandex-Target-Megamind-Url': f'http://localhost:{srv.port}/speechkit/app/pa',
                    "Content-Length": str(len(request.body)),
                }
                expected_body = b'{"application":{"app_id":"test-app-id","device_color":"test-device-color","platform":"test-platform"},'
                expected_body += b'"header":{"ref_message_id":"test-message-id","request_id":"my_lovely_reqid"},"memento":"memchiki)))0)0",'
                expected_body += b'"request":{"additional_options":{}},"session":"my_session"}'
                assert request.body == expected_body, "{} != {}".format(request.body.decode(), expected_body.decode())

                resp_str = b'{"version": "1.2.3", "header": {"sequence_number": 228}}'
                await stream.write(
                    b"HTTP/1.1 " + vins_http_code + b"\r\n"
                    b"Content-Length: " + str(len(resp_str)).encode() + b"\r\n"
                    b"Connection: Keep-Alive\r\n" + additional_vins_headers + b"\r\n" + resp_str
                )
        except:
            logging.exception("server failed")
            assert False, "Error threw"
        finally:
            await finished

    @pytest.mark.asyncio
    @pytest.mark.parametrize("use_protobuf", [True, False])
    async def test_everything(self, use_protobuf, cuttlefish: Cuttlefish):
        DATASYNC_RESPONSE_CONTENT = json.dumps(
            json.loads(
                '''
{
  "items": [
    {
      "body": "{\\"items\\":[{\\"last_used\\":\\"2020-09-30T06:04:58.251000+00:00\\",\\"address_id\\":\\"home\\",\\"tags\\":[\\"_w-_traffic-1-start\\"],\\"title\\":\\"home\\",\\"modified\\":\\"2020-09-30T06:04:58.251000+00:00\\",\\"longitude\\":37.3909817619,\\"created\\":\\"2015-12-10T09:07:25.951000+00:00\\",\\"mined_attributes\\":[],\\"address_line_short\\":\\"Производственная улица, 12к2\\",\\"latitude\\":55.6430517188,\\"address_line\\":\\"Россия, Москва, Производственная улица, 12к2\\"},{\\"last_used\\":\\"2020-09-30T06:04:58.253000+00:00\\",\\"address_id\\":\\"work\\",\\"tags\\":[],\\"title\\":\\"1-й Красногвардейский проезд, 19\\",\\"custom_metadata\\":\\"{\\\\\\"uri\\\\\\":\\\\\\"ymapsbm1://geo?ll=37.537%2C55.751&spn=0.001%2C0.001&text=%D0%A0%D0%BE%D1%81%D1%81%D0%B8%D1%8F%2C%20%D0%9C%D0%BE%D1%81%D0%BA%D0%B2%D0%B0%2C%201-%D0%B9%20%D0%9A%D1%80%D0%B0%D1%81%D0%BD%D0%BE%D0%B3%D0%B2%D0%B0%D1%80%D0%B4%D0%B5%D0%B9%D1%81%D0%BA%D0%B8%D0%B9%20%D0%BF%D1%80%D0%BE%D0%B5%D0%B7%D0%B4%2C%2019\\\\\\",\\\\\\"floor_number\\\\\\":\\\\\\"35\\\\\\",\\\\\\"quarters_number\\\\\\":\\\\\\"\\\\\\",\\\\\\"doorphone_number\\\\\\":\\\\\\"\\\\\\"}\\",\\"modified\\":\\"2021-02-11T15:21:31.658000+00:00\\",\\"longitude\\":37.536971,\\"created\\":\\"2015-12-10T09:07:25.911000+00:00\\",\\"mined_attributes\\":[],\\"address_line_short\\":\\"1-й Красногвардейский проезд, 19\\",\\"latitude\\":55.750122,\\"address_line\\":\\"Россия, Москва, 1-й Красногвардейский проезд, 19\\"}],\\"total\\":2,\\"limit\\":20,\\"offset\\":0}",
      "headers": {
        "Access-Control-Allow-Methods": "POST, GET, OPTIONS",
        "Access-Control-Allow-Credentials": "true",
        "Yandex-Cloud-Request-ID": "rest-05e6d41b170ac6c1a78fe500a0ec7ad1-api12v",
        "Cache-Control": "no-cache",
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Headers": "Accept-Language, Accept, X-Uid, X-HTTP-Method, X-Requested-With, Content-Type, Authorization",
        "Content-Type": "application/json; charset=utf-8"
      },
      "code": 200
    },
    {
      "body": "{\\"items\\":[{\\"id\\":\\"AutomotivePromoCounters\\",\\"value\\":\\"{\\\\\\"auto_music_promo_2020\\\\\\":3}\\"},{\\"id\\":\\"proactivity_history\\",\\"value\\":\\"{\\\\\\"LastShowTime\\\\\\":\\\\\\"1615288721\\\\\\",\\\\\\"PostrollCount\\\\\\":\\\\\\"3\\\\\\",\\\\\\"TagStats\\\\\\":{\\\\\\"music_features_new_users_3\\\\\\":{\\\\\\"LastShowTime\\\\\\":\\\\\\"1615288721\\\\\\",\\\\\\"LastShowPostrollCount\\\\\\":\\\\\\"3\\\\\\",\\\\\\"LastShowRequestCount\\\\\\":\\\\\\"10\\\\\\"},\\\\\\"music_features_new_users_like_dislike_delimimter\\\\\\":{\\\\\\"LastShowTime\\\\\\":\\\\\\"1615116530\\\\\\",\\\\\\"LastShowPostrollCount\\\\\\":\\\\\\"2\\\\\\",\\\\\\"LastShowRequestCount\\\\\\":\\\\\\"6\\\\\\"},\\\\\\"music_features_new_users_4\\\\\\":{\\\\\\"LastShowTime\\\\\\":\\\\\\"1615013031\\\\\\",\\\\\\"LastShowPostrollCount\\\\\\":\\\\\\"1\\\\\\",\\\\\\"LastShowRequestCount\\\\\\":\\\\\\"3\\\\\\"},\\\\\\"music_features_new_users\\\\\\":{\\\\\\"LastShowTime\\\\\\":\\\\\\"1615288721\\\\\\",\\\\\\"LastShowPostrollCount\\\\\\":\\\\\\"3\\\\\\",\\\\\\"LastShowRequestCount\\\\\\":\\\\\\"10\\\\\\"},\\\\\\"music_features_new_users_6\\\\\\":{\\\\\\"LastShowTime\\\\\\":\\\\\\"1615116530\\\\\\",\\\\\\"LastShowPostrollCount\\\\\\":\\\\\\"2\\\\\\",\\\\\\"LastShowRequestCount\\\\\\":\\\\\\"6\\\\\\"}},\\\\\\"LastPostrollViews\\\\\\":[{\\\\\\"ItemId\\\\\\":\\\\\\"music_features_new_users_4\\\\\\",\\\\\\"Analytics\\\\\\":{\\\\\\"Info\\\\\\":\\\\\\"personal_assistant.scenarios.music_play\\\\\\",\\\\\\"SuccessConditions\\\\\\":[{\\\\\\"Frame\\\\\\":{\\\\\\"name\\\\\\":\\\\\\"personal_assistant.scenarios.music_play\\\\\\",\\\\\\"slots\\\\\\":[{\\\\\\"name\\\\\\":\\\\\\"fake_frame\\\\\\",\\\\\\"value\\\\\\":\\\\\\"fake_frame\\\\\\"}]}}]},\\\\\\"Tags\\\\\\":[\\\\\\"music_features_new_users\\\\\\",\\\\\\"music_features_new_users_4\\\\\\"],\\\\\\"Source\\\\\\":\\\\\\"Commands\\\\\\",\\\\\\"Context\\\\\\":{\\\\\\"App\\\\\\":\\\\\\"aliced\\\\\\",\\\\\\"Language\\\\\\":\\\\\\"rus\\\\\\"},\\\\\\"RequestId\\\\\\":\\\\\\"61ed37ea-2473-4f9c-8df2-849ba182fca8\\\\\\"},{\\\\\\"ItemId\\\\\\":\\\\\\"music_features_new_users_6\\\\\\",\\\\\\"Analytics\\\\\\":{\\\\\\"Info\\\\\\":\\\\\\"personal_assistant.scenarios.music_play\\\\\\",\\\\\\"SuccessConditions\\\\\\":[{\\\\\\"Frame\\\\\\":{\\\\\\"name\\\\\\":\\\\\\"personal_assistant.scenarios.player.like\\\\\\"}},{\\\\\\"Frame\\\\\\":{\\\\\\"name\\\\\\":\\\\\\"personal_assistant.scenarios.player.dislike\\\\\\"}},{\\\\\\"Frame\\\\\\":{\\\\\\"name\\\\\\":\\\\\\"personal_assistant.scenarios.player_like\\\\\\"}},{\\\\\\"Frame\\\\\\":{\\\\\\"name\\\\\\":\\\\\\"personal_assistant.scenarios.player_dislike\\\\\\"}}]},\\\\\\"Tags\\\\\\":[\\\\\\"music_features_new_users\\\\\\",\\\\\\"music_features_new_users_6\\\\\\",\\\\\\"music_features_new_users_like_dislike_delimimter\\\\\\"],\\\\\\"Source\\\\\\":\\\\\\"Commands\\\\\\",\\\\\\"Context\\\\\\":{\\\\\\"App\\\\\\":\\\\\\"aliced\\\\\\",\\\\\\"Language\\\\\\":\\\\\\"rus\\\\\\"},\\\\\\"RequestId\\\\\\":\\\\\\"39ffbd39-90fb-45a4-b62d-8f45dea4f19a\\\\\\"},{\\\\\\"ItemId\\\\\\":\\\\\\"music_features_new_users_3\\\\\\",\\\\\\"Analytics\\\\\\":{\\\\\\"Info\\\\\\":\\\\\\"personal_assistant.scenarios.music_play\\\\\\",\\\\\\"SuccessConditions\\\\\\":[{\\\\\\"Frame\\\\\\":{\\\\\\"name\\\\\\":\\\\\\"personal_assistant.scenarios.music_play\\\\\\",\\\\\\"slots\\\\\\":[{\\\\\\"name\\\\\\":\\\\\\"novelty\\\\\\",\\\\\\"value\\\\\\":\\\\\\"new\\\\\\"}]}}]},\\\\\\"Tags\\\\\\":[\\\\\\"music_features_new_users\\\\\\",\\\\\\"music_features_new_users_3\\\\\\"],\\\\\\"Source\\\\\\":\\\\\\"Commands\\\\\\",\\\\\\"Context\\\\\\":{\\\\\\"App\\\\\\":\\\\\\"aliced\\\\\\",\\\\\\"Language\\\\\\":\\\\\\"rus\\\\\\"},\\\\\\"RequestId\\\\\\":\\\\\\"06c7cd34-1a5a-419a-9dc5-6f1a0caad8ab\\\\\\"}],\\\\\\"LastPostrollRequestId\\\\\\":\\\\\\"06c7cd34-1a5a-419a-9dc5-6f1a0caad8ab\\\\\\",\\\\\\"RequestCount\\\\\\":\\\\\\"28\\\\\\",\\\\\\"LastShowRequestCount\\\\\\":\\\\\\"10\\\\\\",\\\\\\"LastStorageUpdateTime\\\\\\":\\\\\\"1616478344\\\\\\"}\\"}]}",
      "headers": {
        "Access-Control-Allow-Methods": "PUT, POST, GET, DELETE, OPTIONS",
        "Access-Control-Allow-Credentials": "true",
        "Yandex-Cloud-Request-ID": "rest-05e6d41b170ac6c1a78fe500a0ec7ad1-api12v",
        "Cache-Control": "no-cache",
        "Access-Control-Allow-Origin": "*",
        "Access-Control-Allow-Headers": "Accept-Language, Accept, X-Uid, X-HTTP-Method, X-Requested-With, Content-Type, Authorization",
        "Content-Type": "application/json; charset=utf-8"
      },
      "code": 200
    }
  ]
}
        '''
            )
        )  # noqa: E501

        DATASYNC_DEVICE_ID_RESPONSE_CONTENT = DATASYNC_UUID_RESPONSE_CONTENT = json.dumps(
            json.loads(resource.find('datasync_response').decode("utf-8")))
        )

        QUASAR_IOT_DATA = TIoTUserInfo(
            RawUserInfo='{"status": "ok", "payload": {"colors": [{"id": "ololo", "name": "trololo"}]}}',
            Colors=[
                TIoTUserInfo.TColor(Id="green", Name="Shrek"),
                TIoTUserInfo.TColor(Id="yellow", Name="Banana"),
            ],
        )

        NOTIFICATOR_DATA = TNotificationState(
            CountArchived=228,
            Subscriptions=[
                TSubscription(
                    Id="dva",
                    Timestamp="dva",
                    Name="vosem",
                )
            ],
        )

        MM_SESSION_DATA = TCachalotResponse(
            Status=EResponseStatus.OK,
            MegamindSessionLoadResp=TMegamindSessionLoadResponse(
                Data=b"best session",
            ),
        )

        MM_RESPONSE_JSON = {
            "header": {"asr_partial_number": 0, "classification_partial_number": 0, "scoring_partial_number": 0},
            "voice_response": {
                "directives": [
                    {
                        "type": "uni_action",
                        "name": "gachi1",
                        "payload": {
                            "money": "three hundred bucks",
                            "actor": "ricardo milos",
                        },
                    },
                    {
                        "type": "uni_action",
                        "name": "gachi2",
                        "payload": {
                            "money": "infinity",
                            "actor": "billy herrington",
                        },
                    },
                    {
                        "type": "uni_action",
                        "name": "save_user_audio",
                        "payload": {
                            "key": "test_edge_flag",
                        },
                    },
                ],
                "uniproxy_directives": [
                    {
                        "context_save_directive": {
                            "directive_id": "one",
                        },
                    },
                    {
                        "context_save_directive": {
                            "directive_id": "two",
                        },
                    },
                ],
            },
            "sessions": {
                "": base64.b64encode(b"my better session").decode(),
            },
        }

        MM_RESPONSE_JSON_STATELESS = MM_RESPONSE_JSON.copy()
        del MM_RESPONSE_JSON_STATELESS["sessions"]

        loop = tornado.ioloop.IOLoop.current()
        finished = tornado.concurrent.Future()

        async def run(port):
            try:
                client = AppHostGrpcClient(cuttlefish.grpc_endpoint.split(":"))

                req_ctx = TRequestContext(VinsUrl=f"http://localhost:{port}/speechkit/app/pa")
                req_ctx.ExpFlags["my_mega_unique_flag_from_requset_payload"] = "feature_enabled"
                if use_protobuf:
                    req_ctx.ExpFlags["send_protobuf_to_megamind"] = "1"

                async with client.create_stream(path="/megamind_run") as stream:
                    # first pack of items
                    stream.write_items(
                        {
                            ItemTypes.SESSION_CONTEXT: [
                                TSessionContext(
                                    ConnectionInfo=TConnectionInfo(
                                        IpAddress="228.228.228.228",
                                    ),
                                    UserInfo=TUserInfo(
                                        Yuid="yuid",
                                        Puid="puid",
                                        ICookie="i-cookie",
                                    ),
                                    UserOptions=TUserOptions(
                                        DoNotUseLogs=True,
                                    ),
                                ),
                            ],
                            ItemTypes.MEGAMIND_REQUEST: [
                                TMegamindRequest(),
                            ],
                            ItemTypes.REQUEST_CONTEXT: [req_ctx],
                        }
                    )

                    # second pack of items
                    stream.write_items(
                        {
                            ItemTypes.CONTEXT_LOAD_RESPONSE: [
                                TContextLoadResponse(
                                    MementoResponse=THttpResponse(
                                        StatusCode=200,
                                        Content=b"MEMCHIKI)))0))0)",
                                    ),
                                    DatasyncResponse=THttpResponse(
                                        StatusCode=200,
                                        Content=DATASYNC_RESPONSE_CONTENT.encode(),
                                    ),
                                    DatasyncDeviceIdResponse=THttpResponse(
                                        StatusCode=200,
                                        Content=DATASYNC_DEVICE_ID_RESPONSE_CONTENT.encode(),
                                    ),
                                    DatasyncUuidResponse=THttpResponse(
                                        StatusCode=200,
                                        Content=DATASYNC_UUID_RESPONSE_CONTENT.encode(),
                                    ),
                                    IoTUserInfo=QUASAR_IOT_DATA,
                                    NotificatorResponse=THttpResponse(
                                        StatusCode=200,
                                        Content=NOTIFICATOR_DATA.SerializeToString(),
                                    ),
                                    FlagsInfo=TFlagsInfo(
                                        ExpBoxes="XBOX ONE",
                                        VoiceFlags=TExperimentsProto(
                                            Storage={
                                                "123-32": TExperimentsProto.TValue(String="91"),
                                                "456-32": TExperimentsProto.TValue(Integer=424),
                                            },
                                        ),
                                        AllTestIds=[
                                            "6973",
                                            "10205",
                                            "invalid",
                                        ],
                                    ),
                                    MegamindSessionResponse=MM_SESSION_DATA,
                                    LaasResponse=THttpResponse(
                                        StatusCode=200,
                                        Content=json.dumps(
                                            {
                                                "city_id": 213,
                                                "country_id_by_ip": 225,
                                            }
                                        ).encode("ascii"),
                                    ),
                                ),
                            ],
                        }
                    )

                    resp = await stream.read(timeout=1.0)

                    sessionLogVinsRequest = resp.get_only_item(
                        item_type=ItemTypes.UNIPROXY2_DIRECTIVE_SESSION_LOG, proto_type=TUniproxyDirective
                    )
                    assert sessionLogVinsRequest is not None
                    try:
                        mr = resp.get_only_item(item_type=ItemTypes.MEGAMIND_RESPONSE, proto_type=TMegamindResponse)
                    except RuntimeError:
                        logging.exception("not find MEGAMIND_RESPONSE, try next read")
                        resp = await stream.read(timeout=1.0)
                        mr = resp.get_only_item(item_type=ItemTypes.MEGAMIND_RESPONSE, proto_type=TMegamindResponse)

                assert json.loads(mr.data.RawJsonResponse) == MM_RESPONSE_JSON_STATELESS

                context_save_request = resp.get_only_item(
                    item_type=ItemTypes.CONTEXT_SAVE_REQUEST, proto_type=TContextSaveRequest
                )
                context_save_important_request = resp.get_only_item(
                    item_type=ItemTypes.CONTEXT_SAVE_IMPORTANT_REQUEST, proto_type=TContextSaveRequest
                )

                all_speechkit_directives = sorted(
                    list(context_save_request.data.Directives) + list(context_save_important_request.data.Directives),
                    key=lambda d: d.Name,
                )
                assert (
                    sorted(mr.data.ProtoResponse.VoiceResponse.Directives, key=lambda d: d.Name)
                    == all_speechkit_directives
                )

                speechkit_directives = context_save_request.data.Directives
                assert len(speechkit_directives) == 2
                assert speechkit_directives[0].Type == "uni_action"
                assert speechkit_directives[0].Name == "gachi1"
                assert speechkit_directives[0].Payload["actor"] == "ricardo milos"
                assert speechkit_directives[0].Payload["money"] == "three hundred bucks"
                assert speechkit_directives[1].Type == "uni_action"
                assert speechkit_directives[1].Name == "gachi2"
                assert speechkit_directives[1].Payload["actor"] == "billy herrington"
                assert speechkit_directives[1].Payload["money"] == "infinity"

                important_speechkit_directives = context_save_important_request.data.Directives
                assert len(important_speechkit_directives) == 1
                assert important_speechkit_directives[0].Type == "uni_action"
                assert important_speechkit_directives[0].Name == "save_user_audio"
                assert important_speechkit_directives[0].Payload["key"] == "test_edge_flag"

                context_save_directives = context_save_request.data.ContextSaveDirectives
                assert len(context_save_directives) == 2
                assert context_save_directives[0].DirectiveId == "one"
                assert context_save_directives[1].DirectiveId == "two"

                important_context_save_directives = context_save_important_request.data.ContextSaveDirectives
                assert len(important_context_save_directives) == 0

                assert resp.get_flags() == ["context_save_important_need_full_incoming_audio"]

                finished.set_result(None)
            except Exception as exc:
                finished.set_exception(exc)
                assert False, "Error threw"

        try:
            # mocked Megamind server
            with QueuedTcpServer() as srv:
                loop.add_callback(partial(run, port=srv.port))

                stream = await srv.pop_stream()
                request = await read_http_request(stream, with_body=True)

                assert request.method == "POST"
                assert request.uri == "/speechkit/app/pa"  # run suffix

                if use_protobuf:
                    proto = TSpeechKitRequestProto()
                    proto.MergeFromString(request.body)
                    body = json.loads(json_format.MessageToJson(proto))
                else:
                    body = json.loads(request.body.decode())

                assert body["memento"] == base64.b64encode(b'MEMCHIKI)))0))0)').decode()
                assert json.loads(body["request"]["raw_personal_data"]) == json.loads(resource.find('datasync_response_parsed').decode("utf-8"))

                modified_iot_data = QUASAR_IOT_DATA
                modified_iot_data.RawUserInfo = ""
                assert body["iot_user_info_data"] == base64.b64encode(modified_iot_data.SerializeToString()).decode()
                assert body["request"]["smart_home"] == {
                    "payload": {
                        "colors": [
                            {
                                "id": "ololo",
                                "name": "trololo",
                            }
                        ],
                    },
                }

                assert body["request"]["notification_state"] == {
                    "count_archived": 228,
                    "subscriptions": [
                        {
                            "id": "dva",
                            "timestamp": "dva",
                            "name": "vosem",
                        }
                    ],
                }

                assert body["request"]["additional_options"] == {
                    "bass_options": {
                        "client_ip": "228.228.228.228",
                    },
                    "do_not_use_user_logs": False,
                    "expboxes": "XBOX ONE",
                    "icookie": "i-cookie",
                    "puid": "puid",
                    "yandex_uid": "yuid",
                }

                assert body["request"]["laas_region"]["city_id"] == 213

                exps_proto = TExperimentsProto()
                json_format.ParseDict(body["request"]["Experiments"], exps_proto)

                assert exps_proto.Storage["123-32"].String == "91"
                assert exps_proto.Storage["456-32"].Integer == 424
                assert exps_proto.Storage["my_mega_unique_flag_from_requset_payload"].String == "feature_enabled"

                assert body["request"]["test_ids"] == ["6973", "10205", "-1"]

                assert body["session"] == base64.b64encode(b"best session").decode()

                resp_str = json.dumps(MM_RESPONSE_JSON).encode()
                await stream.write(
                    b"HTTP/1.1 200 OK\r\n"
                    b"Content-Length: " + str(len(resp_str)).encode() + b"\r\n"
                    b"Connection: Keep-Alive\r\n"
                    b"\r\n" + resp_str
                )
        except:
            logging.exception("server failed")
            assert False, "Error threw"
        finally:
            await finished
