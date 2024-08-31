from alice.uniproxy.library.vins.vinsadapter import VinsTimings
from alice.uniproxy.library.vins.vinsrequest import VinsRequestTimings
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings, Unistat
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
import alice.uniproxy.library.perf_tester.events as events
from enum import IntEnum, unique
import time


class FakeUnisystem:
    closed = False


class FakeVinsRequest:
    @unique
    class RequestType(IntEnum):
        Run = 0,
        Apply = 1

    def __init__(self):
        self.timings = VinsRequestTimings()
        self.type = self.RequestType.Run


def get_metrics_dict():
    metrics = GlobalTimings.get_metrics()
    metrics_dict = {}
    for it in metrics:
        metrics_dict[it[0]] = it[1]
    return metrics_dict


def count_events(metric_name, metrics):
    return sum([it[1] for it in metrics[metric_name]])


def test_vins_timings_run_only():
    UniproxyCounter.init()
    UniproxyTimings.init()
    unistat = Unistat(FakeUnisystem())
    is_quasar = True
    unistat.is_quasar = is_quasar
    currtime = time.monotonic()
    vins_timings_start_ts = currtime + 2
    vins_timings = VinsTimings(request_start_time_sec=vins_timings_start_ts, epoch=currtime + 1, vins_partial=True,
                               is_quasar=is_quasar, unistat=unistat)
    assert(vins_timings.get_event(events.EventHasVinsFullResultOnEOU) is None)
    assert(vins_timings.get_event(events.EventEndOfUtterance) is None)

    vins_timings.on_request_prepared(currtime + 2, currtime + 3)
    assert(vins_timings._vins_preparing_requests == [1])

    vins_timings.on_request(currtime + 3, currtime + 5)
    assert(vins_timings._vins_requests == [2])
    assert(vins_timings.get_event(events.EventHasVinsFullResultOnEOU) is True)
    assert(vins_timings.get_event(events.EventVinsRunWaitAfterEOUDurationSec) is None)
    assert(vins_timings.get_event(events.EventVinsRunDelayAfterEOUDurationSec) == 0)

    asr_end = currtime + 6
    vins_timings.on_event(events.EventEndOfUtterance, timestamp_sec=asr_end)
    assert(vins_timings.get_event(events.EventHasVinsFullResultOnEOU) is True)
    assert(vins_timings.get_event(events.EventVinsRunWaitAfterEOUDurationSec) is None)
    assert(vins_timings.get_event(events.EventVinsRunDelayAfterEOUDurationSec) == 0)

    vins_timings.to_global_counters()
    assert(GlobalCounter.VINS_FULL_RESULT_READY_ON_EOU_SUMM.value() == 1)
    assert(GlobalCounter.VINS_FULL_RESULT_NOT_READY_ON_EOU_SUMM.value() == 0)

    vins_timings.on_request(currtime + 8, currtime + 11)
    assert(vins_timings._vins_requests == [2, 3])
    assert(vins_timings.get_event(events.EventHasVinsFullResultOnEOU) is False)
    assert(vins_timings.get_event(events.EventVinsRunWaitAfterEOUDurationSec) == 5)
    assert(vins_timings.get_event(events.EventVinsRunDelayAfterEOUDurationSec) == 5)

    vins_timings.to_global_counters()
    assert(GlobalCounter.VINS_FULL_RESULT_READY_ON_EOU_SUMM.value() == 1)
    assert(GlobalCounter.VINS_FULL_RESULT_NOT_READY_ON_EOU_SUMM.value() == 1)

    vins_timings.on_request_prepared(currtime + 6, currtime + 9)
    d = vins_timings.to_dict()
    assert d[events.EventMeanVinsPreparingRequestDurationSec.NAME] == 2

    vins_request = FakeVinsRequest()
    vins_request.timings.prepare_request.update({
        events.EventUsefulVinsPrepareRequestAsr.NAME: 0.1,
        events.EventUsefulVinsPrepareRequestClassify.NAME: 0.1,
        events.EventUsefulVinsPrepareRequestMusic.NAME: 0.1,
        events.EventUsefulVinsPrepareRequestSession.NAME: 0.1,
        events.EventUsefulVinsPrepareRequestYabio.NAME: 0.1,
    })
    vins_request.timings.start_ts = vins_timings_start_ts + 3
    vins_request.timings.begin_request_ts = vins_request.timings.start_ts + 3
    vins_request.timings.end_request_ts = vins_request.timings.start_ts + 4
    vins_timings.on_select_useful_request(vins_request)
    vins_timings.useful_partial = True
    vins_timings.to_global_counters()
    metrics_dict = get_metrics_dict()
    assert count_events(events.EventUsefulVinsPrepareRequestAsr.NAME + '_quasar_hgram', metrics_dict) == 1
    assert count_events(events.EventUsefulVinsPrepareRequestClassify.NAME + '_quasar_hgram', metrics_dict) == 1
    assert count_events(events.EventUsefulVinsPrepareRequestMusic.NAME + '_quasar_hgram', metrics_dict) == 1
    assert count_events(events.EventUsefulVinsPrepareRequestSession.NAME + '_quasar_hgram', metrics_dict) == 1
    assert count_events(events.EventUsefulVinsPrepareRequestYabio.NAME + '_quasar_hgram', metrics_dict) == 1
    assert count_events('user_useful_partial_to_asr_end_quasar_hgram', metrics_dict) == 1
    assert count_events(events.EventUsefulVinsRequestDuration.NAME + '_quasar_hgram', metrics_dict) == 1

    d = vins_timings.to_dict()
    assert d[events.EventUsefulPartial.NAME] == vins_request.timings.start_ts - vins_timings_start_ts
    assert d.get(events.EventUsefulVinsPrepareRequestClassify.NAME) is not None
    assert d.get(events.EventUsefulVinsPrepareRequestSession.NAME) is not None
    assert d.get(events.EventUsefulVinsPrepareRequestYabio.NAME) is not None


def test_vins_timings_with_apply():
    UniproxyCounter.init()
    UniproxyTimings.init()

    unistat = Unistat(FakeUnisystem())
    currtime = time.time()
    vins_timings = VinsTimings(request_start_time_sec=currtime + 2, epoch=currtime + 1, vins_partial=True,
                               is_quasar=False, unistat=unistat)
    assert(vins_timings.get_event(events.EventHasVinsFullResultOnEOU) is None)
    assert(vins_timings.get_event(events.EventEndOfUtterance) is None)

    vins_timings.on_request(currtime + 3, currtime + 5)
    assert(vins_timings._vins_requests == [2])
    assert(vins_timings.get_event(events.EventHasVinsFullResultOnEOU) is True)
    assert(vins_timings.get_event(events.EventVinsRunWaitAfterEOUDurationSec) is None)
    assert(vins_timings.get_event(events.EventVinsRunDelayAfterEOUDurationSec) == 0)

    vins_timings.on_event(events.EventEndOfUtterance)
    assert(vins_timings.get_event(events.EventHasVinsFullResultOnEOU) is True)
    assert(vins_timings.get_event(events.EventVinsRunWaitAfterEOUDurationSec) is None)
    assert(vins_timings.get_event(events.EventVinsRunDelayAfterEOUDurationSec) == 0)

    vins_timings.on_apply_request(currtime + 6, currtime + 9)
    assert(vins_timings._vins_requests == [2])
    assert(vins_timings.get_event(events.EventLastVinsApplyRequestDurationSec) == 3)
    assert(vins_timings.get_event(events.EventHasVinsFullResultOnEOU) is False)
    # The counter is for run only, so it is not fired.
    assert(vins_timings.get_event(events.EventVinsRunWaitAfterEOUDurationSec) is None)
    assert(vins_timings.get_event(events.EventVinsRunDelayAfterEOUDurationSec) == 0)

    vins_timings.to_global_counters()
    assert(GlobalCounter.VINS_FULL_RESULT_READY_ON_EOU_SUMM.value() == 0)
    assert(GlobalCounter.VINS_FULL_RESULT_NOT_READY_ON_EOU_SUMM.value() == 1)
