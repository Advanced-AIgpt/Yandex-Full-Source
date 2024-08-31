from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse
from alice.cuttlefish.library.protos.context_save_pb2 import TContextSaveRequest, TContextSaveResponse
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext, TRequestContext
from alice.cuttlefish.library.python.apphost_message.packing import extract_protobuf, pack_protobuf
from alice.uniproxy.library.backends_common.context_save import AppHostedContextSave
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter
from apphost.lib.grpc.protos.service_pb2 import TServiceRequest, TServiceResponse

import common
import logging
import tornado.concurrent


def get_metrics_dict():
    metrics = GlobalCounter.get_metrics()
    metrics_dict = {}
    for it in metrics:
        metrics_dict[it[0]] = it[1]
    return metrics_dict


def clean_metrics():
    UniproxyCounter.init()
    for key in set(get_metrics_dict()):
        getattr(GlobalCounter, key.upper()).set(0)


class FakeLogger:
    def __init__(self):
        self._logs = []

    def log_directive(self, directive):
        self._logs.append(directive)


class FakeSystem:
    def __init__(self):
        self.puid = "some-puid"
        self.app_id = "some-app-id"
        self.logger = FakeLogger()


class SimpleProcessor:
    def func1(self, a, b, c):
        return a + b + c

    def func2(self, a, b):
        return a * b

    def func3(self, name):
        return f"Hello, {name}!"


def add_directives(simple_processor, context_save):
    context_save.add_directive(
        {  # directive
            "name": "mark_notification_as_read",
            "payload": {
                "zachem": "tak nado",
            },
        },
        simple_processor.func1,  # func
        1, 2, 3,  # args
    )

    context_save.add_directive(
        {  # directive
            "name": "personal_cards",
            "payload": {
                "hello": "world",
            },
        },
        simple_processor.func2,  # func
        4, 5,  # args
    )

    context_save.add_directive(
        {  # directive
            "name": "push_message",
            "payload": {
                "sample": "text",
            },
        },
        simple_processor.func3,  # func
        "world",  # args
    )


@common.run_async
async def test_context_save_not_launched():
    clean_metrics()

    simple_processor = SimpleProcessor()
    context_save = AppHostedContextSave(FakeSystem(), {})
    add_directives(simple_processor, context_save)

    unex_funcs = context_save.unexecuted_funcs
    assert len(unex_funcs) == 3

    resp = []
    for func, args in unex_funcs:
        resp.append(func(*args))
    assert resp == [6, 20, "Hello, world!"]


@common.run_async
async def test_context_save_all_good():
    clean_metrics()

    simple_processor = SimpleProcessor()
    context_save = AppHostedContextSave(FakeSystem(), {"exp1": "val1"})
    add_directives(simple_processor, context_save)

    loop = tornado.ioloop.IOLoop.current()
    finished = tornado.concurrent.Future()

    async def run():
        try:
            await context_save.execute(user_ticket="some_ticket", message_id="some_message_id")
            assert not context_save.unexecuted_funcs
            finished.set_result(None)
        except Exception as exc:
            finished.set_exception(exc)

    try:
        with common.QueuedTcpServer() as srv, common.ConfigPatch({"apphost": {"url": f"http://localhost:{srv.port}"}}):
            loop.add_callback(run)

            stream = await srv.pop_stream()
            request = await common.read_http_request(stream, with_body=True)

            # check request
            assert request.method == "POST"
            assert request.path == "/context_save"

            sr = TServiceRequest()
            sr.MergeFromString(request.body)

            ans = sr.Answers
            assert len(ans) == 4

            assert ans[0].Type == "context_load_response"
            cl = extract_protobuf(ans[0].Data, TContextLoadResponse)
            assert cl.UserTicket == "some_ticket"

            assert ans[1].Type == "request_context"
            rc = extract_protobuf(ans[1].Data, TRequestContext)
            assert len(rc.ExpFlags) == 1
            assert rc.ExpFlags["exp1"] == "val1"

            assert ans[2].Type == "session_context"
            sc = extract_protobuf(ans[2].Data, TSessionContext)
            assert sc.UserInfo.Puid == "some-puid"
            assert sc.AppId == "some-app-id"

            assert ans[3].Type == "context_save_request"
            csr = extract_protobuf(ans[3].Data, TContextSaveRequest)
            assert len(csr.Directives) == 3
            for num, name, payload in [
                (0, "mark_notification_as_read", b"\n\x14\n\x06zachem\x12\n\x1a\x08tak nado"),
                (1, "personal_cards", b"\n\x10\n\x05hello\x12\x07\x1a\x05world"),
                (2, "push_message", b"\n\x10\n\x06sample\x12\x06\x1a\x04text"),
            ]:
                sd = csr.Directives[num]
                assert sd.Name == name
                assert sd.Payload.SerializeToString() == payload

            # write response
            save_resp = TContextSaveResponse()
            assert not save_resp.FailedDirectives  # nothing failed
            assert not save_resp.FailedMegamindSession

            sr = TServiceResponse()
            answer = sr.Answers.add()
            answer.Type = "context_save_response"
            answer.Data = pack_protobuf(save_resp)
            sr_str = sr.SerializeToString()

            await stream.write(
                b"HTTP/1.1 200 OK\r\n"
                b"Content-Length: " + str(len(sr_str)).encode() + b"\r\n"
                b"Connection: Keep-Alive\r\n"
                b"\r\n" +
                sr_str
            )
    except:
        logging.exception("server failed")
    finally:
        await finished
