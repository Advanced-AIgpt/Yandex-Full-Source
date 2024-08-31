# coding: utf-8
from __future__ import unicode_literals

import logging

import freezegun

from personal_assistant.testing_framework import check_response, universal_form_mock

logger = logging.getLogger(__name__)

TEST_DATA_PATH = 'functional_data'


def execute_test(vins_response_provider, dialog_test_data, entitysearch_base):
    for utterance_test_data in dialog_test_data.dialog:
        bass_answer = utterance_test_data.bass_answer or {'form': {}, 'blocks': []}
        form_name = bass_answer.get('form_name')
        with universal_form_mock(set_slots_map=bass_answer['form'],
                                 blocks=bass_answer['blocks'],
                                 form_name=form_name,
                                 entity_base=entitysearch_base):
            vins_response = vins_response_provider(dialog_test_data, utterance_test_data)
            match, message = check_response(utterance_test_data, vins_response)

            assert match, message


def test_cases(vins_response_provider, test_name, dialog_test_data, entitysearch_base):
    logger.info('Test case %s, mocked BASS', test_name)

    if dialog_test_data.freeze_time:
        with freezegun.freeze_time(dialog_test_data.freeze_time, tick=True):
            execute_test(vins_response_provider, dialog_test_data, entitysearch_base)
    else:
        execute_test(vins_response_provider, dialog_test_data, entitysearch_base)
