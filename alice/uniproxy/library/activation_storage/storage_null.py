import tornado.gen

from .common import milliseconds


class ActivationStorageNull(object):
    def __init__(self, uid, device_id, spotter_features, number_of_speakers=-1, timestamp=None, fake=False):
        self._uid = uid
        self._device_id = device_id
        self._spotter_features = spotter_features
        self._timestamp = milliseconds() if timestamp is None else timestamp
        self._timings = {}
        self._speakers = number_of_speakers

    def start(self):
        pass

    def make_log(self):
        rval = {
            'Timestamp': self._timestamp,
            'ActivatedTimestamp': self._timestamp,
            'FinishTimestamp': milliseconds(),
            'AvgRMS': self._spotter_features.AvgRMS,
            'ActivatedRMS': self._spotter_features.AvgRMS,
            'DeviceId': self._device_id,
            'MultiActivationReason': 'SmartActivationDisabled',
            'ActivatedDeviceId': self._device_id,
            'AnotherLouderSpotter': False,
            'Speakers': self._speakers,
            'YandexUid': self._uid,
        }
        if len(self._timings) > 0:
            rval['Timings'] = {'Stages': self._timings}
        return rval

    @tornado.gen.coroutine
    def activate_co(self):
        return True, self.make_log()

    def activate(self, uid, device_id, final: bool = False, **kwargs) -> bool:
        if not final:
            self._timings['OnCheckedSpotter'] = milliseconds()
        return self.activate_co()

    def cancel(self):
        pass

    @tornado.gen.coroutine
    def unlock_final(self, wait_ts):
        pass
