# coding: utf-8

import re
import logging
from collections import defaultdict

from alice.analytics.tasks.va_571.basket_configs import get_basket_param, BASKET_CONFIGS_SET
from alice.analytics.tasks.va_571.slices_mapping import (
    toloka_intent_to_general_intent,
    is_command_intent,
)

from alice.analytics.operations.priemka.metrics_calculator.metrics_calculator import MetricCalculator, MetricComparator
from alice.analytics.operations.priemka.metrics_calculator.utils import get_metrics

DIMENSIONS = {
    'basket': lambda item: item.get('basket', '') or '',
    # TODO: dimension: app, generic_scenario
    # 'app': lambda item: item.get('app'),
    # 'generic_scenario': lambda item: item.get('generic_scenario'),
}

DEFAULT_METRICS_GROUPS = ['quality', 'quality_info']


def get_query_key(item):
    """
    Возвращает ключ по словарю с информацией о запросе
    :param dict item:
    :return str:
    """
    return item.get('req_id') or item.get('request_id')


def filter_nulls(item):
    """
    Убирает из словаря ключи, у которых пустые значения (None)
    :param dict item:
    :return dict:
    """
    return {k: v for k, v in item.items() if v is not None}


def notzero(result):
    """
    Возвращает булевый результат, что метрика не равна нулю
    :param dict result:
    :return bool:
    """
    if 'metric_value' in result:
        return result['metric_value'] != 0
    if 'prod_quality' in result and 'test_quality' in result:
        return result['prod_quality'] != 0 or result['test_quality'] != 0
    assert False, 'В json-результате метрики ожидается metric_value или prod_quality+test_quality: "{}"'.format(result)


def prepare_items_dict(raw_items_list):
    """
    Превращает исходный массив с запросов со словарями в словарь,
        где ключом будет ключ запроса, а значением — исходный словарь со всеми полями запроса
    Вычисляет колонки, необходимые для локального расчёта метрик
    :param list raw_items_list:
    :return dict:
    """
    data = {}

    for item in raw_items_list:
        if item.get('toloka_intent'):
            item['general_toloka_intent'] = toloka_intent_to_general_intent(item['toloka_intent'])
            item['is_command'] = is_command_intent(item['toloka_intent'])

        data[get_query_key(item)] = item
    return data


def get_dimensions_keys(dimensions_getters, prod_raw_items_list, test_raw_items_list=None):
    """
    Формирует список ключей для каждого среза
    :param dict dimensions_getters: - объект с декларацией возможных срезов и функции для получения значения среза
    :param list prod_raw_items_list: - массив исходных запросов в проде, у каждого должен быть ключ
    :param list|None test_raw_items_list: - массив исходных запросов в тесте.
    :return dict: - возвращает словарь, где для каждого среза запросов будет хэшсет с соответствующими ключами запросов
    """
    dimensions = {dimension_name: defaultdict(set) for dimension_name in dimensions_getters.keys()}

    for item in prod_raw_items_list:
        for dimension_name, dimension_keys in dimensions.items():
            dimension_key = dimensions_getters[dimension_name](item)
            if dimension_key is not None:
                dimension_keys[dimension_key].add(get_query_key(item))

    if test_raw_items_list:
        for item in test_raw_items_list:
            for dimension_name, dimension_keys in dimensions.items():
                dimension_key = dimensions_getters[dimension_name](item)
                if dimension_key is not None:
                    dimension_keys[dimension_key].add(get_query_key(item))

    return dimensions


def get_metric_by_filters(metrics_list, filters):
    for item in metrics_list:
        is_found = True
        for k, v in filters.items():
            if item.get(k) != v:
                is_found = False
                break
        if is_found:
            return item
    return None


def postprocess_asr_metrics(metrics_list):
    """
    Удаляет wer_aggregated и werp_aggregated, переносит значение и дифф в соответствующие метрики с pvalue
    :param list[dict] metrics_list:
    :return list[dict]:
    """
    TO_REMOVE_LIST = ['wer_aggregated', 'werp_aggregated']
    results = [x for x in metrics_list if x['metric_name'] not in TO_REMOVE_LIST]
    for item in metrics_list:
        if item['metric_name'] in TO_REMOVE_LIST:
            for metrics_item in results:
                if (metrics_item['metric_name'] == item['metric_name'].replace('_aggregated', '')
                    and (metrics_item['metrics_group'] == item['metrics_group'] if item.get('metrics_group') else True)
                        and (metrics_item['basket'] == item['basket'] if item.get('basket') else True)):
                    # ищем такую же метрику, но без суффикса _aggregated по полям: basket, metrics_group, metric_name
                    for field in ['prod_quality', 'test_quality', 'diff', 'diff_percent',
                                  'metric_value', 'metric_num', 'metric_denom']:
                        if item.get(field):
                            metrics_item[field] = item[field]
    return results


def postprocess_metrics_on_slices(metrics_list):
    """
    В метриках качества, рассчитанных на срезе изменения asr/scenario, пересчитывает diff в contrib,
    т.е. во вклад относительно metric_integral на всей корзинке
    :param list[dict] metrics_list:
    :return list[dict]:
    """
    regexp_metric_name = re.compile('integral_on_(asr|scenario)_(changed|same)')
    for item in metrics_list:
        matches = regexp_metric_name.match(item['metric_name'])
        if matches is None:
            continue

        slice_queries_count = get_metric_by_filters(metrics_list, {
            'basket': item.get('basket'),
            'metric_name': 'queries_count_on_{}_{}'.format(matches.group(1), matches.group(2)),
        })
        total_queries_count = get_metric_by_filters(metrics_list, {
            'basket': item.get('basket'),
            'metric_name': 'queries_count',
        })

        if not slice_queries_count or not slice_queries_count.get('diff') or \
                not total_queries_count or not total_queries_count.get('prod_quality'):
            continue

        slice_ratio = slice_queries_count['diff'] / total_queries_count['prod_quality']

        logging.debug(
            'for item: {}, slice queries count: {}, total queries: {}; slice_ratio: {}, new diff: {}, diff_percent: {}'.format(
                item, slice_queries_count['diff'], total_queries_count['prod_quality'],
                slice_ratio, item['diff'] * slice_ratio, item['diff_percent'] * slice_ratio
            ))

        item['diff'] *= slice_ratio
        item['diff_percent'] *= slice_ratio

    return metrics_list


def postprocess_metrics(metrics_list):
    """
    Постобработка списка с посчитанными метриками
    :param list[dict] metrics_list:
    :return list[dict]:
    """
    metrics_list = postprocess_asr_metrics(metrics_list)
    metrics_list = postprocess_metrics_on_slices(metrics_list)

    return sorted(metrics_list, key=lambda x: (x.get('basket', ''), x.get('metrics_group', ''), x['metric_name']))


def need_calc_this_metric(metric_class):
    """
    Нужно ли вычислять данную метрику:
        * если у класса есть метод value и отсутствует calc_this_metric: False
        * если у класса нет метода value, у класса есть наследование от BaseMetric и метод value есть в родителе
    :param class metric_class: инстанс метрики, наследуемой от Metric
    :return bool:
    """
    class_dict = metric_class.__class__.__dict__
    if 'value' in class_dict:
        # в случае, если в классе есть метод value(), то calc_this_metric должен отсутствовать в этом же классе
        # но допускается, чтобы calc_this_metric был у родителя
        return not (class_dict.get('calc_this_metric', None) is False)

    if hasattr(metric_class, 'value'):
        return getattr(metric_class, 'calc_this_metric', None) is True

    return False


def calc_metrics(prod_raw_items_list, test_raw_items_list=None, metrics_groups=None, metrics_params=None):
    """
    Основной метод расчёта метрик
    :param list prod_raw_items_list: массив из объектов в контрольной группе
    :param list|None test_raw_items_list: необязательный массив из объектов в экспериментальной группе.
        Если None или пустой массив — то будут считаться метрики только по первой (единственной) контрольной группе
        Если непустой массив — то будут сравниваться метрики: diff, diff_percent, pvalue
    :param list|None metrics_groups: массив из групп метрик, которые нужно посчитать.
        Если пустой массив или None — то считаются все возможные метрики
    :return list: массив из объектов с названием и значением метрик
    """
    if metrics_groups is None or metrics_groups == '':
        metrics_groups = DEFAULT_METRICS_GROUPS
    if test_raw_items_list is None:
        test_raw_items_list = []

    logging.info('  start calc_metrics: {}'.format(metrics_groups))

    prod_data = prepare_items_dict(prod_raw_items_list)
    logging.info('  prod_data prepared')
    dimensions = get_dimensions_keys(DIMENSIONS, prod_raw_items_list, test_raw_items_list)
    logging.info('  got dimensions_keys')

    results = []
    for metric in get_metrics():
        metric_name = metric.name()
        metric_group = metric.group()

        if len(metrics_groups) > 0 and metric_group not in metrics_groups:
            continue
        if not need_calc_this_metric(metric):
            continue

        logging.info('    calc metric "{}"'.format(metric_name))

        for dimension_name, dimension_keys_dict in dimensions.items():
            for dimension_key, dimension_query_keys_set in dimension_keys_dict.items():
                # для каждого среза dimension_name (срез по корзинке, по app, по generic_scenario, ...)
                if dimension_name == 'basket' and dimension_key in BASKET_CONFIGS_SET \
                        and metric_group in ['quality', 'quality_info']:
                    # Для метрик из basket_configs.py вычисляем только перечисленные метрики качества
                    basket_metrics = get_basket_param('metrics', basket_alias=dimension_key)
                    if metric_name not in basket_metrics:
                        continue

                logging.info('        for {}: {}'.format(dimension_name, dimension_key))

                prod_calculator = MetricCalculator(metric, prod_data, dimension_query_keys_set, metrics_params)

                if len(test_raw_items_list) == 0:
                    # одна система
                    if not prod_calculator.is_defined():
                        continue
                    result = prod_calculator.calc()
                else:
                    # сравнение двух систем
                    test_data = prepare_items_dict(test_raw_items_list)
                    test_calculator = MetricCalculator(metric, test_data, dimension_query_keys_set, metrics_params)
                    if not test_calculator.is_defined() and not prod_calculator.is_defined():
                        # выходим если ни в одной из систем не посчиталась метрика
                        continue
                    result = MetricComparator(prod_calculator, test_calculator).calc()

                if metric.show_zeros() or notzero(result):
                    result[dimension_name] = dimension_key
                    results.append(filter_nulls(result))

    logging.info('  calc metrics finish')

    return postprocess_metrics(results)
