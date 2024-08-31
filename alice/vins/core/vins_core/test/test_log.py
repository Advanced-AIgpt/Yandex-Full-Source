# coding: utf-8
from __future__ import unicode_literals

from datetime import datetime
from uuid import uuid4

from vins_core.dm.session import Session
from vins_core.dm.request import create_request
from vins_core.dm.response import VinsResponse
from vins_core.logger.dialog_log import _create_dialog_log_entry


def test_cookies_cleanup():
    uuid = uuid4()
    session = Session(app_id='test_app', uuid=uuid)
    req_info = create_request(
        uuid=uuid,
        client_time=datetime.now(),
        app_info=None,
        additional_options={
            'bass_options': {
                'cookies': [
                    'user_id=foobar',
                    'session_id=XXXXXX'
                ]
            }
        },
        utterance='Hello world!'
    )
    log_entry = _create_dialog_log_entry(session, req_info, VinsResponse())
    cookies = log_entry.get('request', {}).get('additional_options', {}).get('bass_options', {}).get('cookies', [])
    assert len(cookies) == 1
    assert 'user_id=foobar' in cookies


def test_null_cookies_cleanup():
    uuid = uuid4()
    session = Session(app_id='test_app', uuid=uuid)
    req_info = create_request(
        uuid=uuid,
        client_time=datetime.now(),
        app_info=None,
        additional_options={
            'bass_options': {
                'cookies': None
            }
        },
        utterance='Hello world!'
    )
    log_entry = _create_dialog_log_entry(session, req_info, VinsResponse())
    bass_options = log_entry.get('request', {}).get('additional_options', {}).get('bass_options', {})
    assert 'cookies' not in bass_options
