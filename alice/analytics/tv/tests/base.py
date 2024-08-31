#!/usr/bin/env python
# encoding: utf-8

import json
import unittest
import os

from nile.api.v1 import clusters, Record
from nile.api.v1.local import StreamSource, ListSink
from utils_tests import (
    process_mock_any,
    map_to_lists_in_dict,
    unicode_to_str,
    filter_dict,
)
SERVICE_FIELDS = {"test_name", "only", "debug"}


def run_job(make_job, job_kwargs, sources, sinks, debug=False):
    mock_cluster = clusters.MockCluster()

    job = make_job(mock_cluster.job(), **job_kwargs)

    debug_sink = []
    sources = {
        source_name: StreamSource(source_data)
        for source_name, source_data in sources.iteritems()
    }
    sink_output = {sink_name: [] for sink_name in sinks}
    sinks = {
        sink_name: ListSink(sink_output[sink_name])
        for sink_name, sink_data in sinks.iteritems()
    }

    if debug:
        sinks.update({"debug": ListSink(debug_sink)})

    job.local_run(sources=sources, sinks=sinks)

    if debug:
        print('\n'.join([str(row.to_dict()) for row in debug_sink]))

    return sink_output


class BaseTestCase(unittest.TestCase):
    DATA_FILE = ""
    JOB = None

    @classmethod
    def preprocess(cls, data_file):
        return json.load(data_file)

    @classmethod
    def prepare_source_row(cls, row):
        return Record(**row)

    @classmethod
    def process_sink_row(cls, row):
        return unicode_to_str(row.to_dict())

    @classmethod
    def prepare_exp_row(cls, row):
        return unicode_to_str(process_mock_any(row))

    @classmethod
    def create_test(cls, test):
        def do_test_expected(self):
            job_kwargs = test.get("kwargs", {})

            out_sinks = run_job(
                make_job=cls.JOB[0],
                job_kwargs=job_kwargs,
                sources=map_to_lists_in_dict(cls.prepare_source_row, test['sources']),
                sinks=test.get("sinks", {}),
                debug=test.get("debug", False),
            )

            expected_sinks = map_to_lists_in_dict(cls.prepare_exp_row, test["sinks"])

            for sink_name in expected_sinks:
                out_sink = map(cls.process_sink_row, out_sinks[sink_name])
                exp_sink = map(cls.prepare_exp_row, expected_sinks[sink_name])
                self.assertEqual(len(out_sink), len(exp_sink))
                for out_row, exp_row in zip(out_sink, exp_sink):
                    # Оставляем только те ключи, по которыми будем сравнивать значения
                    out_row = filter_dict(out_row, include=exp_row.keys())
                    self.assertDictEqual(exp_row, out_row)

        return do_test_expected

    @classmethod
    def init_tests(cls):
        cls.maxDiff = None

        try:
            import yatest.common
            DATA_PATH = yatest.common.source_path("alice/analytics/tv/tests/data")
            DATA_PATH = os.path.join(DATA_PATH, cls.DATA_FILE)
        except ImportError:
            DATA_PATH = os.path.join(os.path.dirname(__file__), "/tv", "data/", cls.DATA_FILE)

        with open(DATA_PATH) as data_file:
            data = cls.preprocess(data_file)

        default_kwargs = data.get("default_kwargs", {})
        default_sources = data.get("default_sources", {})
        tests = data.get("tests", [])
        only_mode = max(map(lambda x: x.get("only"), tests))

        for test in tests:
            if only_mode and test.get("only") is not True:
                continue

            test_kwargs = default_kwargs.copy()
            test_kwargs.update(test.get('kwargs', {}))
            test['kwargs'] = test_kwargs

            test_sources = default_sources.copy()
            test_sources.update(test["sources"])
            test['sources'] = test_sources

            test_method = cls.create_test(test)
            test_method.__name__ = str(test["test_name"])
            setattr(cls, test_method.__name__, test_method)
