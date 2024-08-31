# coding: utf-8
from __future__ import unicode_literals

import pytest
import json

from ..compare_reports import compare_reports, _report_and_exit_error_stats


@pytest.fixture(scope='module')
def validation_reports_json(tmpdir_factory):
    tmpdir = tmpdir_factory.mktemp('test_data')
    ref_results = str(tmpdir.join('ref_report.json'))
    new_results = str(tmpdir.join('new_report.json'))

    ref_data = {
        "accuracy": 0.99,
        "weighted_f_score": 0.99,
        "report": {
            "some_report": {
                "f1_measure": 1.0,
                "precision": 1.0,
                "recall": 1.0,
                "support": 100
            }
        }
    }

    with open(ref_results, 'w') as fd:
        json.dump(ref_data, fd)

    yield ref_results, new_results
    tmpdir.remove()


def make_new_report(new_results, path, value):
    new_data = {
        "accuracy": 0.99,
        "weighted_f_score": 0.99,
        "report": {
            "some_report": {
                "f1_measure": 1.0,
                "precision": 1.0,
                "recall": 1.0,
                "support": 100
            }
        }
    }

    data = new_data
    for p in path[:-1]:
        data = data[p]
    data[path[-1]] = value

    with open(new_results, 'w') as fd:
        json.dump(new_data, fd)


def assert_exit(fun, *args):
    with pytest.raises(SystemExit) as pytest_wrapped_e:
        fun(*args)

    assert pytest_wrapped_e.type == SystemExit
    assert pytest_wrapped_e.value.code == 1


def assert_non_exit(fun, *args):
    fun(*args)


def testcompare_reports_overall_accuracy(validation_reports_json):
    (ref_results, new_results) = validation_reports_json

    make_new_report(new_results, ["accuracy"], 0.97)
    assert_exit(compare_reports, ref_results, new_results, [], 0.01)
    assert_exit(compare_reports, ref_results, new_results, [], 0.02)
    assert_non_exit(compare_reports, ref_results, new_results, [], 0.05)


def testcompare_reports_overall_weighted_f_score(validation_reports_json):
    (ref_results, new_results) = validation_reports_json

    make_new_report(new_results, ["weighted_f_score"], 0.97)
    assert_exit(compare_reports, ref_results, new_results, [], 0.01)
    assert_exit(compare_reports, ref_results, new_results, [], 0.02)
    assert_non_exit(compare_reports, ref_results, new_results, [], 0.05)


def testcompare_reports_report_metrics(validation_reports_json):
    (ref_results, new_results) = validation_reports_json

    make_new_report(new_results, ["report", "some_report", "f1_measure"], 0.99)
    assert_non_exit(compare_reports, ref_results, new_results, [], 0.01)
    assert_non_exit(compare_reports, ref_results, new_results, [], 0.02)

    make_new_report(new_results, ["report", "some_report", "f1_measure"], 0.99)
    assert_exit(compare_reports, ref_results, new_results, ["some_report"], 0.01)
    assert_non_exit(compare_reports, ref_results, new_results, ["some_report"], 0.02)


def testcompare_reports_error_stats(tmpdir, validation_reports_json):
    tmp_file = str(tmpdir.join('tmp_file'))
    (ref_results, new_results) = validation_reports_json
    make_new_report(new_results, ["weighted_f_score"], 0.97)
    assert_non_exit(compare_reports, ref_results, new_results, [], 0.01, tmp_file)
    assert_exit(_report_and_exit_error_stats, tmp_file)
