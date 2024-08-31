#!/usr/bin/env python
# -*- coding: utf-8 -*-

import functools

import pytest

from tests.basic import server, connect
from tests.asr import SessionContext, music_file
from tests.mock_backend import mock_music_and_asr, mock_uaas


_PCM_FILE = "tests/test.pcm"


def test_mocked_simple_asr(server, monkeypatch):
    mock_uaas(monkeypatch)
    mock_music_and_asr(monkeypatch)
    ctx = SessionContext()
    with open(_PCM_FILE, 'rb') as audio_file:
        connect(server, functools.partial(ctx.on_connect_asr, audio_file), ctx.on_message_asr, 20)


@pytest.fixture(scope="module",
                params=[2])
def music_api_ver(request):
    return request.param


def test_mocked_music_asr(server, music_file, monkeypatch, music_api_ver):
    mock_music_and_asr(monkeypatch)

    ctx = SessionContext()
    expected_directives = set(('ASR.Result', 'ASR.MusicResult'))

    def on_multiple_asr_messages(msg):
        directive = ctx.on_message_asr(msg, expected_directives)
        if directive:
            expected_directives.remove(directive)

    connect(
        server,
        functools.partial(ctx.on_connect_asr, music_file, music_request=music_api_ver),
        on_multiple_asr_messages,
        15,
    )
