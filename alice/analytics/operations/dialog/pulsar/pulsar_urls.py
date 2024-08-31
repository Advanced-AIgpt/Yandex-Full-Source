# coding: utf-8

import json
import logging
import requests
from .pulsar_columns import BASIC_COLUMNS


def _get_column_with_type(column_name):
    """
    Возвращает строку с типом данных и типом диффа для Пульсара
    :param str column_name:
    :return str:
    """
    if column_name.startswith('metric_'):
        return '{}:type=Metric;best_value=Max'.format(column_name)

    column_info = BASIC_COLUMNS.get(column_name, {})
    result = column_name
    if column_info.get('type'):
        result += ':type={}'.format(column_info.get('type'))

    if column_info.get('diff_type'):
        result += ';' if column_info.get('type') else ':'
        result += 'diff_type={}'.format(column_info.get('diff_type'))
    return result


def _get_pulsar_common_columns():
    """
    Возвращает список общих колонок для обоих инстансов в сравнении пульсара
    Порядок не важен
    :return List[str]:
    """
    return [
        'req_id',
        'basket',
        'app',
        'is_command',
        'has_eosp_tag',
        'general_toloka_intent',
        'toloka_intent',
        'voice_url',
        'text',
        'query',
        'query_eosp',
    ]


def _get_pulsar_diff_columns():
    """
    Возвращает список различающихся колонок в сравнении инстансов пульсара
    Порядок не важен
    :return List[str]:
    """
    return [
        'asr_text',
        'chosen_text',
        'intent',
        'session',
        'session_markdown',
        'screenshot_url',
        'setrace_url',
        'result',
        'result_eosp',
        'fraud',
        'fraud_eosp',
        'generic_scenario',
        'generic_scenario_human_readable',
        'answer',
        'action',
        'hashsum',
        'answer_standard',
    ]


def _get_short_mode_column_names(metric_name, need_screenshot):
    """
    Возвращает список колонок, которые будут показываться в "минимальном шаблоне". Важен порядок колонок
    :param str metric_name:
    :param bool need_screenshot:
    :return List[str]:
    """
    return (
        [
            'req_id',
            'setrace_url',
            'basket',
            metric_name,
            'asr_text',
            'text',
            'answer',
            'action',
            'generic_scenario',
            'session_markdown',
        ]
        + [
            'result',
            'intent',
            'hashsum',
        ]
    )


def _get_eosp_mode_column_names(metric_name, need_screenshot):
    """
    Возвращает список колонок, которые будут показываться в "шаблоне EOSP". Важен порядок колонок
    :param str metric_name:
    :param bool need_screenshot:
    :return List[str]:
    """
    return (
        [
            'req_id',
            'setrace_url',
            'basket',
            metric_name,
            'asr_text',
            'text',
            'query',
            'query_eosp',
            'answer',
            'action',
            'generic_scenario',
            'session',
        ]
        + (['screenshot_url'] if need_screenshot else [])
        + [
            'result',
            'result_eosp',
            'fraud',
            'fraud_eosp',
            'intent',
            'has_eosp_tag',
            'hashsum',
        ]
    )


def _get_full_mode_column_names(metric_name, need_screenshot):
    """
    Возвращает список колонок, которые будут показываться в "полном шаблоне". Важен порядок колонок
    :param str metric_name:
    :param bool need_screenshot:
    :return List[str]:
    """
    return (
        [
            'req_id',
            'setrace_url',
            'basket',
            'app',
            'is_command',
            'general_toloka_intent',
            'toloka_intent',
            'voice_url',
            metric_name,
            'asr_text',
            'chosen_text',
            'text',
            'answer',
            'action',
            'generic_scenario',
            'session',
            'session_markdown',
        ]
        + (['screenshot_url'] if need_screenshot else [])
        + [
            'result',
            'fraud',
            'generic_scenario_human_readable',
            'intent',
            'answer_standard',
            'has_eosp_tag',
            'hashsum',
        ]
    )


def _get_pulsar_visible_columns(metric_name, need_screenshot, mode='full'):
    """
    Возвращает список отображаемых колонок в пульсаре
    :param str metric_name: название метрики, по которой считается дифф
    :param bool need_screenshot: - нужно ли добавлять колонку со скриншотом
    :param str mode: режим: short или full — краткий или полный
    :return:
    """
    if mode == 'short':
        return _get_short_mode_column_names(metric_name, need_screenshot)

    if mode == 'eosp':
        return _get_eosp_mode_column_names(metric_name, need_screenshot)

    # mode == 'full'
    return _get_full_mode_column_names(metric_name, need_screenshot)


def _get_columns_width(column_names):
    """
    Возвращает список объектов с шириной колонки для Pulsar UI API
    :param List[str] column_names:
    :return List[ColumnWidth]:
    """
    from pulsar.ui import ColumnWidth

    columns_width = []
    for column_name in column_names:
        if column_name.startswith('metric_'):
            column_width = (65, 65, 100)
        else:
            column_width = BASIC_COLUMNS.get(column_name).get('width')

        if isinstance(column_width, int):
            columns_width.append(ColumnWidth(column_name, column_width))
        else:
            for idx, width in enumerate(column_width):
                columns_width.append(ColumnWidth(column_name, width, subindex=idx))

    return columns_width


def _pulsar_ui_api_with_retries(ui_client, retries=5, **kwargs):
    current_step = 1
    while current_step <= retries:
        try:
            state = ui_client.create_object_table_config(**kwargs)
            return state
        except:
            current_step += 1
            continue


def _get_pulsar_ui_state(basket_name, metric_name, pulsar_token, need_screenshot, mode, row_height=80, slice_metric_type=None):
    """
    Возвращает objectTableState в пульсаре с набором видимых колонок/сортировкой/фильтрацией
    :param Optional[str] basket_name:
    :param str metric_name:
    :param str pulsar_token:
    :param bool need_screenshot:
    :param str mode:
    :param int row_height: высота строки в пикселях
    :return Optional[str]:
    """
    from pulsar.ui import PulsarUIClient, Sorting, Filtering
    ui_client = PulsarUIClient(pulsar_token)

    filtering = []
    if basket_name:
        filtering.append(Filtering('basket', '^{}$'.format(basket_name)))

    if slice_metric_type:
        if slice_metric_type == 'scenario':
            filtering.append(Filtering('generic_scenario', 'Yes', subindex=2, operator='equal'))
            filtering.append(Filtering('asr_text', 'No', subindex=2, operator='equal'))
        if slice_metric_type == 'asr':
            filtering.append(Filtering('asr_text', 'Yes',  subindex=2))
        metric_name = 'metric_integral'

    column_names = _get_pulsar_visible_columns(metric_name, need_screenshot, mode)

    return _pulsar_ui_api_with_retries(
        ui_client,
        retries=5,
        sorting=Sorting(metric_name, subindex=2),
        filtering=filtering,
        column_bands=column_names,
        column_width=_get_columns_width(column_names),
        row_height=row_height,
        sync_aggregation=True,
    )


def get_pulsar_link_for_instance(instance_id, all_metrics_names):
    all_columns = sorted([_get_column_with_type(x) for x in all_metrics_names]) + \
        sorted([_get_column_with_type(x) for x in _get_pulsar_diff_columns()]) + \
        sorted([_get_column_with_type(x) for x in _get_pulsar_common_columns()])

    return 'https://pulsar.yandex-team.ru/instances/{instance_id}{columns}'.format(
        instance_id=instance_id,
        columns='?column=' + '&column='.join(all_columns)
    )


def get_pulsar_link_compare(instance_id_prod, instance_id_test, basket_name, metric_name, all_metrics_names, pulsar_token, need_screenshot, mode, row_height=80):
    """
    Возвращает ссылку на сравнение в пульсаре двух инстансов с фильтром по корзинке, с заданной метрикой и колонками
    :param str instance_id_prod:
    :param str instance_id_test:
    :param Optional[str] basket_name:
    :param str metric_name: - название метрики, которая будет показываться в таблице, по ней считается дифф
    :param List[str] all_metrics_names: - все возможные метрики
    :param str pulsar_token:
    :param bool need_screenshot:
    :param str mode:
    :param int row_height: высота строки в пикселях
    :return str:
    """
    aggregate_columns = sorted([_get_column_with_type(x) for x in all_metrics_names])
    diff_columns = sorted([_get_column_with_type(x) for x in _get_pulsar_diff_columns()])
    common_columns = sorted([_get_column_with_type(x) for x in _get_pulsar_common_columns()])
    slice_metric_type = None
    if metric_name == 'metric_integral_on_scenario_changed':
        slice_metric_type = 'scenario'
    if metric_name == 'metric_integral_on_asr_changed':
        slice_metric_type = 'asr'
    ui_state = _get_pulsar_ui_state(basket_name, metric_name, pulsar_token, need_screenshot, mode, row_height, slice_metric_type)

    BASE_URL = 'https://pulsar.yandex-team.ru/diff/per_object' \
               '?first={instance_id_prod}&second={instance_id_test}' \
               '&join_by=req_id' \
               '&common_columns={common_columns}' \
               '&diff_columns={diff_columns}' \
               '&aggregate_columns={aggregate_columns}' \
               '{state}'

    return BASE_URL.format(
        instance_id_prod=instance_id_prod,
        instance_id_test=instance_id_test,
        common_columns=','.join(common_columns),
        diff_columns=','.join(diff_columns + aggregate_columns),
        aggregate_columns=','.join(aggregate_columns),
        state='&objectTableState={}'.format(ui_state) if ui_state else '',
    )


def _pulsar_api_with_retries(url, headers, data, verify, retries=5):
    current_step = 1
    while current_step <= retries:
        try:
            requests.post(url, headers=headers, data=data, verify=verify)
        except:
            current_step += 1
            continue
        else:
            break


def hit_pulsar_cache(instance_id_prod, instance_id_test, metric_names, pulsar_token):
    """
    Загружает в кеш пульсара сравнение двух инстансов
    :param str instance_id_prod:
    :param str instance_id_test:
    :param List[str] metric_names:
    :param str pulsar_token:
    :return:
    """

    aggregate_columns = sorted([_get_column_with_type(x) for x in metric_names])
    diff_columns = sorted([_get_column_with_type(x) for x in _get_pulsar_diff_columns()])
    common_columns = sorted([_get_column_with_type(x) for x in _get_pulsar_common_columns()])

    url = 'https://pulsar.yandex-team.ru/api/v1/object_diff'
    data = {
        'data': json.dumps({
            'filters': {
                'baseline': instance_id_prod,
                'competitor': [instance_id_test],
                'common_column': common_columns,
                'diff_column': diff_columns + aggregate_columns,
                'aggregate_column': aggregate_columns,
                'join_column': 'req_id',
            }
        })
    }
    header = {'Authorization': 'OAuth {}'.format(pulsar_token)}

    _pulsar_api_with_retries(url, headers=header, data=data, verify=False)
    logging.info('hit_pulsar_cache url: {}, data: {}'.format(url, data['data']))
