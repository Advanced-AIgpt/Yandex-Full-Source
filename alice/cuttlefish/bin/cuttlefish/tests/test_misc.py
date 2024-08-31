from .common import Cuttlefish, create_grpc_request
from .common.constants import ItemTypes, ServiceHandles
from .common.items import create_synchronize_state_event
from alice.cuttlefish.library.protos.session_pb2 import TSessionContext
import os
import urllib.request
import pytest
import time


@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish(
        args=[
            "-V",
            "server.rtlog.file_stat_check_period=0.1s",
        ]
    ) as x:
        yield x


def reformat_guid(guid):
    guid = guid.replace("-", "")
    return f"{guid[:8]}-{guid[8:12]}-{guid[12:16]}-{guid[16:20]}-{guid[20:]}"


# -------------------------------------------------------------------------------------------------
def test_logs_rotation(cuttlefish: Cuttlefish):
    def make_request_and_check_logs(guid, after_reopen_log=False):
        cuttlefish.make_grpc_request(
            handle=ServiceHandles.SYNCHRONIZE_STATE_PRE,
            request=create_grpc_request(
                Guid=guid,
                items=[
                    {"type": ItemTypes.SESSION_CONTEXT, "data": TSessionContext()},
                    {"type": ItemTypes.SYNCRHONIZE_STATE_EVENT, "data": create_synchronize_state_event()},
                ],
            ),
        )

        eventlog = [event for event in cuttlefish.get_eventlog(from_beginning=True)]
        request_start_messages = 0
        has_end_reopen_log_event = False
        for event in eventlog:
            if event["EventBody"]["Type"] == "CuttlefishServiceFrame":
                request_start_messages += 1
                assert event["EventBody"]["Fields"]["GUID"] == f"{reformat_guid(guid)}"
            elif event["EventBody"]["Type"] == "EndReopenLog":
                has_end_reopen_log_event = True

        assert request_start_messages == 1
        assert not after_reopen_log or has_end_reopen_log_event

        rtlog = [r for r in cuttlefish.get_rtlog(from_beginning=True)]
        assert f"{reformat_guid(guid)}" in rtlog[1]["EventBody"]["Fields"]["Message"]

    make_request_and_check_logs("01234567-89abcdef-01234567-89abcdef")

    # rotate eventlog
    os.rename(cuttlefish.eventlog_path, cuttlefish.eventlog_path + ".1")
    os.rename(cuttlefish.rtlog_path, cuttlefish.rtlog_path + ".1")
    urllib.request.urlopen(f"http://{cuttlefish.http_endpoint}/admin?action=reopenlog")
    time.sleep(0.2)  # double rtlog's check period to be sure in rotation

    make_request_and_check_logs("aaaaaaaa-bbbbbbbb-cccccccc-dddddddd", after_reopen_log=True)
