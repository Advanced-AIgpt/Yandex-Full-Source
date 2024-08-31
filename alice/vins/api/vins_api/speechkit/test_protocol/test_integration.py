# coding: utf-8

from __future__ import unicode_literals

import json
import logging
import os
import pytest

from datetime import datetime
from freezegun import freeze_time

from alice.vins.api.vins_api.speechkit.connectors.protocol.protos.state_pb2 import TState

from personal_assistant import intents
from personal_assistant.api.personal_assistant import FAST_BASS_QUERY
from personal_assistant.testing_framework import (
    load_stubs, form_handling_mock, turn_into_stub_filename, get_classifiers_mocks, get_pa_submit_form_mock,
    update_setup_bass_response
)
from vins_api.speechkit.connectors.protocol.utils import unpack_state
from vins_api.speechkit.connectors.protocol.testing import match_response, process_request
from vins_core.utils.misc import gen_uuid_for_tests

TEST_DATA_PATH = 'integration_data'

logger = logging.getLogger(__name__)

# Warning: disable all inverse experiments for integration tests
intents.QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS = {}
intents.VINS_IRRELEVANT_INTENTS_FOR_SMART_SPEAKERS_AND_TV_DEVICES = {}


@pytest.fixture(scope='session')
def stubs():
    return load_stubs(stubs_path=os.path.join(TEST_DATA_PATH, 'stubs'))


def make_stubs_callback(intents, bass_responses):
    def stubs_callback(request, context):
        request = json.loads(request.body)
        try:
            intent_before_request = intents.next()
            bass_response = bass_responses.next()

            if not intent_before_request:
                assert 'form' not in request
                return update_setup_bass_response(request, bass_response)
            else:
                intent_name = request['form']['name']
                if intent_name != intent_before_request:
                    pytest.fail(
                        'Wrong intent before bass request: %s instead of %s for utterance "%s"' % (
                            intent_name, intent_before_request, request['meta']['utterance']
                        )
                    )
                return bass_response
        except StopIteration:
            pytest.fail('Not enough stubs, current utterance is "%s"' % request['meta']['utterance'])

    return stubs_callback


def test_cases(vins_app, test_name, dialog_test_data, stubs, entitysearch_base):
    uuid = str(gen_uuid_for_tests())
    state = TState()

    stub = stubs.get(turn_into_stub_filename(dialog_test_data.get_full_test_name()))
    if not stub:
        pytest.skip('No stubs for {} test'.format(test_name))
    time_, intents, bass_responses = stub
    intents = iter(intents)
    bass_responses = iter(bass_responses)
    time_ = datetime.utcfromtimestamp(time_).isoformat()

    bass_mock = form_handling_mock(
        url_suffixes=['vins', 'setup'],
        data=make_stubs_callback(intents, bass_responses),
        balancer_type=FAST_BASS_QUERY,
        entity_base=entitysearch_base
    )

    for utterance_data in dialog_test_data.dialog:
        tagger_mock, classifier_mock = get_classifiers_mocks(dialog_test_data.mock_intents)
        with get_pa_submit_form_mock(utterance_data), tagger_mock, classifier_mock, freeze_time(time_,
                                                                                                tick=True), bass_mock:
            response = process_request(vins_app, utterance_data, dialog_test_data, uuid, state)
            if response.WhichOneof('Response') == 'Error':
                pytest.fail(response.Error.Message.decode('utf-8'))
            state = unpack_state(response.ResponseBody.State)
            logger.debug('Proto response: %s', response)
            result, message = match_response(utterance_data, response)
            if not result:
                pytest.fail(message)
