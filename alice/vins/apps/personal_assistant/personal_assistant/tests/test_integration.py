# coding: utf-8
from __future__ import unicode_literals

import functools
import json
import logging
from datetime import datetime
from itertools import izip_longest
from contextlib import contextmanager

import pytest
from freezegun import freeze_time

from personal_assistant import intents
from personal_assistant.api.personal_assistant import PersonalAssistantAPI, FAST_BASS_QUERY, TestUser
from personal_assistant.testing_framework import (
    get_vins_response, form_handling_mock, check_response, load_stubs, DialogSessionTestData, SingleUtteranceTestData,
    AppInfo, turn_into_stub_filename, update_setup_bass_response
)
from vins_core.utils.config import get_bool_setting
from vins_core.utils.misc import gen_uuid_for_tests

logger = logging.getLogger(__name__)

TEST_DATA_PATH = 'integration_data'

# Warning: disable all inverse experiments for integration tests
intents.QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS = {}
intents.VINS_IRRELEVANT_INTENTS_FOR_SMART_SPEAKERS_AND_TV_DEVICES = {}

test_user_api = PersonalAssistantAPI()


@contextmanager
def get_vins_response_bass(dialog_test_data, vins_app):
    test_user = None
    if dialog_test_data.user_tags:
        test_user = test_user_api.get_test_user(dialog_test_data.user_tags)
        dialog_test_data.additional_options['test_user_oauth_token'] = test_user.token
        get_vins_response_with_bass = functools.partial(
            get_vins_response, vins_app, test_user.uuid, dialog_test_data
        )
    else:
        uuid = str(gen_uuid_for_tests())
        get_vins_response_with_bass = functools.partial(get_vins_response, vins_app, uuid, dialog_test_data)
    try:
        yield get_vins_response_with_bass
    finally:
        if test_user:
            test_user_api.free_test_user(test_user.login)


def test_get_vins_response_bass(mocker, vins_app):
    dialog_test_data = DialogSessionTestData(name='1',
                                             dialog=[SingleUtteranceTestData(request='1', text_regexp='1')],
                                             status='normal',
                                             app_info=AppInfo(app_id='telegram', app_version='0',
                                                              os_version='0', platform='telegram'),
                                             user_tags=['has_home'])

    mocker.patch.object(test_user_api, 'get_test_user', returned_value=TestUser(1, 1, 1, 1, 1, 1))
    free_user_mock = mocker.patch('personal_assistant.api.personal_assistant.PersonalAssistantAPI.free_test_user')
    try:
        with get_vins_response_bass(dialog_test_data, vins_app):
            raise ValueError
    except ValueError:
        pass
    assert free_user_mock.call_count == 1


@pytest.fixture(scope='session')
def stubs():
    return load_stubs()


@pytest.mark.integration
def test_cases(vins_app, stubs, test_name, dialog_test_data, entitysearch_base):
    assert dialog_test_data.freeze_time is None, 'integration tests can not be run in freeze time mode'
    for utterance_test_data in dialog_test_data.dialog:
        assert not utterance_test_data.bass_answer, 'integration tests can not be run with mocked bass answers'
    is_integration = get_bool_setting('INTEGRATION_TESTS')

    if is_integration:
        bass_version = PersonalAssistantAPI().get_version(req_info=None)
    else:
        bass_version = None

    def match(response, utterance_test_data):
        if response is None:
            return None, None

        return check_response(utterance_test_data, response)

    logger.info('Test case %s, BASS version %s', test_name, bass_version)

    stubbed_matches = []
    real_matches = []

    stubs = stubs.get(turn_into_stub_filename(dialog_test_data.get_full_test_name()))
    if stubs:
        logger.info('Running stubbed version')

        time, intents, bass_responses = stubs
        time = datetime.utcfromtimestamp(time).isoformat()
        intents = iter(intents)
        bass_responses = iter(bass_responses)

        def stubs_callback(request, context):
            request = json.loads(request.body)
            try:
                intent_before_request = intents.next()
                bass_response = bass_responses.next()

                # Request to /vins should contain 'form' field with intent name, request to /setup shouldn't
                # Checks that either the intent name in the form is equal to the saved in the stubs one
                # or that both given and stubbed requests don't have form
                if not intent_before_request:
                    assert 'form' not in request
                    return update_setup_bass_response(request, bass_response)
                else:
                    intent_name = request['form']['name']
                    if intent_name != intent_before_request:
                        pytest.fail(
                            "Wrong intent before bass request: %s instead of %s for utterance '%s'" % (
                                intent_name, intent_before_request, request['meta']['utterance']
                            )
                        )
                    return bass_response
            except StopIteration:
                pytest.fail("Not enough stubs, current utterance is '%s'" % request['meta']['utterance'])

        get_vins_response_with_stubs = functools.partial(get_vins_response, vins_app, str(gen_uuid_for_tests()))
        mock = form_handling_mock(url_suffixes=['vins', 'setup'], data=stubs_callback,
                                  balancer_type=FAST_BASS_QUERY, entity_base=entitysearch_base)
        with freeze_time(time, tick=True), mock:
            for utterance_test_data in dialog_test_data.dialog:
                mocked_response = get_vins_response_with_stubs(dialog_test_data, utterance_test_data)
                stubbed_matches.append(match(mocked_response, utterance_test_data))

        # check that all stubs were consumed
        try:
            next(bass_responses)
        except StopIteration:
            # as expected
            pass
        else:
            pytest.fail(
                'Not all stubs were consumed. Please check your test %s and redownload stubs.' %
                test_name
            )

    else:
        logger.warning('Please add stubs for %s test', test_name)

    if is_integration:
        logger.info('Running BASS version')

        with get_vins_response_bass(dialog_test_data, vins_app) as get_vins_response_with_bass:
            for utterance_test_data in dialog_test_data.dialog:
                real_response = get_vins_response_with_bass(utterance_test_data)
                real_matches.append(match(real_response, utterance_test_data))

    for real_match, stubbed_match in izip_longest(real_matches, stubbed_matches):
        if real_match is None and stubbed_match is None:
            # unit test without stub data
            continue
        elif real_match is None and stubbed_match is not None:
            # unit test with stub data
            assert stubbed_match[0], stubbed_match[1]
        elif real_match is not None and stubbed_match is None:
            # integration without stub data
            assert real_match[0], real_match[1]
        elif real_match is not None and stubbed_match is not None:
            # integration test WITH stub data
            real_match, real_problem = real_match
            stubbed_match, stubbed_problem = stubbed_match

            if real_match == stubbed_match:
                assert real_match, (
                    'Unexpected response both for integration and stub.\n'
                    'Integration problem: %s\n'
                    'Stub problem: %s\n'
                    % (real_problem, stubbed_problem)
                )
            elif not stubbed_match:
                assert stubbed_match, (
                    'Stub is broken for this test. But integration is ok.'
                    'Please update your stub file.\n'
                    'Stub problem:  %s'
                    % stubbed_problem
                )
            else:
                assert real_match, (
                    'Attention! Stub test passed but the integration one failed.\n'
                    'Integration problem: %s\n'
                    % real_problem
                )
