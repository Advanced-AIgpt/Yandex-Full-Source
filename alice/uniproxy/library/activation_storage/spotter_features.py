import collections
import math


def average(src) -> float:
    summ = 0.0
    count = 0
    for f in src:
        summ += float(f)
        count += 1
    return summ / max(1, count)


# array of arrays of last couple seconds of rawmics 1/16 sec RMSes
def _calc_avg_rms_v0(rms) -> float:
    return average(map(average, rms))


# {'version': integer, 'channels': [{'name':string,'data':array},...]}
# values in data is integers resulted from inintial values muliplied by 100
def _calc_avg_rms_v1(rms) -> float:
    avg = 0.0
    if rms['version'] == 1:
        avg = average(map(lambda channel: average(channel['data']), rms['channels']))

    avg_rms = avg / 100.0
    if math.isnan(avg_rms):
        avg_rms = 0.0

    return avg_rms


def _calc_avg_rms(rms) -> float:
    try:
        if isinstance(rms, collections.abc.Mapping):
            return _calc_avg_rms_v1(rms)
        if isinstance(rms, list):
            return _calc_avg_rms_v0(rms)
    except:
        pass

    return 0.0


class SpotterFeatures:
    def __init__(self):
        self.AvgRMS = 0

    def with_avg_rms(self, value, coefficient=1.0):
        self.AvgRMS = float(value) * coefficient
        return self

    def with_rms(self, value, coefficient=1.0):
        return self.with_avg_rms(_calc_avg_rms(value), coefficient=coefficient)
