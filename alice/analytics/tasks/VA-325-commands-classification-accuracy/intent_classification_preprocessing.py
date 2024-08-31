#!/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation
from utils.statface.nile_client import make_stat_client
from utils.yt.dep_manager import list_deps

import re
import warnings

from nile.api.v1 import (
    clusters,
    Record,
    aggregators as na,
    statface as ns
)

def normalize_vins_intent(vins_intent, map, toloka_map):
    intent = vins_intent if vins_intent is not None else ""
    for expr in map.keys():
        if (re.match(expr, intent) is not None):
            return map[expr]

    if(intent in toloka_map.values() or intent in map.values() or intent == ""):
        return intent
    else:
        warnings.warn('Encountered vins intent \"{}\", which is not in latest mapping (https://github.yandex-team.ru/vins/vins-dm/blob/develop/apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json)'.format(
                vins_intent), RuntimeWarning)


def translate_toloka_intent(toloka_intent, map):
    for expr in map.keys():
        if (re.match(expr, toloka_intent) is not None):
            return map[expr]

    warnings.warn('Encountered toloka intent \"{}\", which is not in latest mapping (https://github.yandex-team.ru/vins/vins-dm/blob/develop/apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json)'.format(
            toloka_intent), RuntimeWarning)


def main(dataset, general_mapping, carhead_mapping_patch=None):
    """
        Считаем количество навыков платформы Диалоги на пользователя платформы
        за периоды равные неделе, для недель, заканчивающихся в даты от start - 7days до end.

        :param start: string "YYYY-MM-DD"
        :param end: string "YYYY-MM-DD"
        :param report_path: путь на статистике до !папки, в которую будем загружать отчет
        :param tmp_path: путь для временных файлов (они небольшие, это не сами таблицы)

    """

    main_toloka_mapping = general_mapping['true_intent']
    main_normalizing_mapping = general_mapping['pred_intent']

    carhead_toloka_mapping = general_mapping['true_intent']
    carhead_normalizing_mapping = general_mapping['pred_intent']

    if(carhead_mapping_patch is not None and carhead_mapping_patch['true_intent'] != {}):
        for k,v in carhead_mapping_patch['true_intent'].items():
            carhead_toloka_mapping[k] = v

    if (carhead_mapping_patch is not None and carhead_mapping_patch['pred_intent'] != {}):
        for k, v in carhead_mapping_patch['pred_intent'].items():
            carhead_normalizing_mapping[k] = v

    out = []

    for i in range(len(dataset)):

        if(dataset[i]['app'] == 'yandex.auto.vinstest'):
            true = translate_toloka_intent(dataset[i]['toloka_intent'], carhead_toloka_mapping)
            pred = normalize_vins_intent(dataset[i]['intent'], carhead_normalizing_mapping, carhead_toloka_mapping)
            type = 'AUTO'
        else:
            true = translate_toloka_intent(dataset[i]['toloka_intent'], main_toloka_mapping)
            pred = normalize_vins_intent(dataset[i]['intent'], main_normalizing_mapping, main_toloka_mapping)
            type = 'NAVI'

        out.append({'type':type, 'true_label':true, 'pred_label':pred, 'text':dataset[i]['text'], 'confidence':dataset[i]['confidence'], 'request_id':dataset[i]['request_id']})

    return out



if __name__ == '__main__':
    call_as_operation(main, input_spec={
        'dataset': {'link_name': 'dataset', 'parser': 'json'},
        'general_mapping': {'link_name': 'general_mapping', 'parser': 'json'},
        'carhead_mapping_patch': {'link_name': 'carhead_mapping_patch', 'parser': 'json', 'required': False}
    })
