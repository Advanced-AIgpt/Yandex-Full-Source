from datetime import datetime
from nile.api.v1 import Record
from tv.tests.base import BaseTestCase
from tests.utils import tuple_to_list, sorted_values

EPOCH = datetime(1970, 1, 1)
DATETIME_FORMAT = "%Y-%m-%d %H:%M:%S"
DATE_FORMAT = "%Y-%m-%d"
SORTED_FIELDS = ['hdmi_timespent', 'cards_info', 'carousels_info', 'screens_info', 'places_info']


def rec(*args, **kwargs):
    if "hdmi_timespent" in kwargs:
        kwargs["hdmi_timespent"] = list(
            sorted([tuple(t) for t in kwargs["hdmi_timespent"]])
        )

    for k in kwargs:
        if k in {"hdmi_timespent", "cards_info", "carousels_info", "screens_info"}:
            kwargs[k] = list(sorted([tuple(t) for t in kwargs[k]]))

    d = EPOCH

    if "event_datetime" in kwargs:
        d = datetime.strptime(kwargs["event_datetime"], DATETIME_FORMAT)
    elif "event_timestamp" in kwargs:
        d = datetime.utcfromtimestamp(kwargs["event_timestamp"])

    event_timestamp = int((d - EPOCH).total_seconds())
    event_date = d.strftime(DATE_FORMAT)
    event_datetime = d.strftime(DATETIME_FORMAT)
    kwargs.update(
        {
            "event_date": event_date,
            "event_datetime": event_datetime,
            "event_timestamp": event_timestamp,
        }
    )

    return Record(*args, **kwargs)


class TvTestCase(BaseTestCase):
    @classmethod
    def prepare_source_row(cls, row):
        return rec(**row)
    @classmethod
    def process_sink_row(cls, row):
        res = tuple_to_list(super(TvTestCase, cls).process_sink_row(row))
        return sorted_values(res, SORTED_FIELDS)

    @classmethod
    def prepare_exp_row(cls, row):
        res = tuple_to_list(super(TvTestCase, cls).prepare_exp_row(row))
        return sorted_values(res, SORTED_FIELDS)
