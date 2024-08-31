#!/usr/bin/env python
# encoding: utf-8

import pytest
from alice.analytics.operations.priemka.metrics_viewer.metrics_viewer import (
    prepare_data, is_significant, is_significant_error, filter_significant_results, has_enough_reused_marks,
    is_bad_quality_for_input_basket, is_bad_wonderlogs, get_metrics_groups_list, has_enough_count_on_slice_metrics
)


def test_prepare_data_simple():
    data = [{'prod_quality': 2, 'test_quality': 8}]
    result = prepare_data(data)
    assert len(result) == 1
    assert result[0]['idx'] == 0
    assert result[0]['diff'] == 6
    assert result[0]['diff_percent'] == 300.0


def test_prepare_data_two():
    data = [{'prod_quality': 2, 'test_quality': 8}, {'prod_quality': 10, 'test_quality': 110, 'pvalue': 0.0001}]
    result = prepare_data(data)
    assert len(result) == 2
    assert result[1]['idx'] == 1
    assert result[1]['diff'] == 100
    assert result[1]['diff_percent'] == 1000.0
    assert result[1]['significant_state'] == 'level_001'


def test_prepare_data_zero():
    data = [{'prod_quality': 2, 'test_quality': 2}]
    result = prepare_data(data)
    assert result[0]['diff'] == 0
    assert result[0]['diff_percent'] == 0


def test_prepare_data_zero_p2():
    data = [{'metric_name': 'some_error', 'prod_quality': 0, 'test_quality': 1}]
    result = prepare_data(data)
    assert result[0]['diff'] == 1
    assert result[0]['diff_percent'] == 100.0


def test_prepare_data_near_epsilon():
    result = prepare_data([{'prod_quality': 0, 'test_quality': 1}])
    assert result[0]['diff'] == 1
    assert result[0]['diff_percent'] == 100.0

    result = prepare_data([{'prod_quality': 0, 'test_quality': 0}])
    assert result[0]['diff'] == 0
    assert result[0]['diff_percent'] == 0

    result = prepare_data([{'prod_quality': 0.1, 'test_quality': 0.1}])
    assert result[0]['diff'] == 0
    assert result[0]['diff_percent'] == 0

    result = prepare_data([{'prod_quality': 0.001, 'test_quality': 0.002}])
    assert result[0]['diff'] == 0.001
    assert result[0]['diff_percent'] == 100


@pytest.mark.parametrize('item,answer', [
    ({'prod_quality': 0.001, 'test_quality': 0.1, 'metric_name': 'vins_sources_error'}, True),
    ({'prod_quality': 0.001, 'test_quality': 0.1, 'metric_name': 'preparer_error'}, True),
    ({'prod_quality': 0.001, 'test_quality': 0.1, 'metric_name': 'uniproxy_error'}, True),
    ({'prod_quality': 0.00606060606060, 'test_quality': 0.00719696969696, 'metric_name': 'uniproxy_error'}, False),

    ({'prod_quality': 0.014, 'test_quality': 0.014, 'metric_name': 'uniproxy_error'}, False),
    ({'prod_quality': 0.015, 'test_quality': 0.015, 'metric_name': 'uniproxy_error'}, True),
    ({'prod_quality': 0.014, 'test_quality': 0.015, 'metric_name': 'uniproxy_error'}, True),
    ({'prod_quality': 0.015, 'test_quality': 0.014, 'metric_name': 'uniproxy_error'}, True),
    ({'prod_quality': 0.01, 'test_quality': 0.01, 'metric_name': 'uniproxy_error'}, False),
    ({'prod_quality': 0.02, 'test_quality': 0.02, 'metric_name': 'uniproxy_error'}, True),

    ({'prod_quality': 0.001, 'test_quality': 0.1, 'metric_name': 'some_other_error'}, True),
    ({'prod_quality': 0.1, 'test_quality': 0.2, 'metric_name': 'some_other_error'}, True),
    ({'prod_quality': 0.1, 'test_quality': 0.1, 'metric_name': 'some_other_error'}, True),
    ({'prod_quality': 0.99, 'test_quality': 0.99, 'metric_name': 'some_other_error'}, True),
    ({'prod_quality': 0.01, 'test_quality': 0.01, 'metric_name': 'some_other_error'}, False),
    ({'prod_quality': 0.015, 'test_quality': 0.015, 'metric_name': 'some_other_error'}, True),
])
def test_is_significant_error(item, answer):
    assert is_significant_error(item) == answer


def test_is_significant():
    assert is_significant({'pvalue': 0.03}) == 'level_003'
    assert is_significant({'pvalue': 0.01}) == 'level_001'
    assert is_significant({'pvalue': 0.0001}) == 'level_001'
    assert is_significant({'pvalue': 0.000000001}) == 'level_001'
    assert is_significant({'pvalue': 0.031}) is None
    assert is_significant({'pvalue': 0.1}) is None
    assert is_significant({'pvalue': 1}) is None
    assert is_significant({'pvalue': None}) is None
    assert is_significant({'pvalue': 0}) == 'level_001'
    assert is_significant({'pvalue': -0}) == 'level_001'
    assert is_significant({'pvalue': 0.0}) == 'level_001'
    assert is_significant({}) is None


def test_get_metrics_groups_list():
    assert get_metrics_groups_list([{'metric_name': '1', 'metrics_group': '1'}], []) == [{'name': '1', 'visible': True}]
    assert get_metrics_groups_list([{'metric_name': '1', 'metrics_group': 'asr'}], []) == [
        {'name': 'asr', 'visible': False}]
    assert get_metrics_groups_list([
        {'metric_name': '1', 'metrics_group': '1'},
        {'metric_name': '2', 'metrics_group': '2'}
    ], []) == [{'name': '1', 'visible': True}, {'name': '2', 'visible': True}]
    assert get_metrics_groups_list([
        {'metric_name': '1', 'metrics_group': 'diff'},
        {'metric_name': '2', 'metrics_group': 'quality'}
    ], []) == [{'name': 'diff', 'visible': False}, {'name': 'quality', 'visible': True}]
    assert get_metrics_groups_list([
        {'metric_name': '1', 'metrics_group': 'diff'},
        {'metric_name': '2', 'metrics_group': 'quality'}
    ], [{'metrics_group': 'diff'}]) == [{'name': 'diff', 'visible': True}, {'name': 'quality', 'visible': True}]


def get_metric_resolution(items, significant_metric_groups=None):
    good, bad = filter_significant_results(items, significant_metric_groups)
    if len(good):
        return 'GOOD'
    elif len(bad):
        return 'BAD'
    else:
        return 'NONE'


def test_metrics_significant_groups():
    # какие-то информационные метрики
    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metrics_group': 'some_informational',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'NONE'

    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metrics_group': 'some_informational',
        'prod_quality': 9,
        'test_quality': 10
    }]) == 'NONE'

    # информационные метрики качества
    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metrics_group': 'quality_info',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'NONE'

    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metrics_group': 'quality_info',
        'prod_quality': 9,
        'test_quality': 10
    }]) == 'NONE'

    # метрики из заданной важной группы
    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metric_name': 'yet_another_metric',
        'metrics_group': 'quality',
        'prod_quality': 9,
        'test_quality': 10
    }]) == 'GOOD'

    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metric_name': 'yet_another_metric',
        'metrics_group': 'quality',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'BAD'

    # метрики без явно-заданной группы -> важные
    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metric_name': 'yet_another_metric',
        'prod_quality': 9,
        'test_quality': 10
    }]) == 'GOOD'

    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metric_name': 'yet_another_metric',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'BAD'

    # метрики wer - где меньше - лучше
    assert get_metric_resolution([{
        'pvalue': 0.1,
        'metric_name': 'werp',
        'prod_quality': 10,
        'test_quality': 10
    }]) == 'NONE'
    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metric_name': 'werp',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'GOOD'
    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metric_name': 'werp',
        'prod_quality': 10,
        'test_quality': 11
    }]) == 'BAD'

    # важные ошибки
    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'error',
        'prod_quality': 0.9,
        'test_quality': 0.9
    }]) == 'BAD'

    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'error',
        'prod_quality': 0.0001,
        'test_quality': 0.0001
    }]) == 'NONE'

    # второстепенные ошибки
    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'minor_errors',
        'prod_quality': 0.9,
        'test_quality': 0.9
    }]) == 'NONE'

    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'minor_errors',
        'prod_quality': 0.0001,
        'test_quality': 0.0001
    }]) == 'NONE'

    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'minor_errors',
        'prod_quality': 0.5,
        'test_quality': 0.5
    }], significant_metric_groups=['minor_errors']) == 'NONE'

    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'download_error_absolute',
        'prod_quality': 0.5,
        'test_quality': 0.5
    }], significant_metric_groups=['download_error_absolute']) == 'NONE'

    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'download_error_percent',
        'prod_quality': 0.5,
        'test_quality': 0.5
    }], significant_metric_groups=['download_error_percent']) == 'BAD'

    # пороги на ошибки для всех остальных ошибок
    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'download_error_percent',
        'prod_quality': 0.009,
        'test_quality': 0.009
    }], significant_metric_groups=['download_error_percent']) == 'NONE'

    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'download_error_percent',
        'prod_quality': 0.01,
        'test_quality': 0.01
    }], significant_metric_groups=['download_error_percent']) == 'NONE'

    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'download_error_percent',
        'prod_quality': 0.014,
        'test_quality': 0.014
    }], significant_metric_groups=['download_error_percent']) == 'NONE'

    assert get_metric_resolution([{
        'metric_name': 'yet_another_error_metric',
        'metrics_group': 'download_error_percent',
        'prod_quality': 0.015,
        'test_quality': 0.015
    }], significant_metric_groups=['download_error_percent']) == 'BAD'

    # ошибки на корзинке e2e_quasar_item_selector
    assert get_metric_resolution([{
        'metric_name': 'all_download_error',
        'basket': 'e2e_quasar_item_selector',
        'prod_quality': 0.035,
        'test_quality': 0.035
    }]) == 'NONE'
    assert get_metric_resolution([{
        'metric_name': 'all_download_error',
        'basket': 'e2e_quasar_item_selector',
        'prod_quality': 0.055,
        'test_quality': 0.055
    }]) == 'BAD'

    # ошибки на корзинах input_basket
    assert is_bad_quality_for_input_basket({
        'metric_name': 'integral',
        'basket': 'input_basket',
        'prod_quality': 0.42,
        'test_quality': 0.69
    }) is True
    assert is_bad_quality_for_input_basket({
        'metric_name': 'integral',
        'basket': 'input_basket',
        'prod_quality': 0.65,
        'test_quality': 0.7
    }) is False
    assert is_bad_quality_for_input_basket({
        'metric_name': 'integral',
        'basket': 'input_basket',
        'prod_quality': 0.70,
        'test_quality': 0.65
    }) is True
    assert is_bad_quality_for_input_basket({
        'metric_name': 'integral',
        'basket': 'input_basket',
        'prod_quality': 0.9,
        'test_quality': 0.71
    }) is False
    assert is_bad_quality_for_input_basket({
        "basket": "input_basket",
        "metric_name": "integral",
        "metric_value": 0.007955801104972356,
        "metric_num": 14.399999999999965,
        "metric_denom": 1810,
        "metrics_group": "quality"
    }) is False
    assert is_bad_quality_for_input_basket({
        "basket": "input_basket",
        "diff": 0.07794401544401541,
        "diff_percent": 17.12437705439507,
        "metric_name": "integral",
        "metrics_group": "quality",
        "prod_quality": 0.45516409266409275,
        "pvalue": 4.754070808879006e-19,
        "test_quality": 0.5331081081081082,
    }) is True

    assert get_metric_resolution([{
        "basket": "input_basket",
        "diff": -0.2038487876214450267,
        "idx": 2,
        "metric_name": "integral",
        "metrics_group": "quality",
        "prod_quality": 0.9224091520861385,
        "pvalue": 0.01428636101274627,
        "test_quality": 0.7185603644646934
    }]) == 'BAD'

    assert get_metric_resolution([{
        "basket": "input_basket",
        "diff": 0.186151212,
        "idx": 2,
        "metric_name": "integral",
        "metrics_group": "quality",
        "prod_quality": 0.5124091520861385,
        "pvalue": 0.01428636101274627,
        "test_quality": 0.6985603644646934
    }]) == 'BAD'

    # ошибки для метрик wonderlogs

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_absolute",
        "metric_name": "wonderlogs_weights",
        "prod_quality": 96380861,
        "test_quality": 87618965
    }) is False

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_absolute",
        "metric_name": "wonderlogs_weights",
        "prod_quality": 96380861,
        "test_quality": 87618964
    }) is True

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_absolute",
        "metric_name": "wonderlogs_weights",
        "prod_quality": 96380861,
        "test_quality": 107089845
    }) is False

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_absolute",
        "metric_name": "wonderlogs_weights",
        "prod_quality": 96380861,
        "test_quality": 107089846
    }) is True

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_absolute",
        "metric_name": "wonderlogs_weights",
        "prod_quality": 96380861,
        "test_quality": 95969437
    }) is False

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_absolute",
        "metric_name": "wonderlogs_rows",
        "prod_quality": 2799,
        "test_quality": 2999
    }) is True

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_absolute",
        "metric_name": "wonderlogs_rows",
        "prod_quality": 2999,
        "test_quality": 2855
    }) is True

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_absolute",
        "metric_name": "wonderlogs_rows",
        "prod_quality": 2999,
        "test_quality": 2885
    }) is False

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_percent",
        "metric_name": "different_wonderlogs_rows",
        "diff": 143,
        "diff_percent": 0.24883413377879865,
        "all": 57468
    }) is False

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_percent",
        "metric_name": "different_wonderlogs_rows",
        "diff": 143,
        "diff_percent": 1.59,
        "all": 57468
    }) is False

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_percent",
        "metric_name": "different_wonderlogs_rows",
        "diff": 143,
        "diff_percent": 1.61,
        "all": 57468
    }) is True

    assert is_bad_wonderlogs({
        "metrics_group": "download_error_percent",
        "metric_name": "different_wonderlogs_rows",
        "diff": 143,
        "all": 57468
    }) is False


def test_metrics_significant_groups_with_reused_marks():
    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metrics_group': 'quality',
        'prod_quality': 0.6,
        'test_quality': 0.5,
    }]) == 'BAD'

    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metrics_group': 'quality',
        'basket': 'all',
        'prod_quality': 0.6,
        'test_quality': 0.5,
    }]) == 'BAD'

    assert get_metric_resolution([
        {
            'pvalue': 0.0001,
            'metrics_group': 'quality',
            'basket': 'all',
            'prod_quality': 0.6,
            'test_quality': 0.5
        },
        {
            'metrics_group': 'toloka_stats',
            'metric_name': 'reused_marks_ratio',
            'basket': 'all',
            'prod_quality': 0.99,
            'test_quality': 0.99
        }
    ]) == 'BAD'

    # 0.98 — недостаточный порог, чтобы метрика считалась значимой
    assert get_metric_resolution([
        {
            'pvalue': 0.0001,
            'metrics_group': 'quality',
            'basket': 'all',
            'prod_quality': 0.6,
            'test_quality': 0.5,
        },
        {
            'metrics_group': 'toloka_stats',
            'metric_name': 'reused_marks_ratio',
            'basket': 'all',
            'prod_quality': 0.98,
            'test_quality': 0.98
        }
    ]) == 'NONE'

    # метрики из some_group не красим
    assert get_metric_resolution([
        {
            'pvalue': 0.0001,
            'metrics_group': 'some_group',
            'basket': 'all',
            'prod_quality': 0.6,
            'test_quality': 0.5
        },
        {
            'metrics_group': 'toloka_stats',
            'metric_name': 'reused_marks_ratio',
            'basket': 'all',
            'prod_quality': 0.99,
            'test_quality': 0.99
        }
    ]) == 'NONE'

    # но some_group можно указать в significant_metric_groups
    assert get_metric_resolution([
        {
            'pvalue': 0.0001,
            'metrics_group': 'some_group',
            'basket': 'all',
            'prod_quality': 0.6,
            'test_quality': 0.5
        }
    ], significant_metric_groups=['some_group']) == 'BAD'
    assert get_metric_resolution([
        {
            'pvalue': 0.0001,
            'metrics_group': 'some_group',
            'basket': 'all',
            'prod_quality': 0.6,
            'test_quality': 0.5
        },
        {
            'metrics_group': 'toloka_stats',
            'metric_name': 'reused_marks_ratio',
            'basket': 'all',
            'prod_quality': 0.99,
            'test_quality': 0.99
        }
    ], significant_metric_groups=['some_group']) == 'BAD'
    assert get_metric_resolution([
        {
            'pvalue': 0.0001,
            'metrics_group': 'some_group',
            'basket': 'all',
            'prod_quality': 0.6,
            'test_quality': 0.5
        },
        {
            'metrics_group': 'toloka_stats',
            'metric_name': 'reused_marks_ratio',
            'basket': 'all',
            'prod_quality': 0.98,
            'test_quality': 0.98
        }
    ], significant_metric_groups=['some_group']) == 'BAD'
    assert get_metric_resolution([
        {
            'pvalue': 0.0001,
            'metrics_group': 'quality',
            'basket': 'all',
            'prod_quality': 0.6,
            'test_quality': 0.5
        },
        {
            'metrics_group': 'toloka_stats',
            'metric_name': 'reused_marks_ratio',
            'basket': 'all',
            'prod_quality': 0.98,
            'test_quality': 0.98
        }
    ], significant_metric_groups=['some_group']) == 'NONE'

    assert get_metric_resolution([{
        "diff": 0,
        "diff_percent": 0.0,
        "metric_name": "some_metric",
        "pvalue": 0.00001
    }]) == 'GOOD'

    assert get_metric_resolution([{
        "metric_name": "some_metric",
        "pvalue": 0.00001
    }]) == 'NONE'

    assert get_metric_resolution([{
        "diff": 0,
        "diff_percent": 0.0,
        "metric_name": "some_metric",
    }]) == 'NONE'


def test_metrics_bad_prod_quality():
    assert get_metric_resolution([{
        "basket": "ue2e_quasar",
        "metric_name": "integral",
        "metrics_group": "quality",
        "prod_quality": 0.72,
        "test_quality": 0.8,
        "pvalue": 0.0001
    }]) == 'BAD'

    assert get_metric_resolution([{
        "basket": "ue2e_quasar",
        "metric_name": "integral",
        "metrics_group": "quality",
        "prod_quality": 0.729,
        "test_quality": 0.8,
        "pvalue": 0.0001
    }]) == 'GOOD'

    assert get_metric_resolution([{
        "basket": "ue2e_tv",
        "metric_name": "integral",
        "metrics_group": "quality",
        "prod_quality": 0.59,
        "test_quality": 0.99,
        "pvalue": 0.0001
    }]) == 'BAD'

    assert get_metric_resolution([
        {
            "basket": "ue2e_tv",
            "metric_name": "integral",
            "metrics_group": "quality",
            "prod_quality": 0.59,
            "test_quality": 0.99,
            "pvalue": 0.0001
        },
        {
            "basket": "ue2e_tv",
            "metric_name": "reused_marks_ratio",
            "prod_quality": 0.7,
            "test_quality": 1.0,
        }
    ]) == 'NONE'


def test_metrics_low_reuse():
    assert get_metric_resolution([{
        "basket": "ue2e_quasar",
        "metric_name": "reused_marks_ratio",
        "prod_quality": 0.95,
        "test_quality": 0.4,
    }]) == 'BAD'

    assert get_metric_resolution([{
        "basket": "ue2e_quasar",
        "metric_name": "reused_marks_ratio",
        "prod_quality": 0.95,
        "test_quality": 0.85,
    }]) == 'NONE'

    assert get_metric_resolution([{
        "basket": "ue2e_quasar",
        "metric_name": "reused_marks_ratio",
        "prod_quality": 0.4,
        "test_quality": 0.4,
    }]) == 'NONE'

    assert get_metric_resolution([{
        "basket": "input_basket",
        "metric_name": "reused_marks_ratio",
        "prod_quality": 0.95,
        "test_quality": 0.4,
    }]) == 'NONE'

    assert get_metric_resolution([{
        "metric_name": "reused_marks_ratio",
        "prod_quality": 0.95,
        "test_quality": 0.4,
    }]) == 'NONE'


def test_metrics_significant_threshholds():
    # разные пороги на pvalue в зависимости от размера корзинки
    assert get_metric_resolution([{
        'pvalue': 0.0,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.0001,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.01,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.02999,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.03,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.031,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'prod_quality': 10,
        'test_quality': 9
    }]) == 'NONE'

    assert get_metric_resolution([{
        'pvalue': 0.02999,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 11000
    }]) == 'NONE'
    assert get_metric_resolution([{
        'pvalue': 0.02999,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 5000
    }]) == 'NONE'
    assert get_metric_resolution([{
        'pvalue': 0.02999,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 4999
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.02999,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 1700
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.02999,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 1100
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.04999,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 1700
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.01,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 1700
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.06,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 1700
    }]) == 'NONE'
    assert get_metric_resolution([{
        'pvalue': 0.06,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 3200
    }]) == 'NONE'
    assert get_metric_resolution([{
        'pvalue': 0.031,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 3200
    }]) == 'NONE'
    assert get_metric_resolution([{
        'pvalue': 0.03,
        'metrics_group': 'quality',
        'metric_name': 'integral',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 3200
    }]) == 'BAD'

    assert get_metric_resolution([{
        'pvalue': 0.03,
        'metrics_group': 'quality',
        'metric_name': 'video',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 3200
    }]) == 'BAD'
    assert get_metric_resolution([{
        'pvalue': 0.03,
        'metrics_group': 'quality',
        'metric_name': 'video',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 7000
    }]) == 'NONE'
    assert get_metric_resolution([{
        'pvalue': 0.009,
        'metrics_group': 'quality',
        'metric_name': 'video',
        'basket': 'basket',
        'prod_quality': 10,
        'test_quality': 9
    }, {
        'basket': 'basket',
        'metric_name': 'queries_count',
        'prod_quality': 7000
    }]) == 'BAD'


def test_has_enough_reused_marks():
    assert has_enough_reused_marks({}, []) is True
    assert has_enough_reused_marks({'basket': None}, []) is True
    assert has_enough_reused_marks({'basket': 'b1'}, []) is True
    assert has_enough_reused_marks(
        {'basket': 'b1'},
        [{'metric_name': 'render_error', 'test_quality': 0.9, 'prod_quality': 0.9}]
    ) is True
    assert has_enough_reused_marks(
        {'basket': 'b1'},
        [{'basket': 'b1', 'metric_name': 'render_error', 'test_quality': 0.9, 'prod_quality': 0.9}]
    ) is True
    assert has_enough_reused_marks(
        {'basket': 'b1'},
        [
            {'basket': 'b1', 'metric_name': 'render_error', 'test_quality': 0.9, 'prod_quality': 0.9},
            {'basket': 'b1', 'metric_name': 'other_metric', 'test_quality': 0.9, 'prod_quality': 0.3},
        ]
    ) is True
    assert has_enough_reused_marks(
        {'basket': 'b1'},
        [
            {'basket': 'b1', 'metric_name': 'render_error', 'test_quality': 0.9, 'prod_quality': 0.9},
            {'basket': 'b1', 'metric_name': 'other_metric', 'test_quality': 0.9, 'prod_quality': 0.3},
        ]
    ) is True
    assert has_enough_reused_marks(
        {'basket': 'b1'},
        [{'basket': 'b1', 'metric_name': 'reused_marks_ratio', 'test_quality': 0.99, 'prod_quality': 0.99}]
    ) is True
    assert has_enough_reused_marks(
        {'basket': 'b1'},
        [{'basket': 'b1', 'metric_name': 'reused_marks_ratio', 'test_quality': 0.9888, 'prod_quality': 0.9888}]
    ) is True
    assert has_enough_reused_marks(
        {'basket': 'b1', 'metrics_group': 'quality-info'},
        [{'basket': 'b1', 'metric_name': 'reused_marks_ratio', 'test_quality': 0.9888, 'prod_quality': 0.9888}]
    ) is False
    assert has_enough_reused_marks(
        {'basket': 'b1', 'metric_name': 'integral_on_asr_changes'},
        [{'basket': 'b1', 'metric_name': 'reused_marks_ratio', 'test_quality': 0.9888, 'prod_quality': 0.9888}]
    ) is False
    assert has_enough_reused_marks(
        {'basket': 'b1', 'metrics_group': 'quality'},
        [{'basket': 'b1', 'metric_name': 'reused_marks_ratio', 'test_quality': 0, 'prod_quality': 0}]
    ) is False
    assert has_enough_reused_marks(
        {'basket': 'b1', 'metrics_group': 'quality'},
        [{'basket': 'b1', 'metric_name': 'reused_marks_ratio', 'test_quality': 0.98, 'prod_quality': 1}]
    ) is False
    assert has_enough_reused_marks({}, []) is True


def test_has_enough_count_on_slice_metrics():
    assert has_enough_count_on_slice_metrics(
        {"basket": "e2e_quasar_facts", "metric_name": "integral_on_scenario_changed"},
        [{"basket": "e2e_quasar_facts", "diff": 31, "metric_name": "queries_count_on_scenario_changed"}]
    ) is True
    assert has_enough_count_on_slice_metrics(
        {"basket": "e2e_quasar_facts", "metric_name": "integral_on_scenario_changed"},
        [{"basket": "e2e_quasar_facts", "diff": 29, "metric_name": "queries_count_on_scenario_changed"}]
    ) is False
    assert has_enough_count_on_slice_metrics(
        {"basket": "e2e_quasar_facts", "metric_name": "integral_on_asr_changed"},
        [{"basket": "e2e_quasar_facts", "diff": 31, "metric_name": "queries_count_on_asr_changed"}]
    ) is True
    assert has_enough_count_on_slice_metrics(
        {"basket": "e2e_quasar_facts", "metric_name": "integral_on_asr_changed"},
        [{"basket": "e2e_quasar_facts", "diff": 29, "metric_name": "queries_count_on_asr_changed"}]
    ) is False
    assert has_enough_count_on_slice_metrics(
        {
            'pvalue': 0.06,
            'metrics_group': 'quality',
            'metric_name': 'integral',
            'basket': 'basket',
            'prod_quality': 10,
            'test_quality': 9
        },
        [{"basket": "e2e_quasar_facts", "diff": 15, "metric_name": "queries_count_on_asr_changed"}]
    ) is True
