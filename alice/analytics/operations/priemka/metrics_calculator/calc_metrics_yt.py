# coding: utf-8

import os
import sys
import six
import logging
import json
from functools import partial

from nile.api.v1 import (
    Record,
    with_hints,
    datetime as nd,
    extended_schema,
    modified_schema,
)
from qb2.api.v1 import (
    typing as qt,
)

from alice.analytics.tasks.va_571.slices_mapping import (
    toloka_intent_to_general_intent,
    is_command_intent,
)
from alice.analytics.operations.priemka.alice_parser.utils.queries_utils import has_eosp_tag
from alice.analytics.operations.priemka.metrics_calculator.metrics import RESULT_MARKS_MAP
from alice.analytics.tasks.va_571.basket_configs import BASKET_CONFIGS
from alice.analytics.utils.yt.dep_manager import hahn_with_deps
from alice.analytics.utils.auth import choose_credential
from .utils import get_metrics, get_metric_name
from .markdown_patterns import (
    card_markdown_pattern,
    markdown_action_pattern,
    markdown_query_pattern,
    markdown_screenshot_pattern,
    markdown_extra_music_pattern,
    markdown_extra_smart_home,
    markdown_extra_pattern
)

QUALITY_METRICS_DICT = {x.name(): x for x in get_metrics()}
QUALITY_METRICS_NAMES = [get_metric_name(x) for x in QUALITY_METRICS_DICT.keys()]

BASKET_METRIC_CLASSES = {
    item['alias']: [
        QUALITY_METRICS_DICT[x]
        for x in item['metrics']
        if x in QUALITY_METRICS_DICT
    ]
    for item in BASKET_CONFIGS}
BASKET_METRIC_NAMES = {item['alias']: item['metrics'] for item in BASKET_CONFIGS}
basket_to_metrics = dict((item['alias'], item['metrics']) for item in BASKET_CONFIGS)


def _get_metric_classes_for_basket(basket, custom_metrics=None):
    """
    Возвращает массив классов метрик для вычисления
    Можно задать набор метрик через массив `custom_metrics`
    :param str basket:
    :param List[str] custom_metrics: список дополнительных метрик, без префикса "metric_"
    :return List[Metric]:
    """
    metrics = []

    if basket in BASKET_METRIC_CLASSES:
        metrics += BASKET_METRIC_CLASSES[basket]
    else:
        metrics += [QUALITY_METRICS_DICT['integral']]

    if custom_metrics:
        metrics += [QUALITY_METRICS_DICT[x] for x in custom_metrics]

    return metrics


def calc_metrics_map(records, custom_metrics=None):
    """
    Map операция, которая вычисляет метрики для каждой строчки
    Добавляет колонку с названием метрики
    :param records: nile итератор Records
    :param Optional[List[str]] custom_metrics:
    :return:
    """
    for record in records:
        metrics = {}
        for metric_class in _get_metric_classes_for_basket(record.get('basket'), custom_metrics):
            metric_name = get_metric_name(metric_class.name())
            metrics[metric_name] = metric_class.value(record.to_dict())

        yield Record(record, **metrics)


def _read_yt_table(job, table):
    """
    Считывает табличку из YT в память
    Возвращает итератор на nile Record'ы
    :param job:
    :param table:
    :return:
    """
    return job.table(table).read(bytes_decode_mode='strict' if six.PY3 else 'never')


def _get_input_baskets_list(job, input_table):
    """
    Возвращает уникальные корзинки из YT таблички
    :param job:
    :param input_table:
    :return List[str]:
    """
    return list(set(record.basket for record in _read_yt_table(job, input_table)))


def _get_metrics_extended_schema(job, input_table, custom_metrics=None):
    """
    Возвращает словарь со схемой для рассчитываемых метрик
    :param str input_table: путь к табличке на YT с исходными данными
    :param Optional[list[str]] custom_metrics: список метрик для кастомной корзинки
    :return dict:
    """
    result_schema = {}
    baskets = _get_input_baskets_list(job, input_table)
    for basket in baskets:
        if basket in BASKET_METRIC_CLASSES:
            # корзинка из basket_configs с описанными метриками
            metric_classes = BASKET_METRIC_CLASSES[basket]
            for metric_class in metric_classes:
                metric_name = get_metric_name(metric_class.name())

                if metric_name not in result_schema:
                    result_schema[metric_name] = qt.Optional[qt.Float]
        else:
            result_schema['metric_integral'] = qt.Optional[qt.Float]

    if custom_metrics:
        for metric_name in custom_metrics:
            result_schema[get_metric_name(metric_name)] = qt.Optional[qt.Float]

    return result_schema


def _get_yql_python_udf_path(udf_path):
    """
    Возвращает абсолютный путь к udf so-шке
    :param str udf_path: - относительный путь к so-шке от бинарника
    :return:
    """
    filename = sys.executable
    udf_path_list = [os.path.dirname(filename)] + udf_path.split('/')

    return os.path.join(*udf_path_list)


def _get_main_quality_metric(metrics_list):
    """
    Возвращает основную метрику качества
    :param list[str] metrics_list:
    :return str:
    """
    if 'metric_integral' in metrics_list:
        return 'metric_integral'

    if len(metrics_list):
        # выбираем произвольную метрику качества для выбора max дубликата по req_id
        return metrics_list[0]

    assert False, 'Ошибка, нужно рассчитывать хотябы одну метрику качества: {}'.format(metrics_list)


def is_record_greater(record1, record2, quality_metric):
    """
    Сравнивает 2 записи record1, record2, возвращает True, если первая запись лучше по метрике качества, чем вторая
    :param Record record1:
    :param Record record2:
    :param str quality_metric:
    :return bool:
    """
    metric1 = record1.get(quality_metric) or -1.
    metric2 = record2.get(quality_metric) or -1.

    # первым делом сравниваем значение метрики
    if metric1 > metric2:
        return True
    if metric2 > metric1:
        return False

    # при равных метриках — сравниваем значение result
    result_score1 = max(
        RESULT_MARKS_MAP.get(record1.get('result'), -1.0),
        RESULT_MARKS_MAP.get(record1.get('result_eosp'), -1.0)
    )
    result_score2 = max(
        RESULT_MARKS_MAP.get(record2.get('result'), -1.0),
        RESULT_MARKS_MAP.get(record2.get('result_eosp'), -1.0)
    )
    if result_score1 > result_score2:
        return True
    if result_score2 > result_score1:
        return False

    return False


@with_hints(output_schema=extended_schema(**dict(
    result_eosp=qt.Optional[qt.String],
    fraud_eosp=qt.Optional[qt.Bool],
    has_eosp_tag=qt.Bool,
)))
def aggregate_reqids(groups, quality_metric):
    """
    Уникализирует одинаковые req_id
        Среди записей с одинаковым req_id выбирает ту, где наибольшие метрики качества
        Заполняет параметры разметки пауз по тегу <EOSp>: result_eosp, fraud_eosp, has_eosp_tag
    :param groups:
    :return Iterator[Record]:
    """
    for key_reqid, records in groups:
        records_list = list(records)
        best_record = records_list[0]
        has_row_eosp_tag = has_eosp_tag(best_record.get('text'))

        if len(records_list) == 1:
            yield Record(
                best_record,
                has_eosp_tag=has_row_eosp_tag,
            )
            continue

        result_eosp, fraud_eosp = None, None
        for record in records_list:
            if record != best_record and is_record_greater(record, best_record, quality_metric):
                best_record = record

            if record.get('query') is not None and record.get('query') == record.get('query_eosp'):
                result_eosp = record.get('result')
                fraud_eosp = record.get('fraud')
            if record.get('result_eosp') is not None:
                result_eosp = record.get('result_eosp')
                fraud_eosp = record.get('fraud_eosp')

        yield Record(
            best_record,
            has_eosp_tag=has_row_eosp_tag,
            result_eosp=result_eosp,
            fraud_eosp=fraud_eosp,
        )


def create_markdown_text(extra_info, query, answer, action, scenario, screenshot_url):
    if screenshot_url is None:
        markdown_query_or_screenshot = markdown_query_pattern.format(query=query)
    else:
        markdown_query_or_screenshot = markdown_screenshot_pattern.format(screenshot_url=screenshot_url)
    if action is None:
        markdown_action = ''
    else:
        markdown_action = markdown_action_pattern.format(action=action)
    return card_markdown_pattern.format(extra_info=extra_info, query_or_screenshot=markdown_query_or_screenshot, answer=answer.replace('\n', ''), action=markdown_action, scenario=scenario)


def create_state_info(element):
    state_info = ''
    if not isinstance(element, dict):
        return ''
    for extra in element.get('extra', []):
        if extra.get('type', '') == 'Последняя прослушанная музыка':
            state_info += markdown_extra_music_pattern.format(content=extra.get('content', ''), playback=extra.get('playback', ''))
        elif extra.get('type', '') == 'Устройства умного дома':
            state_info += markdown_extra_smart_home.format(content=json.dumps(extra.get('content', ''), ensure_ascii=False, indent=4))
        elif extra.get('type', '') == 'Сценарии умного дома, созданные пользователем':
            state_info += markdown_extra_pattern.format(state=extra.get('type', ''), content=json.dumps(extra.get('content', ''), ensure_ascii=False, indent=4))
        else:
            state_info += markdown_extra_pattern.format(state=extra.get('type', ''), content=extra.get('content', ''))
    return state_info


def create_markdown_by_json(session):
    markdown_text = ''
    state_info = ''
    for (i, element) in enumerate(session):
        if i % 2 == 0:
            state_info = create_state_info(element)
        else:
            query = element.get('query', '')
            answer = element.get('answer', '')
            scenario = element.get('scenario', '')
            action = element.get('action', None)
            screenshot_url = element.get('url', None)
            markdown_text += create_markdown_text(state_info, query, answer, action, scenario, screenshot_url)
    return markdown_text


@with_hints(output_schema=modified_schema(
    exclude=['_other', 'mark'],
    extend=dict(
        general_toloka_intent=qt.Optional[qt.String],
        is_command=qt.Optional[qt.Bool],
        session_markdown=qt.Optional[qt.String]
    )
))
def prepare_rows(records):
    """
    Map операция, которая подготовливает строки таблицы к расчёту метрик
    :param records: nile итератор Records
    :return:
    """
    for record in records:
        rec = record.to_dict()

        for key_to_remove in ['_other', 'mark']:
            if key_to_remove in rec:
                del rec[key_to_remove]

        if 'toloka_intent' in rec:
            rec['general_toloka_intent'] = toloka_intent_to_general_intent(rec['toloka_intent'])
            rec['is_command'] = is_command_intent(rec['toloka_intent'])

        if len(rec.get('session', [])):
            rec['session_markdown'] = create_markdown_by_json(rec['session'])

        yield Record.from_dict(rec)


def calc_metrics_nile(stream, metrics_extended_schema, custom_metrics=None):
    quality_metric = _get_main_quality_metric(metrics_extended_schema.keys())

    return stream \
        .map(prepare_rows) \
        .map(
            with_hints(output_schema=extended_schema(**metrics_extended_schema))(
                partial(calc_metrics_map, custom_metrics=custom_metrics)
            ),
            memory_limit=1024*8
        ) \
        .groupby('req_id') \
        .reduce(partial(aggregate_reqids, quality_metric=quality_metric)) \
        .sort('req_id')


def calc_metrics(input_table,
                 output_table,
                 udf_path,
                 yt_pool,
                 token=None,
                 tables_ttl=14,
                 custom_metrics=None
                 ):
    """
    Вычисляет метрики качества для таблички `input_table`
    Результаты пишет в табличку со случайным именем в папке `output_path`
    :param str input_table:
    :param str output_table:
    :param str udf_path: относительный путь к so-библиотеке
    :param Optional[str] yt_pool:
    :param str token: yql токен
    :param int tables_ttl: TTL на табличку с результатами, время в днях
    :param Optional[str] custom_metrics: набор кастомных метрик для вычисления в строке через запятую
    :return str: - название таблицы с результатом
    """
    cluster = hahn_with_deps(
        pool=yt_pool,
        use_yql=True,
        custom_env_params=dict(
            yql_python_udf_path=_get_yql_python_udf_path(udf_path),
            bytes_decode_mode='strict' if six.PY3 else 'never',
        ),
        custom_cluster_params=dict(
            yql_token=choose_credential(token, 'YT_TOKEN', '~/.yt/token'),
        )
    )
    job = cluster.job()

    if custom_metrics:
        custom_metrics = [str(c.strip()) for c in custom_metrics.split(',')]

    logging.info('start get extended schema, custom_metrics: {}'.format(custom_metrics))
    metrics_extended_schema = _get_metrics_extended_schema(job, input_table, custom_metrics)
    logging.info('metrics_extended_schema: {}'.format(metrics_extended_schema))

    calc_metrics_nile(job.table(input_table), metrics_extended_schema, custom_metrics) \
        .put(output_table, ttl=nd.timedelta(days=tables_ttl))

    transaction = os.getenv('YT_TRANSACTION')
    if transaction:
        with job.driver.transaction(transaction):
            job.run()
    else:
        job.run()

    logging.info('calc_metrics done: {}'.format(output_table))

    return output_table
