# coding: utf-8
from __future__ import unicode_literals

import functools
import json
import os

import yatest.common
import mock
import pytest
import requests_mock

from personal_assistant.app import PersonalAssistantApp
from personal_assistant.testing_framework import (
    get_vins_response, load_testcase, parse_placeholders, init_mock, idfn, TEST_REQUEST_ID
)
from vins_core.utils.config import get_bool_setting
from vins_core.utils.data import load_data_from_file
from vins_core.utils.misc import gen_uuid_for_tests
from vins_sdk import connectors
from vins_core.utils.strings import smart_utf8


@pytest.fixture(scope='package', autouse=True)
def init_env():
    os.environ['VINS_RESOURCES_PATH'] = yatest.common.binary_path('alice/vins/resources')


@pytest.fixture(scope='session')
def fixlist():
    return connectors.FIXLIST


@pytest.fixture(scope='session')
def banlist():
    return connectors.BANLIST


@pytest.fixture(scope='session')
def hardcoded_responses():
    with mock.patch('personal_assistant.hardcoded_responses.load_hardcoded',
                    return_value=connectors.HARDCODED_RESPONSES):
        yield connectors.HARDCODED_RESPONSES


@pytest.fixture(scope='session')
def gc_banlist():
    return connectors.GC_BANLIST


@pytest.fixture(scope='package')
def vins_app(fixlist, banlist, hardcoded_responses, gc_banlist):
    def get_s3_data(self, key, *args, **kwargs):
        if key == 'pa_fixlist.json':
            result = fixlist
        elif key == 'pa_banlist_boltalka_50.json':
            result = banlist
        elif key == 'pa_gc_banlist.json':
            result = gc_banlist
        else:
            raise KeyError('Unexpected S3 key: %s' % key)

        return True, None, smart_utf8(json.dumps(result, ensure_ascii=False, indent=4))

    with mock.patch('vins_core.ext.s3.S3DownloadAPI.get_if_modified', get_s3_data):
        vins_application = PersonalAssistantApp(
            vins_file='personal_assistant/config/Vinsfile.json',
            allow_wizard_request=True,
        )
        connector = connectors.TestConnector(vins_app=vins_application)
        return connector


@pytest.fixture
def vins_response_provider(vins_app):
    return functools.partial(get_vins_response, vins_app, str(gen_uuid_for_tests()), request_id=TEST_REQUEST_ID)


@pytest.fixture(autouse=True, scope='function')
def mock_request(request, mocker):
    """ Mock all http calls (except wizard's) on every test """
    if (
        (
            request.keywords.get('integration') is not None and
            get_bool_setting('INTEGRATION_TESTS')
        ) or
        request.keywords.get('no_requests_mock')
    ):
        yield mocker.Mock(spec=requests_mock.Mocker)
        return

    m = requests_mock.Mocker()
    init_mock(m)

    start = m.start
    stop = m.stop

    # prevent enabling/disabling mock outside this fixture
    def empty():
        pass

    m.start = empty
    m.stop = empty

    mocker.patch('requests_mock.Mocker', return_value=m)
    try:
        start()
        yield m
    finally:
        stop()


@pytest.fixture(scope="session")
def entitysearch_base():
    return load_data_from_file('vins_core/test/test_data/entitysearch_base.json')


def pytest_addoption(parser):
    parser.addoption(
        "--test-prefix", action="store", default='',
        help="Allows to choose test cases which names start with specified prefix"
    )
    parser.addoption(
        "--placeholders", action="store", default='',
        help=(
            "Allow replacing placeholders with provided values. "
            "Format: `key1:any value1;key2:any value2`"
        )
    )


def gen_tests(test_data_path, test_prefix, placeholders):
    for test in load_testcase(test_data_path, placeholders):
        if test_prefix and not test.name.startswith(test_prefix):
            continue

        marks = ()
        if test.status == 'skip':
            marks = pytest.mark.skip(reason=test.status_reason)
        elif test.status == 'xfail':
            marks = pytest.mark.xfail(reason=test.status_reason, strict=True)
        elif test.status == 'flaky':
            marks = pytest.mark.xfail(reason=test.status_reason, strict=False)

        yield pytest.param(test.name, test, marks=marks)


def pytest_generate_tests(metafunc):
    if 'dialog_test_data' in metafunc.fixturenames:
        test_prefix = metafunc.config.option.test_prefix
        placeholders = parse_placeholders(metafunc.config.option.placeholders)

        test_data_path = getattr(metafunc.module, 'TEST_DATA_PATH', None)
        if test_data_path is None:
            return

        metafunc.parametrize(
            ['test_name', 'dialog_test_data'],
            gen_tests(test_data_path, test_prefix, placeholders),
            ids=idfn
        )
