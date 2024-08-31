# coding: utf-8

import enum
import logging
import typing
import sys


logger = logging.getLogger(__name__)


class _UnistatMetric(object):
    def serialize(self):
        raise NotImplementedError()


class _Histogram(_UnistatMetric):
    """Гистограмма unistat. Используется для агрегации временных данных."""
    def __init__(self, bounds):
        """
        Buckets – массив левых границ интервалов, для времени обычно начинается с 0.
        """
        # type: typing.List[float] -> None
        if not bounds:
            raise ValueError('buckets should not be empty')
        self.buckets = tuple([bound, 0] for bound in bounds)

    def write_value(self, value):
        # type: (float) -> None
        if value < self.buckets[0][0]:
            logger.warn('Value subbmitted to histogram is less than left bound: %f < %f', value, self.buckets[0])
            return
        for i, (left_bucket_bound, counter) in enumerate(self.buckets):
            right_bucket_bound = self.buckets[i + 1][0] if i < (len(self.buckets) - 1) else sys.maxint
            if left_bucket_bound <= value < right_bucket_bound:
                self.buckets[i][1] += 1
                return

    def serialize(self):
        return self.buckets


class _Counter(_UnistatMetric):

    def __init__(self):
        self.value = 0

    def incr(self, value=1):
        self.value += value

    def serialize(self):
        return self.value


class HistogramName(enum.Enum):
    wizard_response_time = 'wizard_response_time_hgram'


class CounterName(enum.Enum):
    wizard_requests_total = 'wizard_requests_total_summ'
    wizard_requests_2xx = 'wizard_requests_2xx_summ'
    wizard_requests_3xx = 'wizard_requests_3xx_summ'
    wizard_requests_4xx = 'wizard_requests_4xx_summ'
    wizard_requests_5xx = 'wizard_requests_5xx_summ'
    wizard_requests_timeout = 'wizard_requests_timeout_summ'
    wizard_failed_unique = 'wizard_failed_unique_summ'  # number of unique failed API requests without retries
    wizard_retry_count = 'wizard_retry_count_summ'

    api_ner_requests_total = 'api_ner_requests_total_summ'
    api_ner_bad_requests = 'api_ner_bad_requests_summ'
    api_ner_errors = 'api_ner_errors_summ'

    entities_found_datetime = 'entities_found_datetime_summ'
    entities_found_number = 'entities_found_number_summ'
    entities_found_geo = 'entities_found_geo_summ'
    entities_found_fio = 'entities_found_fio_summ'
    entities_found_other = 'entities_found_other_summ'

    entities_validation_ok = 'entities_validation_ok_summ'
    entities_validation_error = 'entities_validation_error_summ'


_entities_count_hgram_buckets = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

_histogram_registry = {
    HistogramName.wizard_response_time.value: _Histogram(
        [0, 0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09, 0.1, 0.15, 0.25]),
}

_counter_registry = {  # type: CounterName: _Counter
    key.value: _Counter() for key in CounterName
}


def write_hgram_value(hgram, value):
    # type: (HistogramName, typing.Union[int, float]) -> None
    # hgram должна быть перечислена в Histograms и _histogram_registry, иначе выпадет AttributeError
    _histogram_registry[hgram.value].write_value(value)


def incr_counter(counter, value=1):
    # type: (CounterName, int) -> None
    _counter_registry[counter.value].incr(value)


def get_serialized_values():
    metrics = []
    for name, hgram in _histogram_registry.iteritems():
        metrics.append((name, hgram.serialize()))
    for name, counter in _counter_registry.iteritems():
        metrics.append((name, counter.serialize()))
    return metrics
