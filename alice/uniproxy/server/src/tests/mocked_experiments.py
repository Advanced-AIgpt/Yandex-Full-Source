#!/usr/bin/env python
import sys

import collections
from tornado.ioloop import IOLoop
import json
import pytest
import functools
import struct
import uuid
from tests.basic import Session, server, connect
from tests.mock_backend import mock_experiments, mock_uaas
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config


def test_mocked_experiment(server, monkeypatch):
    mock_uaas(monkeypatch)
    mock_experiments(monkeypatch)
    connect(server, on_connect, on_message, 5)


def on_connect(socket):
    socket.write_message(json.dumps({
        'event': {
            'header': {
                'namespace': 'System',
                'name': 'SynchronizeState',
                'messageId': str(uuid.uuid4()),
            },
            'payload': {
                'auth_token': config['key'],
                'uuid': 'f' * 16 + str(uuid.uuid4()).replace('-', '')[16:32],
                'yandexuid': '1234567890',
            }
        }
    }))

    message_id = str(uuid.uuid4())
    event = {
        'event': {
            'header': {
                'namespace': 'System',
                'name': 'EchoRequest',
                'messageId': message_id,
            },
            'payload': {
                'lang': 'ru-RU',
                'topic': 'queries',
                'application': 'test',
            }
        }
    }
    socket.write_message(json.dumps(event))


def on_message(msg, expected_directives=['System.EchoResponse', ], ignored_directives=['Messenger.BackendVersions', ]):
    if msg is None:
        Session.fuckit('Socket closed unexpected')
        return

    if isinstance(msg, bytes):
        Session.fuckit('Server returned some binary data')
        return
    try:
        res = json.loads(msg)
        if 'directive' not in res:
            Session.fuckit('Server returned unwanted message')
            return
        else:
            directive = res.get('directive').get('header', {}).get('namespace', '') + '.' + \
                res.get('directive').get('header', {}).get('name', '')
            Logger.get().debug('DIRECTIVE: ' + str(res))
            if directive not in expected_directives:
                if directive not in ignored_directives:
                    Session.fuckit('Wrong directive [1]: {}'.format(directive))
                return

            patch = res.get('directive').get('payload', {}).get('patch')
            if not patch or patch != 'patch_data':
                Session.fuckit('Experiment patching not work!')
                return

            patch2 = res.get('directive').get('payload', {}).get('patch2', '')
            if patch2 == 'patch2_data':
                Logger.get().debug('='*80)
                Logger.get().debug('got 1/2 experiment patch')
                Logger.get().debug('='*80)
            if len(expected_directives) == 1:
                # receive last directive, so finish
                IOLoop.current().stop()
            return directive
        return

    except Exception as e:
        Session.fuckit('Exception in test: {}'.format(e))
        return
