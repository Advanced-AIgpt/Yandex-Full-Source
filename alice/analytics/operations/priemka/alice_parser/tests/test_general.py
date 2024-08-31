# coding: utf-8

import os
import yatest
from nile.api.v1.clusters import MockYQLCluster

from alice.analytics.utils.testing_utils.nile_testing_utils import local_nile_run
from alice.analytics.operations.priemka.alice_parser.lib.alice_parser import AliceParser
from alice.analytics.operations.priemka.alice_parser.lib.prepare_for_render import trim_card_variable_data
from .utils import JOINED_BASKET_DOWNLOADER_SCHEMA


def test_trim_card_variable_data_01():
    card = {
        'buttons': [{
            'directives': [
                {
                    'name': 'open_uri',
                    'payload': {'uri': 'blablabla'},
                    'sub_name': 'open_uri',
                    'type': 'client_action'
                },
                {
                    'name': 'on_suggest',
                    'type': 'server_action',
                    'payload': {
                        '@request_id': 'ffffffff-ffff-ffff-123c-f73a08f32e1b',
                        '@scenario_name': 'Vins',
                        'button_id': 'b5fcb466-435a41e6-65e83eb8-3744e241',
                        'caption': 'Поискать в Яндексе',
                        'request_id': 'ffffffff-ffff-ffff-123c-f73a08f32e1b',
                        'scenario_name': 'Search'
                    }
                }
            ],
            'title': 'Поискать в Яндексе',
            'type': 'action'
        }],
        'text': 'Сейчас найдём',
        'type': 'text_with_button'
    }

    result = trim_card_variable_data(card)
    assert card['buttons'][0]['directives'][1]['payload']['button_id'] == 'b5fcb466-435a41e6-65e83eb8-3744e241'
    assert 'button_id' not in result['buttons'][0]['directives'][1]['payload']


def test_trim_card_variable_data_02():
    card = {
        'text': 'Кажется, вам нужно это место.',
        'type': 'simple_text'
    }
    result = trim_card_variable_data(card)
    assert result == card


def data_path(filename):
    test_folder_name = os.path.basename(__file__).replace('.py', '')
    return yatest.common.test_source_path(os.path.join(test_folder_name, filename))


def test_get_screenshots_data_01():
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy', mode='prepare_for_render')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.get_screenshots_data).label('output')

    input_path = yatest.common.runtime.work_path('test_general/01_prepare_for_render.in.json')

    output_path = local_nile_run(job, input_path, schema=JOINED_BASKET_DOWNLOADER_SCHEMA)

    return yatest.common.canonical_file(output_path)
