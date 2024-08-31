# -*-coding: utf8 -*-
from __future__ import print_function
from collections import defaultdict, OrderedDict
import time
from nile.api.v1 import Record, extractors as ne, with_hints
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from qb2.api.v1 import typing
import utils.yt.basket_common as common

from slices_mapping import toloka_intent_to_general_intent, is_video_related_intent, is_good_music_query, is_command_intent, SLICES_MAPPING
import datetime
import yt.wrapper as yt
yt.config.set_proxy("hahn")

from basket_configs import BASKET_CONFIGS, get_basket_param

from operations.dialog.pulsar.pulsar_columns import BASIC_COLUMNS

from handcrafted_responses import (
    HANDCRAFTED_INTENTS,
    REASK_INTENTS,
    HANDCRAFTED_RESPONSES,
)

from functools import partial

ERROR = "UNIPROXY_ERROR"
RENDER_ERROR = "RENDER_ERROR"
UNANSWER = "EMPTY_VINS_RESPONSE"
EMPTY_SIDESPEECH_RESPONSE = "EMPTY_SIDESPEECH_RESPONSE"

ALL_PREDIFINED_RESULTS = {ERROR, RENDER_ERROR, UNANSWER, EMPTY_SIDESPEECH_RESPONSE, "NONEMPTY_SIDESPEECH_RESPONSE"}

FRAUD_PENALTY = -0.75
SIDESPEECH_PENALTY = -0.1
IGNORE_PENALTY = -0.1
LEGTH_PENALTY = -0.25
HANDCRAFTED_BONUS = 0.1
MIN_ANSWER_LENGTH_TO_PENALTY = 125
MAX_ANSWER_LENGTH_TO_PENALTY = 2

SCHEMA = OrderedDict([
    ("action", typing.Optional[typing.String]),
    ("answer", typing.Optional[typing.String]),
    ("answer_standard", typing.Optional[typing.String]),
    ("app", typing.Optional[typing.String]),
    ("asr_text", typing.Optional[typing.String]),
    ("basket", typing.Optional[typing.String]),
    ("fraud", typing.Optional[typing.Bool]),
    ("general_toloka_intent", typing.Optional[typing.String]),
    ("generic_scenario", typing.Optional[typing.String]),
    ("hashsum", typing.Optional[typing.String]),
    ("intent", typing.Optional[typing.String]),
    ("is_command", typing.Optional[typing.Bool]),
    ("metric_alarms_timers", typing.Optional[typing.Float]),
    ("metric_commands", typing.Optional[typing.Float]),
    ("metric_integral", typing.Optional[typing.Float]),
    ("metric_integral_iot", typing.Optional[typing.Float]),
    ("metric_main", typing.Optional[typing.Float]),
    ("metric_music", typing.Optional[typing.Float]),
    ("metric_no_commands", typing.Optional[typing.Float]),
    ("metric_radio", typing.Optional[typing.Float]),
    ("metric_search", typing.Optional[typing.Float]),
    ("metric_toloka_gc", typing.Optional[typing.Float]),
    ("metric_translate", typing.Optional[typing.Float]),
    ("metric_video", typing.Optional[typing.Float]),
    ("metric_weather", typing.Optional[typing.Float]),
    ("mm_scenario", typing.Optional[typing.String]),
    ("req_id", typing.Optional[typing.String]),
    ("result", typing.Optional[typing.String]),
    ("session", typing.Optional[typing.String]),
    ("session_id", typing.Optional[typing.String]),
    ("setrace_url", typing.Optional[typing.String]),
    ("state0", typing.Optional[typing.Json]),
    ("state1", typing.Optional[typing.Json]),
    ("action0", typing.Optional[typing.Json]),
    ("action1", typing.Optional[typing.Json]),
    ("text", typing.Optional[typing.String]),
    ("toloka_intent", typing.Optional[typing.String]),
    ("voice_url", typing.Optional[typing.String]),
    ("generic_scenario_human_readable", typing.Optional[typing.String]),
    ("has_ivi_in_state_or_query", typing.Optional[typing.Bool]),
    ("screenshot_url", typing.Optional[typing.String]),
    ("metric_geo", typing.Optional[typing.Float])
]
)


# тут целочисленное деление. Так и задумывалось. Не меняйте!
def length_penalty(answer):
    return LEGTH_PENALTY*min(MAX_ANSWER_LENGTH_TO_PENALTY, (len(answer)/MIN_ANSWER_LENGTH_TO_PENALTY))


# В новой ue2e есть метка про обман, а также сайд-спич. Введены штрафы за это + дополнительно за длинный неуместный ответ
def metric_with_fraud_and_length_penalty(rec):
    if rec['result'] in (ERROR, RENDER_ERROR):
        return None

    # SideSpeech
    is_empty_answer = rec['result'] in [UNANSWER, EMPTY_SIDESPEECH_RESPONSE]
    if rec['text'] == '':
        if is_empty_answer:
            return 1.
        else:
            # NONEMPTY_SIDESPEECH_RESPONSE
            return SIDESPEECH_PENALTY
    elif is_empty_answer:
        return IGNORE_PENALTY

    if rec['fraud'] is True:
        return FRAUD_PENALTY

    if rec['result'] == 'good':
        if rec['app'] in common.GENERAL_APPS and rec['action'] and rec['action'].startswith(
                'Открывается поиск по запросу'):
            return 0.5
        return 1.
    if rec['result'] == 'bad':
        return length_penalty(rec.get('answer') or '')
    if rec['intent'] in HANDCRAFTED_INTENTS.union(REASK_INTENTS) or rec['answer'] in HANDCRAFTED_RESPONSES:
        return HANDCRAFTED_BONUS
    return 0.


def metric_old_version(rec):
    if rec['result'] in (ERROR, RENDER_ERROR):
        return None
    if rec['result'] == 'good':
        # для успешного ответа поиска в ПП (и всех приложениях с экраном) - метрика 0.5
        if rec['app'] in common.GENERAL_APPS and rec['action'] and rec['action'].startswith('Открывается поиск по запросу'):
            return 0.5
        return 1.
    return 0.


def metric_integral(rec, override_basket_params=None, new_format=True):
    if rec['basket'] == 'input_basket':
        new_format = new_format
    else:
        new_format = get_basket_param('new_format', basket_alias=rec['basket'], override_basket_params=override_basket_params)

    if new_format is True:
        return metric_with_fraud_and_length_penalty(rec)
    return metric_old_version(rec)


def metric_no_commands(rec):
    return metric_integral(rec) if not rec['is_command'] else None


def metric_general_toloka_intent(rec, metric):
    return metric_integral(rec) if rec['general_toloka_intent'] == metric else None


def metric_commands(rec):
    return metric_integral(rec) if rec['is_command'] else None


def metric_iot(rec):
    if rec.get('answer') and 'проверьте настройки' in rec['answer'].lower() and rec['toloka_intent'] == 'iot' and \
            rec['result'] == 'good':
        return 0.
    return metric_old_version(rec)


def metric_integral_translate(rec):
    return metric_old_version(rec) if rec.get('general_toloka_intent') == 'translate' or 'translate' in rec.get('intent', '') else None


def metric_fairytale(rec):
    return metric_old_version(rec) if rec['toloka_intent'] == 'Y' else None


def metric_fraud(rec):
    if rec.get('result') in ALL_PREDIFINED_RESULTS:
        return None
    return 1 if rec.get('fraud') else 0


def metric_general_conversation_fraud(rec):
    if rec.get('result') in ALL_PREDIFINED_RESULTS or rec.get('generic_scenario') != "general_conversation":
        return None
    return 1 if rec.get('fraud') else 0


def metric_main(rec):
    # Метрика "как на дашборде" (без болталки и без команд)
    if rec['result'] != 'bad' and rec.get('intent') and 'general_conversation' in rec['intent'] and \
            rec.get('general_toloka_intent') == 'toloka_gc':
        return None
    if rec['is_command']:
        return None
    return metric_old_version(rec)


def metric_main_stable(rec):
    # Стабильный срез VA-1242
    # без погоды и команд, болталки, умного дома
    if rec['general_toloka_intent'] == 'weather' or rec['is_command'] or \
            (rec['general_toloka_intent'] == 'toloka_gc' and 'general_conversation' in rec['intent']) or \
            (rec['toloka_intent'] == 'action.other' and rec.get('generic_scenario', '') == 'iot_do') or \
            (rec.get('has_ivi_in_state_or_query') is True and is_video_related_intent(rec['toloka_intent'])):
        return None
    # не будет работать для корзин без контекста (не натыкала таких)
    # и вообще не работает для ПП (но для него не подбирала стабильный срез)
    if rec.get('action1'):
        if is_good_music_query(rec['action1'].get('query'), rec['action1'].get('answer'), rec['generic_scenario']):
            return 1.
    if rec['general_toloka_intent'] != "toloka_gc" and "general_conversation" in rec['intent']:
        return 0.
    return metric_old_version(rec)


basket_to_metrics = dict((item['alias'], item['metrics']) for item in (BASKET_CONFIGS or []))


def get_metrics(records, metrics, override_basket_params=None, new_format=True):
    for record in records:
        rec = record.to_dict()
        if rec['basket'] != 'input_basket':
            metrics = basket_to_metrics[rec['basket']]
        for metric in metrics:
            if metric in SLICES_MAPPING:
                metric_value = metric_general_toloka_intent(rec, metric)
            elif metric == 'commands':
                metric_value = metric_commands(rec)
            elif metric == 'integral':
                metric_value = metric_integral(rec, override_basket_params, new_format)
            elif metric == 'no_commands':
                metric_value = metric_no_commands(rec)
            elif metric == 'main':
                metric_value = metric_main(rec)
            elif metric == 'integral_iot':
                metric_value = metric_iot(rec)
            elif metric == 'integral_translate':
                metric_value = metric_integral_translate(rec)
            elif metric == 'fairytale':
                metric_value = metric_fairytale(rec)
            elif metric == 'main_stable':
                metric_value = metric_main_stable(rec)
            elif metric == 'fraud':
                metric_value = metric_fraud(rec)
            elif metric == 'general_conversation_fraud':
                metric_value = metric_general_conversation_fraud(rec)
            else:
                raise Exception('Metric {} is not implemented'.format(metric))
            rec['metric_' + metric] = metric_value
        yield Record(**rec)


def get_grouped_metrics(groups):
    for key, records in groups:
        metric_nums = defaultdict(int)
        metric_denoms = defaultdict(int)
        for record in records:
            rec = record.to_dict()
            for column_name, column_value in rec.items():
                if column_name.startswith('metric_') and column_value is not None:
                    metric_nums[column_name] += column_value
                    metric_denoms[column_name] += 1

        for metric_name in metric_nums.keys():
            grouped_metrics = {
                'metric_type': metric_name,
                'metric_num': metric_nums[metric_name],
                'metric_denom': metric_denoms[metric_name],
                'metric_value': float(metric_nums[metric_name]) / metric_denoms[metric_name]
            }
            yield Record(**grouped_metrics)


def main(ue2e_results, metrics, output_table, metrics_table=None, override_basket_params=None, new_format=True, with_stat_metrics=False, tmp_path=None, pool=None, mr_output_ttl=30):
    templates = {"job_root": "//tmp/robot-voice-qa"}
    if tmp_path is not None:
        templates['tmp_files'] = tmp_path
    cluster = hahn_with_deps(pool=pool,
                             templates=templates,
                             neighbours_for=__file__,
                             neighbour_names=['slices_mapping.py', 'basket_configs.py'],
                             include_utils=True)
    job = cluster.job()

    baskets = list(set(record.basket for record in job.table(ue2e_results).read()))

    results = job.table(ue2e_results) \
        .project(
            ne.all(exclude=['_other', 'mark']),
            general_toloka_intent=ne.custom(toloka_intent_to_general_intent, 'toloka_intent'),
            is_command=ne.custom(is_command_intent, 'toloka_intent'),
        ) \
        .map(with_hints(output_schema=SCHEMA)(partial(
            get_metrics,
            metrics=metrics,
            override_basket_params=override_basket_params,
            new_format=new_format
        ))) \
        .put(output_table)

    #TODO: move split by basket to load_to_pulsar
    @with_hints(outputs=baskets)
    def split_by_basket(records, *outputs):
        for record in records:
            outputs[baskets.index(record.basket)](record)

    if len(baskets) == 1:
        outputs = [results]
    else:
        outputs = results.map(split_by_basket)

    if with_stat_metrics:
        metrics_table = metrics_table or "//tmp/robot-voice-qa/metrics_table_" + common.random_part(16)
        results.groupby().reduce(get_grouped_metrics).put(metrics_table)

    basket_output_tables = []
    basket_metrics_tables = []

    for basket, output in zip(baskets, outputs):
        basket_output_table = output_table + '_' + basket
        output.put(basket_output_table)
        basket_output_tables.append(basket_output_table)

        basket_metrics_table = None
        if with_stat_metrics:
            basket_metrics_table = metrics_table + '_' + basket
            output.groupby().reduce(get_grouped_metrics) \
                  .put(basket_metrics_table)
        basket_metrics_tables.append(basket_metrics_table)

    job.run()
    stat_expiration_time = datetime.datetime.now() + datetime.timedelta(days=5)
    if mr_output_ttl is not None:
        expiration_time = datetime.datetime.now() + datetime.timedelta(days=mr_output_ttl)
        yt.set(output_table + '/@expiration_time', expiration_time.isoformat())
    if with_stat_metrics:
        yt.set(metrics_table + '/@expiration_time', stat_expiration_time.isoformat())

    result = []
    for basket, basket_output_table, basket_metrics_table in zip(baskets, basket_output_tables, basket_metrics_tables):
        result_table_columns = job.table(basket_output_table).read()[0].to_dict().keys()
        if mr_output_ttl is not None:
            yt.set(basket_output_table + '/@expiration_time', expiration_time.isoformat())
        if with_stat_metrics:
            yt.set(basket_metrics_table + '/@expiration_time', stat_expiration_time.isoformat())


        metric_columns = []
        for column in result_table_columns:
            if column.startswith("metric_"):
                metric_columns.append({
                    "name": column,
                    "type": "Metric",
                    "best_value": "Max",
                    "aggregate": True
                })
            elif column in BASIC_COLUMNS:
                metric_columns.append(BASIC_COLUMNS[column])

        result.append({'cluster': 'hahn',
                       'table': basket_output_table,
                       'dataset_name': basket,
                       'per_object_data_metainfo': metric_columns,
                       'stat_metrics_table': basket_metrics_table,
                       })


    return result


if __name__ == '__main__':
    st = time.time()
    print('start at', time.ctime(st))
    call_as_operation(main)
    print('total elapsed {:2f} min'.format((time.time() - st) / 60))
