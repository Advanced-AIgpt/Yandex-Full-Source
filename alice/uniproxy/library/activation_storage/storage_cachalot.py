from .common import (
    compare_spotters_raw,
    milliseconds,
)

from alice.uniproxy.library.global_counter import GlobalCounter, UnistatTiming
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config

from alice.cachalot.client import CachalotClient
from alice.cachalot.client.activation import ActivationClient, ActivationSubjectInfo

from tornado.ioloop import IOLoop
from tornado.locks import Event
import tornado.gen

from datetime import timedelta
import copy


def make_cachalot_client():
    cachlot_config = config["cachalot"]
    return CachalotClient(cachlot_config["host"], cachlot_config["http_port"])


def is_competitor_better(self: ActivationClient, competitor: ActivationSubjectInfo):
    return compare_spotters_raw(
        l_validated=False,
        l_rms=self.avg_rms,
        l_time=self.timestamp.ToMilliseconds(),
        l_device_id=self.device_id,
        r_validated=False,
        r_rms=competitor.spotter_features.avg_rms,
        r_time=competitor.activation_attempt_time.ToMilliseconds(),
        r_device_id=competitor.device_id,
    )


class ActivationStorageCachalot:
    """
        Adapter class.
        Emulates interface of ActivationStorage2 but uses cachalot instead of memcached.
    """

    def __init__(
        self, user_id, device_id, spotter_features, retry_delay_milliseconds=0,
        freshness_delta_milliseconds=None, allow_activation_by_unvalidated_spotter=False,
        single_step_mode=False
    ):
        if isinstance(user_id, str):
            user_id = user_id.encode("utf-8")

        if isinstance(device_id, str):
            device_id = device_id.encode("utf-8")

        self.client = ActivationClient(
            make_cachalot_client(), user_id, device_id, spotter_features.AvgRMS,
            freshness_delta_milliseconds=freshness_delta_milliseconds,
            allow_activation_by_unvalidated_spotter=allow_activation_by_unvalidated_spotter,
        )
        self._final_stage_unlocked = Event()
        self._better_spotter_found = None
        self._retry_delay_milliseconds = retry_delay_milliseconds
        self._logger = Logger.get(".cachalot.activation")
        self._zero_rms_found = False
        self._spotter_validated = False
        self._single_step_mode = single_step_mode

        self._log_info = {
            "YandexUid": self.client.user_id,
            "DeviceId": self.client.device_id,
            "AvgRMS": self.client.avg_rms,
            "Timestamp": self.client.timestamp.ToMilliseconds(),
            "FinishTimestamp": milliseconds(),
            "ActivatedDeviceId": None,
            "ActivatedTimestamp": None,
            "ActivatedRMS": None,
            "MultiActivationReason": "OK",
            "SpotterValidatedBy": None,
            "ThisSpotterIsValid": None,
            "Timings": {
                "FirstWindowSeconds": 0,
                "SecondWindowMilliseconds": freshness_delta_milliseconds or 2500,
            },
            "AnotherLouderSpotter": None,
            "Key": None,
        }

    def get_log_info(self):
        info = copy.deepcopy(self._log_info)
        info["FinishTimestamp"] = milliseconds()
        return info

    def start(self):
        try:
            with UnistatTiming("cachalot_activation_make_announcement"):
                IOLoop.current().spawn_callback(self.client.make_announcement, False)
                GlobalCounter.CACHALOT_ACTIVATION_FIRST_REQUEST_SUMM.increment()
        except Exception as exc:
            GlobalCounter.CACHALOT_ACTIVATION_FIRST_ERROR_SUMM.increment()
            self._logger.exception(exc)

    async def activate(self, *args, final=False, spotter_validated=True, **kwargs):
        if not final:
            self._spotter_validated = spotter_validated
            self._log_info["ThisSpotterIsValid"] = spotter_validated

            try:
                with UnistatTiming("cachalot_activation_make_announcement"):
                    response = await self.client.make_announcement(spotter_validated=spotter_validated)
                    GlobalCounter.CACHALOT_ACTIVATION_SECOND_REQUEST_SUMM.increment()

                competitor = response.best_competitor

                # Here we hope that better spotter will be validated and
                # this spotter will be canceled after second request.
                # It works only when _retry_delay_milliseconds set by ITS.
                if (
                    response.continuation_allowed and
                    competitor and
                    is_competitor_better(self.client, competitor)
                ):
                    if self._retry_delay_milliseconds > 1:
                        await tornado.gen.sleep(self._retry_delay_milliseconds / 1000)
                        with UnistatTiming("cachalot_activation_make_announcement"):
                            response = await self.client.make_announcement(spotter_validated=spotter_validated)
                            GlobalCounter.CACHALOT_ACTIVATION_SECOND_REQUEST_SUMM.increment()

                if not response.continuation_allowed:
                    if response.leader_found:
                        self._set_reason("LeaderAlreadyElected", 2)
                        GlobalCounter.CACHALOT_ACTIVATION_SECOND_LEADER_ALREADY_ELECTED_SUMM.increment()
                    else:
                        self._set_reason("AnotherIsBetter", 2)
                        GlobalCounter.CACHALOT_ACTIVATION_SECOND_ANOTHER_IS_BETTER_SUMM.increment()

                    if competitor:
                        self._log_info["ActivatedDeviceId"] = competitor.device_id
                        self._log_info["ActivatedTimestamp"] = competitor.activation_attempt_time.ToMilliseconds()
                        self._log_info["ActivatedRMS"] = competitor.spotter_features.avg_rms
                    else:
                        pass  # unreachable
                else:
                    self._log_info["ActivatedDeviceId"] = self.client.device_id
                    self._log_info["ActivatedTimestamp"] = self.client.timestamp.ToMilliseconds()
                    self._log_info["ActivatedRMS"] = self.client.avg_rms
                    GlobalCounter.CACHALOT_ACTIVATION_SECOND_ALLOWED_SUMM.increment()

                self._zero_rms_found = response.zero_rms_found
                return response.continuation_allowed, self.get_log_info()
            except Exception as exc:
                GlobalCounter.CACHALOT_ACTIVATION_SECOND_ERROR_SUMM.increment()
                self._logger.exception(exc)
        else:
            try:
                await self._final_stage_unlocked.wait(timedelta(milliseconds=100))
            except:
                pass

            try:
                with UnistatTiming("cachalot_activation_try_acquire_leadership"):
                    response = await self.client.try_acquire_leadership(ignore_rms=self._zero_rms_found)

                GlobalCounter.CACHALOT_ACTIVATION_THIRD_REQUEST_SUMM.increment()

                self._log_info["SpotterValidatedBy"] = response.spotter_validated_by

                if not response.activation_allowed:
                    if (not response.spotter_validated_by) or (response.spotter_validated_by == "Nobody!"):
                        self._set_reason("NoValidSpotterFound", 3)
                        GlobalCounter.CACHALOT_ACTIVATION_THIRD_ALL_INVALID_SUMM.increment()

                    elif response.leader_info is not None:
                        self._log_info["ActivatedDeviceId"] = response.leader_info.device_id
                        self._log_info["ActivatedTimestamp"] = \
                            response.leader_info.activation_attempt_time.ToMilliseconds()
                        self._log_info["ActivatedRMS"] = response.leader_info.spotter_features.avg_rms

                        if is_competitor_better(self.client, response.leader_info):
                            self._set_reason("AnotherIsBetter", 3)
                            GlobalCounter.CACHALOT_ACTIVATION_THIRD_ANOTHER_IS_BETTER_SUMM.increment()
                        else:
                            if self._spotter_validated:
                                self._set_reason("LeaderAlreadyElected", 3)
                                GlobalCounter.CACHALOT_ACTIVATION_THIRD_LEADER_ALREADY_ELECTED_SUMM.increment()
                            else:
                                self._set_reason("LoudInvalidAndSlow", 3)
                                GlobalCounter.CACHALOT_ACTIVATION_THIRD_LOUD_INVALID_AND_SLOW_SUMM.increment()

                    else:
                        self._set_reason("UnknownLeaderAlreadyElected", 3)
                        GlobalCounter.CACHALOT_ACTIVATION_THIRD_UNKNOWN_LEADER_SUMM.increment()

                elif response.activation_allowed:
                    self._log_info["ActivatedDeviceId"] = self.client.device_id
                    self._log_info["ActivatedTimestamp"] = self.client.timestamp.ToMilliseconds()
                    self._log_info["ActivatedRMS"] = self.client.avg_rms
                    GlobalCounter.CACHALOT_ACTIVATION_THIRD_ALLOWED_SUMM.increment()

                else:
                    pass  # unreachable

                return response.activation_allowed, self.get_log_info()
            except Exception as exc:
                GlobalCounter.CACHALOT_ACTIVATION_THIRD_ERROR_SUMM.increment()
                self._logger.exception(exc)

        # just activate all clients in case of internal error.
        self._set_reason("Internal error")
        GlobalCounter.CACHALOT_ACTIVATION_FATAL_ERROR_SUMM.increment()
        return True, self.get_log_info()

    async def cancel(self, *args, **kwargs):
        pass

    async def unlock_final(self, *args, **kwargs):
        self._final_stage_unlocked.set()

    def _set_reason(self, reason, stage=None):
        if self._single_step_mode:
            reason = reason + " [single step mode]"
        else:
            if stage == 2:
                reason = reason + " (on second stage)"
            elif stage == 3:
                reason = reason + " (on third stage)"

        self._log_info["MultiActivationReason"] = reason


class ActivationStorageCachalotSimple:
    def __init__(self, *args, **kwargs):
        self._client = ActivationStorageCachalot(*args, single_step_mode=True, **kwargs)

    def start(self):
        pass

    async def activate(self, *args, **kwargs):
        await self._client.unlock_final()
        return await self._client.activate(final=True)
