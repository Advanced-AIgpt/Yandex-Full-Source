# coding: utf-8

from __future__ import unicode_literals

import json
import mock
import mongomock
import os
import pytest

import yatest.common

from personal_assistant.app import PersonalAssistantApp
from personal_assistant.testing_framework import load_testcase, idfn
from vins_api.speechkit.resources import protocol
from vins_api.speechkit.session import SKSessionStorage
from vins_core.utils.data import load_data_from_file
from vins_sdk import connectors


@pytest.fixture(scope='package', autouse=True)
def init_env():
    os.environ['VINS_RESOURCES_PATH'] = yatest.common.binary_path('alice/vins/resources')


@pytest.fixture(scope='session')
def entitysearch_base():
    return load_data_from_file('vins_core/test/test_data/entitysearch_base.json')


class TestConnector(connectors.ConnectorBase):
    def __init__(self, vins_app=None):
        super(TestConnector, self).__init__(vins_app=vins_app)

    def handle_run(self, run_request):
        return protocol.on_run_request(self.vins_app, run_request)

    def handle_apply(self, apply_request):
        return protocol.on_apply_request(self.vins_app, apply_request)

    def handle_continue(self, continue_request):
        return protocol.on_continue_request(self.vins_app, continue_request)


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

        return True, None, json.dumps(result, ensure_ascii=False, indent=4).encode('utf-8')

    with mock.patch('vins_core.ext.s3.S3DownloadAPI.get_if_modified', get_s3_data):
        vins_application = PersonalAssistantApp(
            vins_file='personal_assistant/config/Vinsfile.json',
            session_storage=SKSessionStorage(mongomock.MongoClient().test_db.sessions),
            allow_wizard_request=True,
        )
        connector = TestConnector(vins_app=vins_application)
        return connector


def gen_tests(test_data_path):
    for test in load_testcase(test_data_path):
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
        test_data_path = getattr(metafunc.module, 'TEST_DATA_PATH', None)
        if test_data_path is None:
            return

        metafunc.parametrize(
            ['test_name', 'dialog_test_data'],
            gen_tests(test_data_path),
            ids=idfn
        )
