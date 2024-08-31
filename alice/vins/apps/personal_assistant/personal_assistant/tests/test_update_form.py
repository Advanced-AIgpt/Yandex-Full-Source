# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.dm.form_filler.models import Form
from vins_core.dm.intent import Intent
from vins_core.dm.request import ReqInfo, AppInfo
from vins_core.dm.session import Session

from personal_assistant.bass_result import BassFormInfo
from vins_core.utils.datetime import utcnow
from vins_core.utils.misc import gen_uuid_for_tests


@pytest.fixture(scope='function')
def form():
    return Form.from_dict({
        'name': 'test-form',
        'slots': [
            {
                'name': 'input',
                'type': 'string',
                'optional': False,
                'value': 'KAWABANGA',
                'active': False,
            },
            {
                'name': 'result',
                'type': 'string',
                'optional': True,
                'value': None,
                'active': False,
            },
        ],
    })


@pytest.fixture(scope='module')
def pa_app(vins_app):
    return vins_app._vins_app


@pytest.fixture(scope='function')
def session(form):
    session = Session('app', 'uuid')
    session.change_form(form)
    session.change_intent(Intent(form.name))
    return session


@pytest.fixture(scope='function')
def form_info_dict():
    return {
        'name': 'test-form',
        'slots': [
            {
                'name': 'input',
                'type': 'string',
                'optional': False,
                'value': 'KAWABANGA'
            },
            {
                'name': 'result',
                'type': 'string',
                'optional': True,
                'value': None
            }
        ]
    }


@pytest.fixture
def req_info():
    return ReqInfo(
        request_id=str(gen_uuid_for_tests()),
        client_time=utcnow(),
        uuid=str(gen_uuid_for_tests()),
        app_info=AppInfo(
            app_id='com.yandex.vins.tests',
            app_version='0.0.1',
            os_version='1',
            platform='unknown'
        ),
    )


def test_update_slot_value(pa_app, session, req_info, form_info_dict):
    form_info_dict['slots'][1]['value'] = 'XXX'
    form, should_resubmit = pa_app._update_form(session.form, req_info, BassFormInfo.from_dict(form_info_dict))
    assert form.result.value == 'XXX'
    assert should_resubmit is False


def test_make_slot_required(pa_app, session, req_info, form_info_dict):
    form_info_dict['slots'][1]['optional'] = False
    form, should_resubmit = pa_app._update_form(session.form, req_info, BassFormInfo.from_dict(form_info_dict))
    assert form.result.optional is False
    assert should_resubmit is True


def test_make_slot_optional(pa_app, session, req_info, form_info_dict):
    form_info_dict['slots'][0]['optional'] = False
    form, should_resubmit = pa_app._update_form(session.form, req_info, BassFormInfo.from_dict(form_info_dict))
    assert form.input.value == 'KAWABANGA'
    assert form.input.optional is False
    assert should_resubmit is False


def test_clear_required_slot_value(pa_app, session, req_info, form_info_dict):
    form_info_dict['slots'][0]['value'] = None
    form, should_resubmit = pa_app._update_form(session.form, req_info, BassFormInfo.from_dict(form_info_dict))
    assert form.input.value is None
    assert should_resubmit is True


def test_change_form(pa_app, session, req_info):
    form_info_dict = {
        'name': 'personal_assistant.scenarios.convert',
        'slots': []
    }
    form, should_resubmit = pa_app._update_form(session.form, req_info, BassFormInfo.from_dict(form_info_dict))
    assert form.name == 'personal_assistant.scenarios.convert'
    assert should_resubmit is True


def test_set_new_form(pa_app, session, req_info):
    session.form.name = 'personal_assistant.handcrafted.hello'
    assert len(session.form.slots) == 2
    form_info_dict = {
        'name': 'personal_assistant.handcrafted.hello',
        'slots': [],
        'set_new_form': True
    }
    form, should_resubmit = pa_app._update_form(session.form, req_info, BassFormInfo.from_dict(form_info_dict))
    assert len(form.slots) == 0
