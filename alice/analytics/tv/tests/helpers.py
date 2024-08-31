from nile.api.v1 import clusters, Record
from nile.api.v1.local import StreamSource, ListSink
from datetime import datetime
import json
import unittest
import pprint

EPOCH = datetime(1970, 1, 1)
DATETIME_FORMAT = "%Y-%m-%d %H:%M:%S"
DATE_FORMAT = "%Y-%m-%d"


class Options(object):
    def __init__(self, **kwargs):
        self._dict = kwargs

    def __getattr__(self, item, default=None):
        return self._dict.get(item, default)


def digest(data, options, make_job=None, debug=False):
    mock_cluster = clusters.MockCluster()

    job = make_job(mock_cluster.job(), **options)

    output = []
    debug_output = []
    sources = {
        input_name: StreamSource(input_data)
        for input_name, input_data in data.iteritems()
    }
    sinks = {"output": ListSink(output)}

    if debug:
        sinks.update({"debug": ListSink(debug_output)})

    job.local_run(sources=sources, sinks=sinks, statbox_dict_dirs=['statbox_dicts'])

    if debug:
        pprint.pprint(debug_output)

    return output


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


def filter_fields(record, fields):
    return rec(**{field: record[field] for field in fields if field in record})


def unicode_to_str(obj):
    if isinstance(obj, dict):
        return {
            unicode_to_str(key): unicode_to_str(value) for key, value in obj.iteritems()
        }
    elif isinstance(obj, list):
        return [unicode_to_str(element) for element in obj]
    elif isinstance(obj, tuple):
        return tuple([unicode_to_str(element) for element in obj])
    elif isinstance(obj, unicode):
        return obj.encode("utf-8")
    else:
        return obj


class BaseTestCase(unittest.TestCase):
    DATA_FILE = ""
    JOB = None

    @classmethod
    def preprocess(cls, data_file):
        return json.load(data_file)

    @classmethod
    def create_test(cls, test):
        def do_test_expected(self):
            options = {"date": test["date"]}
            result = digest(
                {
                    input_name: [rec(**row) for row in input_data]
                    for input_name, input_data in test.iteritems()
                    if input_name
                    not in ["date", "test_name", "output", "only", "debug"]
                },
                options,
                cls.JOB[0],
                test.get("debug", False),
            )

            expected = [rec(**row) for row in test["output"]]
            self.assertEqual(len(expected), len(result))
            for r, e in zip(result, expected):
                fields = e.to_dict().keys()
                self.assertDictEqual(
                    unicode_to_str(e.to_dict()),
                    unicode_to_str(filter_fields(r, fields).to_dict()),
                )

        return do_test_expected

    @classmethod
    def init_tests(cls):
        cls.maxDiff = None
        with open(cls.DATA_FILE) as data_file:
            data = cls.preprocess(data_file)
        only_mode = max(map(lambda x: x.get("only"), data))

        for test in data:
            if only_mode and test.get("only") is not True:
                continue
            test_method = cls.create_test(test)
            test_method.__name__ = str(test["test_name"])
            setattr(cls, test_method.__name__, test_method)