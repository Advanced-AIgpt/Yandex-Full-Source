# coding: utf-8
from __future__ import unicode_literals

import logging
import os
import mock
from copy import deepcopy

from personal_assistant.testing_framework import (
    process_file, check_form, get_classifiers_mocks, get_pa_submit_form_mock, convert_vins_response_for_tests
)
from vins_core.utils.data import list_resource_files_with_prefix, open_resource_file, load_data_from_file
from vins_core.utils.config import get_setting
from vins_core.dm.request import configure_experiment_flags, get_experiments
from crm_bot.api.crm_bot import CrmBotAPI

logger = logging.getLogger(__name__)


def spy_fake_submit_form_and_check(source_method, check_form_data):
    def wrapper(self, result, req_info, form):
        check_form(req_info, form, check_form_data)
        return source_method(self, result, req_info, form)

    return wrapper


def get_pa_fake_submit_form_mock(utterance_test_data):
    submit_form_wrapper = spy_fake_submit_form_and_check(CrmBotAPI.fake_submit_form, utterance_test_data.vins_form)
    return mock.patch.object(CrmBotAPI, 'fake_submit_form', submit_form_wrapper)


def list_data_files(test_data_path):
    res_prefix = os.path.join('crm_bot', 'tests', test_data_path)
    tests = list_resource_files_with_prefix(res_prefix)
    for fname in tests:
        name, _ = os.path.splitext(fname)
        if all([part.startswith('test_') for part in fname.split('/')]):
            yield name.replace('/', '::'), os.path.join(res_prefix, fname)


def load_app_infos(test_data_path):
    fname = os.path.join('crm_bot', 'tests', test_data_path, 'app_presets.yaml')
    app_infos = load_data_from_file(fname)

    env_platform = get_setting('TEST_PLATFORM', '')
    if env_platform:
        if env_platform not in app_infos:
            raise AssertionError(
                "Unexpected app platform in VINS_TEST_PLATFORM: '{}'".format(env_platform)
            )
        app_infos = {env_platform: app_infos[env_platform]}

    return app_infos


def load_testcase(test_data_path, placeholders=None):
    app_infos = load_app_infos(test_data_path)
    for name, path in list_data_files(test_data_path):
        for dialog_test_data in process_file(open_resource_file(path), name, placeholders, app_infos):
            yield dialog_test_data


def get_vins_response(vins_app, uuid, dialog_test_data, utterance_test_data, request_id=None):
    additional_options = dialog_test_data.additional_options
    if additional_options and 'oauth_token' in additional_options:
        # change oauth_token value from setting name to setting value
        additional_options = deepcopy(additional_options)
        try:
            additional_options['oauth_token'] = get_setting(additional_options['oauth_token'])
        except Exception:
            logger.warning(
                'can not get additional_options/oauth_token={} value from settings'
                ' (skip using oauth)'.format(additional_options['oauth_token'])
            )
            additional_options.pop('oauth_token', None)

    if additional_options and 'test_user_oauth_token' in additional_options:
        additional_options = deepcopy(additional_options)
        additional_options['oauth_token'] = additional_options['test_user_oauth_token']
        additional_options.pop('test_user_oauth_token', None)

    if dialog_test_data.device_state and 'filtration_level' in dialog_test_data.device_state:
        additional_options = additional_options or {}
        if 'bass_options' not in additional_options:
            additional_options['bass_options'] = {}
        additional_options['bass_options']['filtration_level'] = dialog_test_data.device_state['filtration_level']

    tagger_mock, classifier_mock = get_classifiers_mocks(dialog_test_data.mock_intents)
    with get_pa_submit_form_mock(utterance_test_data), \
         get_pa_fake_submit_form_mock(utterance_test_data), \
         tagger_mock, classifier_mock:
        response = vins_app.handle_event(
            uuid,
            utterance_test_data.event,
            text_only=False,
            experiments=configure_experiment_flags(get_experiments(), utterance_test_data.experiments),
            location=dialog_test_data.geo_info,
            app_info=dialog_test_data.app_info,
            device_state=dialog_test_data.device_state,
            additional_options=additional_options,
            reset_session=dialog_test_data.dialog[0] is utterance_test_data,
            dialog_id=utterance_test_data.dialog_id,
            request_id=request_id,
        )

    return convert_vins_response_for_tests(response)
