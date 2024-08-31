# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import pytest

from vins_core.ext.general_conversation import GeneralConversationAPI, gc_mock
from vins_core.dm.request import Experiments


@pytest.mark.parametrize('api_version', [1, 2])
def test_general_conversation(api_version):
    gc = GeneralConversationAPI(api_version=api_version)

    with gc_mock(context=['привет'], response='как дела? _EOS_', api_version=api_version):
        gc_response = gc.handle(prev_phrases=['привет'],
                                experiments=Experiments())[0]
        assert gc_response.text == 'как дела?'
        assert gc_response.source is None

    with gc_mock(context=['привет', 'и тебе привет', 'как дела'], response='норм _EOS_', api_version=api_version):
        gc_response = gc.handle(prev_phrases=['привет', 'и тебе привет', 'как дела'],
                                experiments=Experiments())[0]
        assert gc_response.text == 'норм'
        assert gc_response.source is None


def test_gc_source():
    gc = GeneralConversationAPI(api_version=2)

    with gc_mock(context=['давай', 'проверим', 'источник'], response='ок _EOS_', api_version=2,
                 source='source_0'):
        gc_response = gc.handle(prev_phrases=['давай', 'проверим', 'источник'],
                                experiments=Experiments())[0]
        assert gc_response.text == 'ок'
        assert gc_response.source == 'source_0'


def test_gc_proactivity():
    gc = GeneralConversationAPI(api_version=2)

    with gc_mock(context=['давай', 'проверим', 'proactivity'], response='Хотите включить музыку? _EOS_', api_version=2,
                 source='proactivity', action='personal_assistant.scenarios.music_play'):
        gc_response = gc.handle(prev_phrases=['давай', 'проверим', 'proactivity'],
                                experiments=Experiments())[0]
        assert gc_response.text == 'Хотите включить музыку?'
        assert gc_response.source == 'proactivity'
        assert gc_response.action == 'personal_assistant.scenarios.music_play'


@pytest.mark.parametrize('seq2seq', [False, True])
def test_seq2seq(seq2seq):
    gc = GeneralConversationAPI(api_version=2)

    with gc_mock(context=['давай', 'проверим', 'источник'], response='ок _EOS_', api_version=2, seq2seq=seq2seq, seq2seq_feature=True):
        gc_response = gc.handle(prev_phrases=['давай', 'проверим', 'источник'],
                                experiments=Experiments())[0]
        assert gc_response.text == 'ок'
        if seq2seq:
            assert gc_response.source == 'seq2seq'


@pytest.mark.parametrize('api_version', [1, 2])
def test_debug_prefix(api_version):
    gc = GeneralConversationAPI(add_debug_prefix=True, api_version=api_version)

    with gc_mock(context=['ах ты ж пес'], response='как дела? _EOS_', api_version=api_version):
        assert gc.handle(prev_phrases=['ах ты ж пес'],
                         experiments=Experiments())[0].text == '[Говорилка] как дела?'


def test_quote_unescape():
    gc = GeneralConversationAPI(api_version=2)
    with gc_mock(context=['ах ты ж пес'], response='\\"привет\\\'', api_version=2):
        assert gc.handle(prev_phrases=['ах ты ж пес'], experiments=Experiments())[0].text == '"привет\''


def test_experiment_forwarding():
    gc = GeneralConversationAPI(api_version=2)
    with gc_mock(context=['ах ты ж пес'], response='нет', api_version=2,
                 additional_params='&pron=exp_gc_a&pron=exp_gc_b'):
        gc.handle(prev_phrases=['ах ты ж пес'], experiments=Experiments({'gc_a': True, 'other': True, 'gc_b': True}))
