# encoding: utf-8

from itertools import zip_longest

from alice.analytics.operations.priemka.metrics_calculator import calc_metrics_local


def _prepare_data_for_test(values, basket, toloka_intent, app, asr_texts, answers):
    return [
        {
            'req_id': str(idx),
            'text': res,
            'asr_text': asr_text,
            'answer': answer,
            'result': res,
            'fraud': False,
            'basket': basket,
            'app': app,
            'toloka_intent': toloka_intent,
            'voice_url': 'some_voice_url',
        }
        for idx, (res, asr_text, answer) in enumerate(zip_longest(values, asr_texts, answers))
    ]


def calc_specific_metric(prod_values, test_values, metric_name=None, basket=None, toloka_intent='other', app='quasar',
                         metrics_groups=None, asr_prod_texts=None, asr_test_texts=None,
                         prod_answers=None, test_answers=None):
    results = calc_metrics_local.calc_metrics(
        _prepare_data_for_test(prod_values, basket, toloka_intent, app, asr_prod_texts or [], prod_answers or []),
        _prepare_data_for_test(test_values, basket, toloka_intent, app, asr_test_texts or [], test_answers or []),
        metrics_groups=metrics_groups
    )

    if metric_name is None:
        return results

    for item in results:
        if item['metric_name'] == metric_name:
            return item


def _at(container, idx):
    return container[idx] if idx < len(container) else None


def _prepare_data_for_test_v2(general_data, data):
    is_negative_query = general_data.get('is_negative_query', [])
    values = data.get('values', [])
    asr_texts = data.get('asr_texts', [])
    answers = data.get('answer', [])
    intents = data.get('intents', [])

    containers = (is_negative_query, values, asr_texts, answers, intents)
    size = max(map(len, containers))

    return [
        {
            'req_id': str(idx),
            'text': _at(values, idx),
            'asr_text': _at(asr_texts, idx),
            'answer': _at(answers, idx),
            'result': _at(values, idx),
            'intent': _at(intents, idx),
            'is_negative_query': _at(is_negative_query, idx),
            'fraud': False,
            'basket': general_data.get('basket'),
            'app': general_data.get('app', 'quasar'),
            'toloka_intent': general_data.get('toloka_intent', 'other'),
            'voice_url': 'some_voice_url',
        }
        for idx in range(size)
    ]


def calc_specific_metric_v2(general_data, prod_data, test_data,
                            metric_name=None, metrics_groups=None, metrics_params=None):
    results = calc_metrics_local.calc_metrics(
        _prepare_data_for_test_v2(general_data, prod_data),
        _prepare_data_for_test_v2(general_data, test_data),
        metrics_groups=metrics_groups, metrics_params=metrics_params
    )

    if metric_name is None:
        return results

    for item in results:
        if item['metric_name'] == metric_name:
            return item
