from alice.uniproxy.library import testing
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.processors.vins import ContextLoadResponseWrap
from tornado.concurrent import Future
import base64
import json
import logging

from alice.cuttlefish.library.protos.context_load_pb2 import TContextLoadResponse
from alice.cachalot.api.protos.cachalot_pb2 import TResponse, TMegamindSessionLoadResponse
from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from apphost.lib.proto_answers.http_pb2 import THttpResponse
from alice.memento.proto.api_pb2 import TRespGetAllObjects, TUserConfigs
from alice.megamind.protos.scenarios.notification_state_pb2 import TNotificationState, TNotification


def get_metrics_dict():
    metrics = GlobalCounter.get_metrics()
    metrics_dict = {}
    for it in metrics:
        metrics_dict[it[0]] = it[1]
    return metrics_dict


def clean_metrics():
    for key in set(get_metrics_dict()):
        getattr(GlobalCounter, key.upper()).set(0)


all_related_metrics = (
    "cld_apply_personal_data_ok_summ",
    "cld_apply_personal_data_err_summ",
    "cld_apply_memento_ok_summ",
    "cld_apply_memento_err_summ",
    "cld_apply_notificator_ok_summ",
    "cld_apply_notificator_err_summ",
    "cld_apply_vins_session_ok_summ",
    "cld_apply_vins_session_err_summ",
    "cld_apply_smart_home_ok_summ",
    "cld_apply_smart_home_err_summ",
    "cld_apply_flags_json_ok_summ",
    "cld_apply_flags_json_err_summ",
    "cld_apply_laas_ok_summ",
    "cld_apply_laas_err_summ",
    "cld_apply_cancelled_summ",
)


def check_metrics(expected={}):
    metrics = get_metrics_dict()
    for m in all_related_metrics:
        assert metrics[m] == expected.get(m, 0)


resp1 = TContextLoadResponse(
    MementoResponse=THttpResponse(
        StatusCode=200,
        Content=TRespGetAllObjects(  # "CgA=" in b64
            UserConfigs=TUserConfigs()
        ).SerializeToString()
    ),
    QuasarIotResponse=THttpResponse(
        StatusCode=202,
        Content=TIoTUserInfo(
            RawUserInfo=b"""{"key": "value"}"""
        ).SerializeToString()
    ),
    IoTUserInfo=TIoTUserInfo(
        Households=[
            TIoTUserInfo.THousehold(Longitude=28, Latitude=0.8),
        ],
        RawUserInfo=b"""{"key2": "value2"}"""
    ),
    NotificatorResponse=THttpResponse(
        StatusCode=200,
        Content=TNotificationState(
            Notifications=[
                TNotification(Text="get up"),
                TNotification(Text="you moron")
            ]
        ).SerializeToString()
    ),
    MegamindSessionResponse=TResponse(
        MegamindSessionLoadResp=TMegamindSessionLoadResponse(
            Data=b"ABABABABABAB"  # "QUJBQkFCQUJBQkFC" in b64
        )
    ),
    FlagsJsonResponse=THttpResponse(
        StatusCode=200,
        Content=json.dumps({
            "flags_json_version": "1574",
            "all": {
                "CONTEXT": {
                    "MAIN": {
                        "VOICE": {
                            "flags": ["flag-1", "flag-2"]
                        },
                        "ASR": {
                            "flags": ["asr-flag"]
                        }
                    }
                },
                "TESTID": [
                    "375478",
                    "329372",
                ]
            },
            "reqid": "1628534291733267-9520574839949052944-sas2-0238-sas-l7-balancer-8080-BAL-3634",
            "exphandler": "uniproxy",
            "exp_config_version": "16197",
            "is_prestable": 1,
            "version_token": "CAIQ7NXI664vGPzMu,,",
            "ids": [
                "375478",
                "329372",
            ],
            "exp_boxes": "375478,0,59;329372,0,1",
        }).encode("ascii")
    ),
    LaasResponse=THttpResponse(
        StatusCode=200,
        Content=json.dumps({
            "laas_key": "laas_value"
        }).encode("ascii")
    )
)


# -------------------------------------------------------------------------------------------------
@testing.ioloop_run
def test_coros_on_empty():
    clean_metrics()

    wrap = ContextLoadResponseWrap(None, logging.getLogger())

    ret = yield wrap.get_smart_home()
    assert ret is None

    ret = yield wrap.get_notificator()
    assert ret is None

    ret = yield wrap.get_memento()
    assert ret is None

    check_metrics()


# -------------------------------------------------------------------------------------------------
@testing.ioloop_run
def test_coros_on_resp1():
    clean_metrics()

    fut = Future()
    wrap = ContextLoadResponseWrap(fut, logging.getLogger())
    fut.set_result(resp1)

    ret = yield wrap.get_smart_home()
    assert ret == (base64.b64encode(TIoTUserInfo(
        Households=[
            TIoTUserInfo.THousehold(Longitude=28, Latitude=0.8),
        ],
    ).SerializeToString()).decode('utf-8'), {"key2": "value2"})

    ret = yield wrap.get_memento()
    assert ret == "CgA="

    ret = yield wrap.get_notificator()
    assert ret == TNotificationState(Notifications=[TNotification(Text="get up"), TNotification(Text="you moron")])

    check_metrics({
        "cld_apply_memento_ok_summ": 1,
        "cld_apply_smart_home_ok_summ": 1,
        "cld_apply_notificator_ok_summ": 1
    })

    # cancellation means nothing for completed feature
    wrap.set_cancelled()
    assert fut.result() is not None
    check_metrics({
        "cld_apply_memento_ok_summ": 1,
        "cld_apply_smart_home_ok_summ": 1,
        "cld_apply_notificator_ok_summ": 1
    })


# -------------------------------------------------------------------------------------------------
@testing.ioloop_run
def test_cancellation():
    clean_metrics()

    fut = Future()
    wrap = ContextLoadResponseWrap(fut, logging.getLogger())

    wrap.set_cancelled()
    assert fut.result() is None

    ret = yield wrap.get_smart_home()
    assert ret is None

    ret = yield wrap.get_memento()
    assert ret is None

    ret = yield wrap.get_notificator()
    assert ret is None

    ret = yield wrap.get_flags_json()
    assert ret is None

    ret = yield wrap.get_laas()
    assert ret is None

    check_metrics({"cld_apply_cancelled_summ": 1})


# -------------------------------------------------------------------------------------------------
@testing.run_async(timeout=3)
async def test_async_on_empty():
    clean_metrics()

    wrap = ContextLoadResponseWrap(None, logging.getLogger())
    ret = await wrap.get_vins_session()
    assert ret == (0, None)

    check_metrics()


# -------------------------------------------------------------------------------------------------
@testing.run_async(timeout=3)
async def test_async_on_resp1():
    clean_metrics()

    fut = Future()
    wrap = ContextLoadResponseWrap(fut, logging.getLogger())
    fut.set_result(resp1)

    hash, session = await wrap.get_vins_session()
    assert session == "QUJBQkFCQUJBQkFC"

    ret = await wrap.get_flags_json()
    assert ret
    assert ret.get_all_flags()["flag-1"] == "1"

    ret = await wrap.get_laas()
    assert ret == {"laas_key": "laas_value"}

    check_metrics({
        "cld_apply_vins_session_ok_summ": 1,
        "cld_apply_laas_ok_summ": 1,
        "cld_apply_flags_json_ok_summ": 1
    })

    # cancellation means nothing for completed feature
    wrap.set_cancelled()
    assert fut.result() is not None
    check_metrics({
        "cld_apply_vins_session_ok_summ": 1,
        "cld_apply_laas_ok_summ": 1,
        "cld_apply_flags_json_ok_summ": 1
    })
