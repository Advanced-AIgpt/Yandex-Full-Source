# encoding: utf-8

from alice.analytics.operations.priemka.metrics_calculator import calc_metrics_local
from alice.analytics.operations.priemka.metrics_calculator import utils
from alice.analytics.operations.priemka.metrics_calculator.metrics import length_penalty, update_max_eosp_result
from collections import defaultdict
from functools import partial
import unittest

from .test_utils import calc_specific_metric
from .test_utils import calc_specific_metric_v2


def test_get_query_key():
    item = {'req_id': 111}
    assert 111 == calc_metrics_local.get_query_key(item)

    item = {'request_id': 222}
    assert 222 == calc_metrics_local.get_query_key(item)

    item = {'qqq': 222}
    assert calc_metrics_local.get_query_key(item) is None

    item = {}
    assert calc_metrics_local.get_query_key(item) is None


def test_prepare_items_dict():
    items = [{'req_id': '1'}, {'req_id': '2'}]
    assert {'1': {'req_id': '1'}, '2': {'req_id': '2'}} == calc_metrics_local.prepare_items_dict(items)


def test_get_dimensions_keys_1():
    DIMENSIONS = {
        'basket': lambda item: item.get('basket', '')
    }
    data = [{'basket': 'b1', 'req_id': '111'}, {'basket': 'b1', 'req_id': '222'}]
    result = defaultdict(set)
    result['b1'].update(['111', '222'])

    assert calc_metrics_local.get_dimensions_keys(DIMENSIONS, data) == {'basket': result}


def test_get_dimensions_keys_2():
    DIMENSIONS = {
        'basket': lambda item: item.get('basket', '')
    }
    data1 = [{'basket': 'b1', 'req_id': '111'}, {'basket': 'b2', 'req_id': '222'}]
    data2 = [{'basket': 'b1', 'req_id': '111'}, {'basket': 'b2', 'req_id': '333'}]
    result = defaultdict(set)
    result['b1'].update(['111'])
    result['b2'].update(['222', '333'])

    assert calc_metrics_local.get_dimensions_keys(DIMENSIONS, data1, data2) == {'basket': result}


def test_get_all_subclasses_simple():
    class A(object):
        pass

    class B(A):
        pass

    class C(A):
        pass

    subclasses = utils.get_all_subclasses(A)
    assert len(subclasses) == 2
    assert B in subclasses
    assert C in subclasses


def test_get_all_subclasses_nested():
    class A(object):
        pass

    class B(A):
        pass

    class C(B):
        pass

    class D(C):
        pass

    subclasses = utils.get_all_subclasses(A)
    assert len(subclasses) == 3
    assert B in subclasses
    assert C in subclasses
    assert D in subclasses


class TestFilterNulls(unittest.TestCase):
    def test_base_filter_nulls(self):
        assert calc_metrics_local.filter_nulls({'a': 1}) == {'a': 1}
        assert calc_metrics_local.filter_nulls({'a': 1, 'b': None}) == {'a': 1}
        assert calc_metrics_local.filter_nulls({'a': None}) == {}
        assert calc_metrics_local.filter_nulls({'a': 1, 'b': None, 'c': ''}) == {'a': 1, 'c': ''}


def almost_equal(x, y, threshold=0.0001):
    return abs(x - y) < threshold


def test_metric_integral_length_penalty():
    assert almost_equal(length_penalty(''), 0)
    assert almost_equal(length_penalty('Привет'), 0)
    assert almost_equal(length_penalty('7' * 120), 0)
    assert almost_equal(length_penalty('Ю' * 120), 0)
    assert almost_equal(length_penalty('7' * 130), -0.25)
    assert almost_equal(length_penalty('Ю' * 130), -0.25)
    assert almost_equal(length_penalty('7' * 260), -0.5)
    assert almost_equal(length_penalty('Ю' * 260), -0.5)


def test_calc_metrics_01_integral():
    prod_values = ['good', 'good', 'bad']
    test_values = ['good', 'good', 'good']
    result = calc_specific_metric(prod_values, test_values, 'integral', basket='input_basket')

    assert result['basket'] == 'input_basket'
    assert result['metric_name'] == 'integral'
    assert almost_equal(result['diff'], 0.3333)
    assert almost_equal(result['diff_percent'], 50)
    assert almost_equal(result['prod_quality'], 0.6666)
    assert almost_equal(result['test_quality'], 1.0)
    assert almost_equal(result['pvalue'], 0.4226)


def test_calc_metrics_02_custom_basket():
    result = calc_specific_metric(['good'], ['good'], 'integral', basket='my_custom_basket')

    assert result['basket'] == 'my_custom_basket'
    assert result['metric_name'] == 'integral'
    assert almost_equal(result['prod_quality'], 1.0)
    assert almost_equal(result['test_quality'], 1.0)


def test_calc_metrics_03_basket_ue2e_quasar_deep():
    # метрика качества из basket_configs
    result = calc_specific_metric(['good'], ['good'], 'integral', basket='ue2e_quasar_deep')

    assert result['basket'] == 'ue2e_quasar_deep'
    assert result['metric_name'] == 'integral'
    assert almost_equal(result['prod_quality'], 1.0)
    assert almost_equal(result['test_quality'], 1.0)


def test_calc_metrics_04_basket_ue2e_quasar_deep():
    # метрика качества из basket_configs, weather отсутствует в quasar и есть в general
    result = calc_specific_metric(
        ['good'], ['good'], 'weather', basket='ue2e_quasar_deep', toloka_intent='search.news_and_weather.weather')
    assert result is None

    result = calc_specific_metric(
        ['good'], ['bad'], 'weather', basket='ue2e_general_deep', toloka_intent='search.news_and_weather.weather')
    assert result['basket'] == 'ue2e_general_deep'
    assert result['metric_name'] == 'weather'
    assert almost_equal(result['prod_quality'], 1.0)
    assert almost_equal(result['test_quality'], 0.0)


def test_calc_metrics_05_render_errors():
    # ошибка рендера
    result = calc_specific_metric(
        ['good', 'RENDER_ERROR', 'UNIPROXY_ERROR'], ['RENDER_ERROR', 'RENDER_ERROR', 'UNIPROXY_ERROR'],
        'render_error', basket='ue2e_general_deep', app='search_app_prod', metrics_groups=['download_error_percent'])

    assert result['metric_name'] == 'render_error'
    assert result['metrics_group'] == 'download_error_percent'
    assert almost_equal(result['prod_quality'], 0.5)
    assert almost_equal(result['test_quality'], 1.0)


def test_calc_metrics_06_toloka_bad_url_error():
    # битый скриншот в толоке
    result = calc_specific_metric(
        ['ill_url', 'bad', 'good', 'RENDER_ERROR'], ['ill_url', 'good', 'RENDER_ERROR', 'RENDER_ERROR'],
        'toloka_bad_url_error', basket='ue2e_general_deep', app='search_app_prod',
        metrics_groups=['download_error_percent'])

    assert result['metric_name'] == 'toloka_bad_url_error'
    assert result['metrics_group'] == 'download_error_percent'
    assert almost_equal(result['prod_quality'], 0.3333)
    assert almost_equal(result['test_quality'], 0.5)


def test_calc_metrics_07_toloka_bad_url_error_absolute():
    # битый скриншот в толоке, абсолютные значения
    result = calc_specific_metric(['ill_url', 'good', 'RENDER_ERROR'], ['ill_url', 'ill_url', 'RENDER_ERROR'],
                                   'toloka_bad_url_error_absolute', basket='ue2e_general_deep', app='search_app_prod',
                                  metrics_groups=['download_error_absolute'])

    assert result['metric_name'] == 'toloka_bad_url_error_absolute'
    assert result['metrics_group'] == 'download_error_absolute'
    assert almost_equal(result['prod_quality'], 1)
    assert almost_equal(result['test_quality'], 2)


def test_calc_metrics_08_ill_url_metric_integral():
    result = calc_specific_metric(['ill_url', 'good'], ['good', 'good'],
                                   'toloka_bad_url_error', basket='ue2e_general_deep', app='search_app_prod',
                                  metrics_groups=['download_error_percent', 'quality'])

    assert result['metric_name'] == 'toloka_bad_url_error'
    assert result['metrics_group'] == 'download_error_percent'
    assert almost_equal(result['prod_quality'], 0.5)
    assert almost_equal(result['test_quality'], 0)

    result = calc_specific_metric(['ill_url', 'good'], ['good', 'good'],
                                   'integral', basket='ue2e_general_deep', app='search_app_prod',
                                  metrics_groups=['download_error_percent', 'quality'])

    assert result['metric_name'] == 'integral'
    assert result['metrics_group'] == 'quality'
    assert almost_equal(result['prod_quality'], 1.0)
    assert almost_equal(result['test_quality'], 1.0)


def test_calc_metrics_09_negative_query():
    general_data = {'is_negative_query': [1, 0, 0, 0, 0, 1]}
    prod_data = {'values': ['bad', 'good', 'good', 'bad', 'good', 'good']}
    test_data = {'values': ['bad', 'good', 'good', 'good', 'good', 'bad']}

    results = calc_specific_metric_v2(general_data, prod_data, test_data, metric_name='queries_count', metrics_groups=['diff'])
    assert almost_equal(results['prod_quality'], 4)
    assert almost_equal(results['test_quality'], 4)
    assert almost_equal(results['diff'], 0)
    assert almost_equal(results['diff_percent'], 0)

    results = calc_specific_metric_v2(general_data, prod_data, test_data, metric_name='negative_queries_count', metrics_groups=['diff'])
    assert almost_equal(results['prod_quality'], 2)
    assert almost_equal(results['test_quality'], 2)
    assert almost_equal(results['diff'], 0)
    assert almost_equal(results['diff_percent'], 0)


def test_calc_metrics_10_intent_classification_metrics():
    general_data = {'is_negative_query': [1, 1, 0, 0, 0, 0, 1, 1]}
    prod_data = {
        'intents': ['1', '0', '1', '2', '2', '3', '4', '0']
    }
    test_data = {
        'intents': ['0', '0', '2\t1', '3\t4', '2', '1', '4', '0']
    }
    metrics_params = {
        'classification_metrics_selector': {
            'intent': ['1', '4']
        }
    }

    results = calc_specific_metric_v2(general_data=general_data, prod_data=prod_data,
                                      test_data=test_data, metrics_groups=['classification'])
    assert results == []

    predefined_calc = partial(calc_specific_metric_v2,
                              general_data=general_data, prod_data=prod_data, test_data=test_data,
                              metrics_groups=['classification'], metrics_params=metrics_params)

    results = predefined_calc(metric_name='accuracy')
    assert results['metric_name'] == 'accuracy'
    assert almost_equal(results['prod_quality'], 0.375)
    assert almost_equal(results['test_quality'], 0.75)
    assert almost_equal(results['diff'], 0.375)

    results = predefined_calc(metric_name='precision')
    assert results['metric_name'] == 'precision'
    assert almost_equal(results['prod_quality'], 0.33333)
    assert almost_equal(results['test_quality'], 0.75)
    assert almost_equal(results['diff'], 0.41666)

    results = predefined_calc(metric_name='recall')
    assert results['metric_name'] == 'recall'
    assert almost_equal(results['prod_quality'], 0.25)
    assert almost_equal(results['test_quality'], 0.75)
    assert almost_equal(results['diff'], 0.5)

    results = predefined_calc(metric_name='fpr')
    assert results['metric_name'] == 'fpr'
    assert almost_equal(results['prod_quality'], 0.5)
    assert almost_equal(results['test_quality'], 0.25)
    assert almost_equal(results['diff'], -0.25)


def test_calc_metrics_11_classification_metrics_complex():
    general_data = {'is_negative_query': [1, None, 0, 0, 0, 0, 1, 1]}
    prod_data = {
        'intents': ['1', None, '1', '2', None, '3', '4', None],
        'values': ['good', 'bad', None, 'bad', None, 'good', 'good', None]
    }
    test_data = {
        'intents': [None, '0', '2\t1', '3\t4', None, '1', '4', '0'],
        'values': ['bad', 'good', 'bad', 'good', None, 'good', 'bad', None]
    }
    metrics_params = {
        'classification_metrics_selector': {
            'intent': ['1', '4'],
            'result': ['good']
        }
    }

    predefined_calc = partial(calc_specific_metric_v2,
                              general_data=general_data, prod_data=prod_data, test_data=test_data,
                              metrics_groups=['classification'], metrics_params=metrics_params)

    results = predefined_calc(metric_name='accuracy')
    assert results['metric_name'] == 'accuracy'
    assert almost_equal(results['prod_quality'], 0.33333)
    assert almost_equal(results['test_quality'], 0.85714)
    assert almost_equal(results['diff'], 0.5238)

    results = predefined_calc(metric_name='precision')
    assert results['metric_name'] == 'precision'
    assert almost_equal(results['prod_quality'], 0.5)
    assert almost_equal(results['test_quality'], 0.8)
    assert almost_equal(results['diff'], 0.3)

    results = predefined_calc(metric_name='recall')
    assert results['metric_name'] == 'recall'
    assert almost_equal(results['prod_quality'], 0.5)
    assert almost_equal(results['test_quality'], 1)
    assert almost_equal(results['diff'], 0.5)

    results = predefined_calc(metric_name='fpr')
    assert results['metric_name'] == 'fpr'
    assert almost_equal(results['prod_quality'], 1)
    assert almost_equal(results['test_quality'], 0.33333)
    assert almost_equal(results['diff'], -0.66667)


def test_calc_metrics_12_classification_metrics_nulls():
    general_data = {'is_negative_query': [0, 0, 0]}
    prod_data = {
        'intents': ['1', '1', '1', '1'],
        'values': ['good', 'good', 'good']
    }
    test_data = {
        'intents': ['1', '2', '2', '2'],
        'values': ['good', 'good', 'good']
    }
    metrics_params = {
        'classification_metrics_selector': {
            'intent': ['2'],
        }
    }

    predefined_calc = partial(calc_specific_metric_v2,
                              general_data=general_data, prod_data=prod_data, test_data=test_data,
                              metrics_groups=['classification'], metrics_params=metrics_params)

    results = predefined_calc(metric_name='accuracy')
    assert results['metric_name'] == 'accuracy'
    assert almost_equal(results['prod_quality'], 0.0)
    assert almost_equal(results['test_quality'], 0.75)
    assert 'diff' not in results

    results = predefined_calc(metric_name='precision')
    assert results['metric_name'] == 'precision'
    assert almost_equal(results['test_quality'], 1.0)
    assert 'prod_quality' not in results
    assert 'diff' not in results

    results = predefined_calc(metric_name='recall')
    assert results['metric_name'] == 'recall'
    assert almost_equal(results['prod_quality'], 0.0)
    assert almost_equal(results['test_quality'], 0.75)
    assert 'diff' not in results


def test_calc_metrics_13_calc_fraud():
    prod_values = ['fraud', 'fraud']
    result = calc_specific_metric(prod_values, [], 'integral', basket='input_basket')

    assert result['basket'] == 'input_basket'
    assert result['metric_name'] == 'integral'
    assert almost_equal(result['metric_value'], -0.75)


class TestEOSP(unittest.TestCase):
    def test_update_max_eosp_result_same_object(self):
        source = {
            'req_id': '123',
            'result': 'good',
            'fraud': False,
        }
        assert update_max_eosp_result(source) == source

        source = {
            'req_id': '123',
            'result': 'bad',
            'fraud': False,
        }
        assert update_max_eosp_result(source) == source

        source = {
            'req_id': '123',
            'result': 'bad',
            'fraud': True,
        }
        assert update_max_eosp_result(source) == source

        source = {
            'req_id': '123',
            'result': 'part',
            'fraud': False,
            'result_eosp': 'part',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source) == source

        source = {
            'req_id': '123',
            'result': 'part',
            'fraud': False,
            'result_eosp': 'part',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source) == source

        source = {
            'req_id': '123',
            'result': 'part',
            'fraud': False,
            'result_eosp': 'part',
            'fraud_eosp': True,
        }
        assert update_max_eosp_result(source) == source

        source = {
            'req_id': '123',
            'result': 'part',
            'fraud': False,
            'result_eosp': 'bad',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source) == source

        source = {
            'req_id': '123',
            'result': 'good',
            'fraud': False,
            'result_eosp': 'good',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source) == source

    def test_update_max_eosp_result_change_item(self):
        source = {
            'req_id': '123',
            'result': 'part',
            'fraud': False,
            'result_eosp': 'good',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source)['result'] == 'good'
        assert update_max_eosp_result(source)['fraud'] is False

        source = {
            'req_id': '123',
            'result': 'part',
            'fraud': True,
            'result_eosp': 'part',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source)['result'] == 'part'
        assert update_max_eosp_result(source)['fraud'] is False

        source = {
            'req_id': '123',
            'result': 'bad',
            'fraud': True,
            'result_eosp': 'part',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source)['result'] == 'part'
        assert update_max_eosp_result(source)['fraud'] is False

        source = {
            'req_id': '123',
            'result': 'part',
            'fraud': False,
            'result_eosp': 'good',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source)['result'] == 'good'
        assert update_max_eosp_result(source)['fraud'] is False

        source = {
            'req_id': '123',
            'result': 'bad',
            'fraud': True,
            'result_eosp': 'part',
            'fraud_eosp': True,
        }
        assert update_max_eosp_result(source)['result'] == 'part'
        assert update_max_eosp_result(source)['fraud'] is True

        source = {
            'req_id': '123',
            'result': 'good',
            'fraud': True,
            'result_eosp': 'good',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source)['result'] == 'good'
        assert update_max_eosp_result(source)['fraud'] is False

    def test_update_max_eosp_result_with_fraud_result(self):
        source = {
            'req_id': '123',
            'result': 'fraud',
            'fraud': True,
            'result_eosp': 'fraud',
            'fraud_eosp': True,
        }
        assert update_max_eosp_result(source)['result'] == 'fraud'
        assert update_max_eosp_result(source)['fraud'] is True

        source = {
            'req_id': '123',
            'result': 'bad',
            'fraud': False,
            'result_eosp': 'fraud',
            'fraud_eosp': True,
        }
        assert update_max_eosp_result(source)['result'] == 'bad'
        assert update_max_eosp_result(source)['fraud'] is False

        source = {
            'req_id': '123',
            'result': 'fraud',
            'fraud': True,
            'result_eosp': 'bad',
            'fraud_eosp': False,
        }
        assert update_max_eosp_result(source)['result'] == 'bad'
        assert update_max_eosp_result(source)['fraud'] is False

        # странный нереальный пример
        source = {
            'req_id': '123',
            'result': 'part',
            'fraud': True,
            'result_eosp': 'fraud',
            'fraud_eosp': True,
        }
        assert update_max_eosp_result(source)['result'] == 'part'
        assert update_max_eosp_result(source)['fraud'] is True


def test_postprocess_metrics_01():
    # сортировка
    assert calc_metrics_local.postprocess_metrics([
        {'basket': '3', 'metrics_group': '1', 'metric_name': '1'},
        {'basket': '2', 'metrics_group': '2', 'metric_name': '2'},
        {'basket': '1', 'metrics_group': '1', 'metric_name': '1'},
    ]) == [
        {'basket': '1', 'metrics_group': '1', 'metric_name': '1'},
        {'basket': '2', 'metrics_group': '2', 'metric_name': '2'},
        {'basket': '3', 'metrics_group': '1', 'metric_name': '1'},
    ]


def test_postprocess_asr_metrics_01():
    # мёрж wer и wer_aggregated для ue2e
    assert calc_metrics_local.postprocess_asr_metrics([
        {
            "basket": "e2e_fairytale",
            "metric_name": "wer",
            "metrics_group": "asr",
            "pvalue": 0.15,
        },
        {
            "basket": "e2e_fairytale",
            "diff": 1.0,
            "diff_percent": 0.2,
            "metric_name": "wer_aggregated",
            "metrics_group": "asr",
            "prod_quality": 5.0,
            "test_quality": 6.0
        }
    ]) == [
        {
            "basket": "e2e_fairytale",
            "diff": 1.0,
            "diff_percent": 0.2,
            "metric_name": "wer",
            "metrics_group": "asr",
            "prod_quality": 5.0,
            "test_quality": 6.0,
            "pvalue": 0.15,
        }
    ]

    # мёрж werp и werp_aggregated для крона
    assert calc_metrics_local.postprocess_asr_metrics([
        {
            "basket": "e2e_fairytale",
            "metric_name": "werp",
            "metrics_group": "asr"
        },
        {
            "basket": "e2e_fairytale",
            "metric_denom": 3497,
            "metric_name": "werp_aggregated",
            "metric_num": 18044.533605680383,
            "metric_value": 5.1600038906721135,
            "metrics_group": "asr"
        }
    ]) == [
        {
            "basket": "e2e_fairytale",
            "metric_denom": 3497,
            "metric_name": "werp",
            "metric_num": 18044.533605680383,
            "metric_value": 5.1600038906721135,
            "metrics_group": "asr"
        }
    ]


def test_postprocess_diff_metrics_01():
    assert calc_metrics_local.postprocess_metrics([
        {'basket': '1', 'metric_name': 'integral', 'prod_quality': 200, 'test_quality': 225, 'diff': 25, 'diff_percent': 12.5},
        {'basket': '1', 'metric_name': 'integral_on_asr_changed', 'prod_quality': 50, 'test_quality': 75, 'diff': 25, 'diff_percent': 50.0},
        {'basket': '1', 'metric_name': 'queries_count', 'prod_quality': 200, 'test_quality': 200},
        {'basket': '1', 'metric_name': 'queries_count_on_asr_changed', 'diff': 50, 'diff_percent': 25},
    ]) == [
        {'basket': '1', 'metric_name': 'integral', 'prod_quality': 200, 'test_quality': 225, 'diff': 25, 'diff_percent': 12.5},
        {'basket': '1', 'metric_name': 'integral_on_asr_changed', 'prod_quality': 50, 'test_quality': 75, 'diff': 6.25, 'diff_percent': 12.5},
        {'basket': '1', 'metric_name': 'queries_count', 'prod_quality': 200, 'test_quality': 200},
        {'basket': '1', 'metric_name': 'queries_count_on_asr_changed', 'diff': 50, 'diff_percent': 25},
    ]

    assert calc_metrics_local.postprocess_metrics([
        {'metric_name': 'integral', 'prod_quality': 200, 'test_quality': 225, 'diff': 25, 'diff_percent': 12.5},
        {'metric_name': 'integral_on_asr_changed', 'prod_quality': 50, 'test_quality': 75, 'diff': 25, 'diff_percent': 50.0},
        {'metric_name': 'queries_count', 'prod_quality': 200, 'test_quality': 200},
        {'metric_name': 'queries_count_on_asr_changed', 'diff': 50, 'diff_percent': 25},
    ]) == [
        {'metric_name': 'integral', 'prod_quality': 200, 'test_quality': 225, 'diff': 25, 'diff_percent': 12.5},
        {'metric_name': 'integral_on_asr_changed', 'prod_quality': 50, 'test_quality': 75, 'diff': 6.25, 'diff_percent': 12.5},
        {'metric_name': 'queries_count', 'prod_quality': 200, 'test_quality': 200},
        {'metric_name': 'queries_count_on_asr_changed', 'diff': 50, 'diff_percent': 25},
    ]
