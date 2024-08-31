import pytest
import base64
import common
from alice.uniproxy.library.processors import create_event_processor
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.async_http_client import HTTPResponse, HTTPError
from alice.cachalot.api.protos.cachalot_pb2 import TResponse, EResponseStatus
from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse
from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from apphost.lib.proto_answers.http_pb2 import THttpResponse


def get_metrics_dict():
    metrics = GlobalCounter.get_metrics()
    metrics_dict = {}
    for it in metrics:
        metrics_dict[it[0]] = it[1]
    return metrics_dict


def clean_metrics():
    for key in set(get_metrics_dict()):
        getattr(GlobalCounter, key.upper()).set(0)


def create_cuttlefish_answer(code=None, body=None):
    resp = THttpResponse()
    if code is not None:
        resp.StatusCode = code
    if body is not None:
        resp.Content = body
    return resp


class TestContextLoadDiffCheck:

    fields = [
        ("memento", "MementoResponse", "u2_cld_diff_memento_summ"),
        ("datasync", "DatasyncResponse", "u2_cld_diff_datasync_summ"),
        ("datasync_device_id", "DatasyncDeviceIdResponse", "u2_cld_diff_datasync_device_id_summ"),
        ("datasync_uuid", "DatasyncUuidResponse", "u2_cld_diff_datasync_uuid_summ"),
        ("quasar_iot", "QuasarIotResponse", "u2_cld_diff_quasar_iot_summ"),
        ("notificator", "NotificatorResponse", "u2_cld_diff_notificator_summ"),
    ]

    # empty event
    def test_empty_payload(self):
        clean_metrics()

        event = common.FakeEvent("Uniproxy2", "ContextLoadDiffCheck")
        proc = create_event_processor(common.FakeSystem(), event)
        proc.process_event(event)

        # nothing had broke or changed
        metrics = get_metrics_dict()
        assert metrics["u2_context_load_diff_check_summ"] == 0
        assert metrics["u2_cld_nodiff_summ"] == 0
        for key, value in metrics.items():
            if key.startswith("u2_cld_diff_"):
                assert value == 0

    # empty responses everywhere
    def test_empty_responses(self):
        clean_metrics()

        payload = {}
        payload["requestId"] = "my-lovely-request-id"
        payload["response"] = base64.b64encode(TContextLoadResponse().SerializeToString())

        event = common.FakeEvent("Uniproxy2", "ContextLoadDiffCheck", payload=payload)
        proc = create_event_processor(common.FakeSystem(), event)
        proc.process_event(event)

        # no diffs found
        metrics = get_metrics_dict()
        assert metrics["u2_context_load_diff_check_summ"] == 1
        assert metrics["u2_cld_nodiff_summ"] == 1
        for key, value in metrics.items():
            if key.startswith("u2_cld_diff_"):
                assert value == 0

    # common data
    @pytest.mark.parametrize("storage_hash, proto_field, triggered_metric", fields)
    # uniproxy1 response's class
    @pytest.mark.parametrize("uniproxy1_http_cls", [HTTPResponse, HTTPError])
    # cuttlefish's answer
    @pytest.mark.parametrize("cuttlefish_answer", [
        create_cuttlefish_answer(),
        create_cuttlefish_answer(code=417, body=b"I am a teapot"),
    ])
    def test_single_diff(self, storage_hash, proto_field, triggered_metric, uniproxy1_http_cls, cuttlefish_answer):
        clean_metrics()

        payload = {}
        payload["requestId"] = "my-lovely-request-id"

        context_load_response = TContextLoadResponse()
        getattr(context_load_response, proto_field).MergeFrom(cuttlefish_answer)
        payload["response"] = base64.b64encode(context_load_response.SerializeToString())

        http_resp = uniproxy1_http_cls(code=200, body=b"Test Testovich")
        system = common.FakeSystem()
        system.responses_storage.store("my-lovely-request-id", storage_hash, http_resp)

        event = common.FakeEvent("Uniproxy2", "ContextLoadDiffCheck", payload=payload)
        proc = create_event_processor(system, event)
        proc.process_event(event)

        # one diff found
        metrics = get_metrics_dict()
        assert metrics["u2_context_load_diff_check_summ"] == 1
        assert metrics["u2_cld_nodiff_summ"] == 0
        assert metrics[triggered_metric] == 1
        for key, value in metrics.items():
            if key.startswith("u2_cld_diff_") and key != triggered_metric:
                assert value == 0

        # one log directive written
        logs = system.logger._logs
        assert len(logs) == 1
        log = logs[0][0]

        assert log["type"] == "ContextLoadDiff"
        assert log["ForEvent"] == event.message_id

        # one diff found
        diffs = log["diff"]
        assert len(diffs) == 1
        diff = diffs[0]

        assert diff["hash"] == storage_hash
        assert diff["expected"]["code"] == http_resp.code
        assert diff["expected"]["body"] == http_resp.body.decode()
        assert diff["actual"]["code"] == cuttlefish_answer.StatusCode
        assert diff["actual"]["body"] == cuttlefish_answer.Content.decode()

    # all answers differ
    def test_big_diff(self):
        clean_metrics()

        payload = {}
        payload["requestId"] = "my-lovely-request-id"

        system = common.FakeSystem()
        context_load_response = TContextLoadResponse()

        for storage_hash, proto_field, _ in self.fields:
            uniproxy_resp = HTTPResponse(code=200, body=str.encode("I am not a teapot " + storage_hash))
            system.responses_storage.store("my-lovely-request-id", storage_hash, uniproxy_resp)

            context_load_resp = create_cuttlefish_answer(code=417, body=str.encode("I am a teapot " + proto_field))
            getattr(context_load_response, proto_field).MergeFrom(context_load_resp)

        payload["response"] = base64.b64encode(context_load_response.SerializeToString())

        event = common.FakeEvent("Uniproxy2", "ContextLoadDiffCheck", payload=payload)
        proc = create_event_processor(system, event)
        proc.process_event(event)

        # one diff found
        metrics = get_metrics_dict()
        assert metrics["u2_context_load_diff_check_summ"] == 1
        assert metrics["u2_cld_nodiff_summ"] == 0
        for _, _, metric in self.fields:
            assert metrics[metric] == 1

        # one log directive written
        logs = system.logger._logs
        assert len(logs) == 1
        log = logs[0][0]

        assert log["type"] == "ContextLoadDiff"
        assert log["ForEvent"] == event.message_id

        # multiple partual diffs found
        diffs = log["diff"]
        assert len(diffs) == len(self.fields)

        for i in range(len(self.fields)):
            storage_hash, proto_field, _ = self.fields[i]

            diff = diffs[i]
            assert diff["hash"] == storage_hash
            assert diff["expected"]["code"] == 200
            assert diff["expected"]["body"] == "I am not a teapot " + storage_hash
            assert diff["actual"]["code"] == 417
            assert diff["actual"]["body"] == "I am a teapot " + proto_field

    # no answers differ
    def test_no_diffs(self):
        clean_metrics()

        payload = {}
        payload["requestId"] = "my-lovely-request-id"

        system = common.FakeSystem()
        context_load_response = TContextLoadResponse()

        for storage_hash, proto_field, _ in self.fields:
            uniproxy_resp = HTTPResponse(code=417, body=b"I am a teapot")
            system.responses_storage.store("my-lovely-request-id", storage_hash, uniproxy_resp)

            context_load_resp = create_cuttlefish_answer(code=417, body=b"I am a teapot")
            getattr(context_load_response, proto_field).MergeFrom(context_load_resp)

        payload["response"] = base64.b64encode(context_load_response.SerializeToString())

        event = common.FakeEvent("Uniproxy2", "ContextLoadDiffCheck", payload=payload)
        proc = create_event_processor(system, event)
        proc.process_event(event)

        # no diffs found
        metrics = get_metrics_dict()
        assert metrics["u2_context_load_diff_check_summ"] == 1
        assert metrics["u2_cld_nodiff_summ"] == 1
        for _, _, metric in self.fields:
            assert metrics[metric] == 0

        # no log directive written
        logs = system.logger._logs
        assert len(logs) == 0

    @pytest.mark.parametrize("expected_content, actual_content, has_diff", [
        (b"some user session data!", b"some user session data!", False),
        (b"", b"", False),
        (b"ABACABA", b"ABACAB", True),
    ])
    def test_megamind_session(self, expected_content, actual_content, has_diff):
        clean_metrics()

        # prepare uniproxy1 answer (base64 string)
        system = common.FakeSystem()
        system.responses_storage.store("my-lovely-request-id", "megamind_session", base64.b64encode(expected_content).decode("ascii"))

        # prepare apphost answer (NCachalotProtocol.TResponse object)
        context_load_response = TContextLoadResponse()
        resp = TResponse()
        resp.Status = EResponseStatus.OK
        resp.MegamindSessionLoadResp.Data = actual_content
        context_load_response.MegamindSessionResponse.MergeFrom(resp)

        # send diff check event
        payload = {}
        payload["requestId"] = "my-lovely-request-id"
        payload["response"] = base64.b64encode(context_load_response.SerializeToString())
        event = common.FakeEvent("Uniproxy2", "ContextLoadDiffCheck", payload=payload)
        proc = create_event_processor(system, event)
        proc.process_event(event)

        # check diffs
        metrics = get_metrics_dict()
        assert metrics["u2_context_load_diff_check_summ"] == 1

        if has_diff:
            assert metrics["u2_cld_nodiff_summ"] == 0
            assert metrics["u2_cld_diff_megamind_session_summ"] == 1
        else:
            assert metrics["u2_cld_nodiff_summ"] == 1
            assert metrics["u2_cld_diff_megamind_session_summ"] == 0

    @pytest.mark.parametrize("expected_content, actual_content, has_diff", [
        (
            "oops!!! no json returned",
            "oops!!! no json returned",
            False,
        ),
        (
            "abacaba",
            "babcbab",
            True,
        ),
        (
            """{"status": "ok", "request_id": 111}""",
            """{"status": "ok", "request_id": 222}""",
            False,
        ),
        (
            """{"status": "ok", "another_field": 111}""",
            """{"status": "ok", "another_field": 222}""",
            True,
        ),
        (
            """{"status": "ok", "one_field": 111}""",
            """{"status": "ok", "two_field": 111}""",
            True,
        ),
        (
            """{"status": "ok", "arr": [1, 2, 3]}""",
            """{"status": "ok", "arr": [3, 2, 1]}""",
            False,
        ),
        (
            """{"status": "ok", "arr": [[1, 3, 2], [4, 6, 5], [9, 8, 7]]}""",
            """{"status": "ok", "arr": [[2, 3, 1], [4, 5, 6], [8, 9, 7]]}""",
            False,
        ),
        (
            """{"status": "ok", "arr": [[1, 1, 1], [2, 2]]}""",
            """{"status": "ok", "arr": [[1, 1, 1], [2 ,2 ,2]]}""",
            True,
        ),
        (
            """{"status": "ok", "obj": {"a": [0, 1, 2], "b": ["aa", "bb", "cc"]}}""",
            """{"status": "ok", "obj": {"b": ["cc", "aa", "bb"], "a": [0, 2, 1]}}""",
            False,
        ),
        (
            """{"status": "ok", "obj": {"b": {"e": "f"}, "a": {"c": "d", "j": "h"}}}""",
            """{"status": "ok", "obj": {"a": {"j": "h", "c": "d"}, "b": {"e": "f"}}}""",
            False,
        ),
    ])
    def test_quasar_iot(self, expected_content, actual_content, has_diff):
        clean_metrics()

        # prepare uniproxy1 answer
        resp_proto = TIoTUserInfo()
        resp_proto.RawUserInfo = expected_content

        system = common.FakeSystem()
        system.responses_storage.store("my-lovely-request-id", "quasar_iot", HTTPResponse(code=200, body=resp_proto.SerializeToString()))

        # prepare apphost answer
        context_load_response = TContextLoadResponse()
        resp_proto.RawUserInfo = actual_content
        context_load_response.QuasarIotResponse.MergeFrom(create_cuttlefish_answer(code=200, body=resp_proto.SerializeToString()))

        # send diff check event
        payload = {}
        payload["requestId"] = "my-lovely-request-id"
        payload["response"] = base64.b64encode(context_load_response.SerializeToString())
        event = common.FakeEvent("Uniproxy2", "ContextLoadDiffCheck", payload=payload)
        proc = create_event_processor(system, event)
        proc.process_event(event)

        # check diffs
        metrics = get_metrics_dict()
        assert metrics["u2_context_load_diff_check_summ"] == 1

        if has_diff:
            assert metrics["u2_cld_nodiff_summ"] == 0
            assert metrics["u2_cld_diff_quasar_iot_summ"] == 1
        else:
            assert metrics["u2_cld_nodiff_summ"] == 1
            assert metrics["u2_cld_diff_quasar_iot_summ"] == 0

    @pytest.mark.parametrize("uniproxy_field_name, cuttlefish_field_name, affected_metric", [
        ("datasync", "DatasyncResponse", "u2_cld_diff_datasync_summ"),
        ("datasync_device_id", "DatasyncDeviceIdResponse", "u2_cld_diff_datasync_device_id_summ"),
        ("datasync_uuid", "DatasyncUuidResponse", "u2_cld_diff_datasync_uuid_summ"),
    ])
    @pytest.mark.parametrize("expected_content, actual_content, has_diff", [
        (
            "oops!!! no json returned",
            "oops!!! no json returned",
            False,
        ),
        (
            "abacaba",
            "babcbab",
            True,
        ),
        (
            """{"items":[{"body":12345,"headers":{"cool_header":"cool_value","Yandex-Cloud-Request-ID":85285}}]}""",
            """{"items":[{"body":12345,"headers":{"cool_header":"cool_value","Yandex-Cloud-Request-ID":9285128991}}]}""",
            False,
        ),
        (
            """{"items":[{"body":1,"headers":{"cool_header":"cool_value","Yandex-Cloud-Request-ID":85285}}]}""",
            """{"items":[{"body":2,"headers":{"cool_header":"cool_value","Yandex-Cloud-Request-ID":9285128991}}]}""",
            True,
        ),
        (
            """{"items":[{"body":12345,"headers":{"cool_header":"cool_value","Yandex-Cloud":85285}}]}""",
            """{"items":[{"body":12345,"headers":{"cool_header":"cool_value","Yandex-Cloud":9285128991}}]}""",
            True,
        ),
    ])
    def test_datasync(self, uniproxy_field_name, cuttlefish_field_name, affected_metric, expected_content, actual_content, has_diff):
        clean_metrics()

        # prepare uniproxy1 answer
        system = common.FakeSystem()
        system.responses_storage.store("my-lovely-request-id", uniproxy_field_name, HTTPResponse(code=200, body=str.encode(expected_content)))

        # prepare apphost answer
        context_load_response = TContextLoadResponse()
        getattr(context_load_response, cuttlefish_field_name).MergeFrom(create_cuttlefish_answer(code=200, body=str.encode(actual_content)))

        # send diff check event
        payload = {}
        payload["requestId"] = "my-lovely-request-id"
        payload["response"] = base64.b64encode(context_load_response.SerializeToString())
        event = common.FakeEvent("Uniproxy2", "ContextLoadDiffCheck", payload=payload)
        proc = create_event_processor(system, event)
        proc.process_event(event)

        # check diffs
        metrics = get_metrics_dict()
        assert metrics["u2_context_load_diff_check_summ"] == 1

        if has_diff:
            assert metrics["u2_cld_nodiff_summ"] == 0
            assert metrics[affected_metric] == 1
        else:
            assert metrics["u2_cld_nodiff_summ"] == 1
            assert metrics[affected_metric] == 0
