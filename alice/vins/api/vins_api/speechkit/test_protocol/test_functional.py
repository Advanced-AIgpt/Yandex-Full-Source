# coding: utf-8

from __future__ import unicode_literals

import logging
import pytest

from datetime import datetime
from freezegun import freeze_time

from alice.vins.api.vins_api.speechkit.connectors.protocol.protos.state_pb2 import TState

from personal_assistant import intents
from personal_assistant.testing_framework import universal_form_mock, get_classifiers_mocks, get_pa_submit_form_mock
from vins_api.speechkit.connectors.protocol.testing import match_response, process_request
from vins_api.speechkit.connectors.protocol.utils import unpack_state
from vins_core.utils.misc import gen_uuid_for_tests

TEST_DATA_PATH = 'functional_data'

logger = logging.getLogger(__name__)

# Warning: disable all inverse experiments for functional tests
intents.QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS = {}
intents.VINS_IRRELEVANT_INTENTS_FOR_SMART_SPEAKERS_AND_TV_DEVICES = {}


def test_cases(vins_app, test_name, dialog_test_data, entitysearch_base):
    uuid = str(gen_uuid_for_tests())
    state = TState()
    for utterance_data in dialog_test_data.dialog:

        bass_answer = utterance_data.bass_answer or {'form': {}, 'blocks': []}
        form_name = bass_answer.get('form_name')

        bass_mock = universal_form_mock(set_slots_map=bass_answer['form'],
                                        blocks=bass_answer['blocks'],
                                        form_name=form_name,
                                        entity_base=entitysearch_base)
        time_mock = freeze_time(dialog_test_data.freeze_time or datetime.now(), tick=True)
        tagger_mock, classifier_mock = get_classifiers_mocks(dialog_test_data.mock_intents)
        with get_pa_submit_form_mock(utterance_data), bass_mock, time_mock, tagger_mock, classifier_mock:
            response = process_request(vins_app, utterance_data, dialog_test_data, uuid, state)
            if response.WhichOneof('Response') == 'Error':
                pytest.fail(response.Error.Message.decode('utf-8'))
            state = unpack_state(response.ResponseBody.State)
            logger.debug('Proto response: %s', response)
            result, message = match_response(utterance_data, response)
            if not result:
                pytest.fail(message)
