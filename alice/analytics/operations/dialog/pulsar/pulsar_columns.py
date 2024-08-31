# coding: utf-8

import json
import copy

# конфиг колонок в пульсаре: описывает все доступные колонки и их тип и ширину
BASIC_COLUMNS = {
    'req_id': {
        'name': 'req_id',
        'join_type': 'inner',
        'width': 85,
    },
    'session': {
        'name': 'session',
        'type': 'Json',
        'width': (500, 500),
    },
    'session_markdown': {
        'name': 'session_markdown',
        'type': 'Markdown',
        'width': (360, 360),
    },
    'basket': {
        'name': 'basket',
        'width': 170,
    },
    'asr_text': {
        'name': 'asr_text',
        'type': 'Text',
        'diff_type': 'HasDiff',
        'width': (130, 130, 50),
    },
    'chosen_text': {
        'name': 'chosen_text',
        'type': 'Text',
        'diff_type': 'HasDiff',
        'width': (130, 130, 50),
    },
    'general_toloka_intent': {
        'name': 'general_toloka_intent',
        'width': 100,
    },
    'intent': {
        'name': 'intent',
        'type': 'Text',
        'width': (150, 150),
    },
    'voice_url': {
        'name': 'voice_url',
        'type': 'Audio',
        'width': 310,
    },
    'toloka_intent': {
        'name': 'toloka_intent',
        'width': 100,
    },
    'result': {
        'name': 'result',
        'width': (65, 65),
    },
    'result_eosp': {
        'name': 'result_eosp',
        'width': (65, 65),
    },
    'fraud': {
        'name': 'fraud',
        'width': (65, 65),
    },
    'fraud_eosp': {
        'name': 'fraud_eosp',
        'width': (65, 65),
    },
    'setrace_url': {
        'name': 'setrace_url',
        'type': 'Link',
        'diff_type': 'Nothing',
        'width': (65, 65),
    },
    'text': {
        'name': 'text',
        'width': 200,
    },
    'query': {
        'name': 'query',
        'width': 200,
    },
    'query_eosp': {
        'name': 'query_eosp',
        'width': 200,
    },
    'generic_scenario': {
        'name': 'generic_scenario',
        'type': 'Text',
        'diff_type': 'HasDiff',
        'width': (130, 130, 50),
    },
    'answer': {
        'name': 'answer',
        'type': 'Text',
        'width': (250, 250)
    },
    'action': {
        'name': 'action',
        'type': 'Text',
        'width': (200, 200),
    },
    'generic_scenario_human_readable': {
        'name': 'generic_scenario_human_readable',
        'type': 'Text',
        'width': (200, 200),
    },
    'app': {
        'name': 'app',
        'width': 130,
    },
    'is_command': {
        'name': 'is_command',
        'width': 65,
    },
    'has_eosp_tag': {
        'name': 'has_eosp_tag',
        'width': 65,
    },
    'hashsum': {
        'name': 'hashsum',
        'diff_type': 'HasDiff',
        'width': (80, 80, 50),
    },
    'answer_standard': {
        'name': 'answer_standard',
        'width': (130, 130, 50),
    },
    'screenshot_url': {
        'name': 'screenshot_url',
        'type': 'Image',
        'width': (330, 330),
    },
}


def get_pulsar_columns(result_table_columns):
    """
    Возвращает набор колонок для пульсара
    :param List[str] result_table_columns:
    :return List[dict]:
    """
    metric_columns = []
    for column in result_table_columns:
        if column.startswith('metric_'):
            metric_columns.append({
                'name': column,
                'type': 'Metric',
                'best_value': 'Max',
                'aggregate': True
            })
        if column in BASIC_COLUMNS:
            column_data = copy.deepcopy(BASIC_COLUMNS[column])
            if 'width' in column_data:
                del column_data['width']
            metric_columns.append(column_data)
    return metric_columns


def main(data):
    return {'per_object_data_metainfo': json.dumps(get_pulsar_columns(data))}
