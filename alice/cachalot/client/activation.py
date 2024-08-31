from google.protobuf.timestamp_pb2 import Timestamp as ProtoTimestamp


class SpotterFeatures:
    def __init__(self, json_data):
        self.avg_rms = json_data["AvgRMS"]
        self.validated = json_data["Validated"]


class ActivationSubjectInfo:
    def __init__(self, json_data):
        self.user_id = json_data["UserId"]
        self.device_id = json_data["DeviceId"]
        self.spotter_features = SpotterFeatures(json_data["SpotterFeatures"])

        # Warning! Field ActivationAttemptTime a string (ISO 8601).
        self.activation_attempt_time = ProtoTimestamp()
        self.activation_attempt_time.FromJsonString(json_data["ActivationAttemptTime"])


class ActivationAnnouncementResponse:
    def __init__(self, json_response):
        rsp = json_response["ActivationAnnouncement"]
        self.continuation_allowed = bool(rsp["ContinuationAllowed"])
        self.error_msg = rsp["Error"]
        self.zero_rms_found = bool(rsp["ZeroRmsFound"])
        self.leader_found = bool(rsp["LeaderFound"])

        if "BestCompetitor" in rsp:
            self.best_competitor = ActivationSubjectInfo(rsp["BestCompetitor"])
        else:
            self.best_competitor = None


class ActivationFinalResponse:
    def __init__(self, json_response):
        rsp = json_response["ActivationFinal"]
        self.activation_allowed = bool(rsp["ActivationAllowed"])
        self.error_msg = rsp["Error"]
        self.spotter_validated_by = rsp["SpotterValidatedBy"]

        if "LeaderInfo" in rsp:
            self.leader_info = ActivationSubjectInfo(rsp["LeaderInfo"])
        else:
            self.leader_info = None


class ActivationClientException(Exception):
    pass


class ActivationClientAlreadyRejectedException(ActivationClientException):
    pass


class ActivationClientBase:
    def __init__(
        self,
        cachalot_client,
        user_id,
        device_id,
        avg_rms,
        freshness_delta_milliseconds=None,
        raise_on_already_rejected=True,
    ):
        """
            cachalot_client is one of (CachalotClient, SyncCachalotClient).
            Its methods will be awaited in ActivationClient or just synchronously called in SyncActivationClient.
        """

        # constant memebers
        self.cachalot_client = cachalot_client
        self.user_id = user_id
        self.device_id = device_id
        self.avg_rms = avg_rms
        self.timestamp = ProtoTimestamp()
        self.timestamp.GetCurrentTime()
        self.freshness_delta_milliseconds = freshness_delta_milliseconds
        self.raise_on_already_rejected = raise_on_already_rejected

        # runtime params
        self.rejected = False
        self.zero_rms_found = False

    def _activation_announcement(self, spotter_validated):
        if self.raise_on_already_rejected and self.rejected:
            raise ActivationClientAlreadyRejectedException()

        rsp = ActivationAnnouncementResponse(self.cachalot_client.activation_announcement(
            self.user_id, self.device_id, self.avg_rms, self.timestamp, spotter_validated,
            freshness_delta_milliseconds=self.freshness_delta_milliseconds,
        ))
        self.rejected = bool(self.rejected or not rsp.continuation_allowed)
        self.zero_rms_found = bool(self.zero_rms_found or rsp.zero_rms_found)
        return rsp

    def _activation_final(self):
        if self.raise_on_already_rejected and self.rejected:
            raise ActivationClientAlreadyRejectedException()

        return ActivationFinalResponse(self.cachalot_client.activation_final(
            self.user_id, self.device_id, self.avg_rms, self.timestamp, ignore_rms=self.zero_rms_found,
            freshness_delta_milliseconds=self.freshness_delta_milliseconds,
        ))


class ActivationClient(ActivationClientBase):
    async def make_announcement(self, *args, **kwargs):
        return await self._activation_announcement(*args, **kwargs)

    async def try_acquire_leadership(self, *args, **kwargs):
        return await self._activation_final(*args, **kwargs)


class SyncActivationClient(ActivationClientBase):
    def make_announcement(self, *args, **kwargs):
        return self._activation_announcement(*args, **kwargs)

    def try_acquire_leadership(self, *args, **kwargs):
        return self._activation_final(*args, **kwargs)
