# coding: utf-8

from __future__ import unicode_literals

from operator import attrgetter

import pytest
from freezegun import freeze_time

from vins_core.common.utterance import Utterance
from vins_core.dm.dialog_history import (
    DialogHistory, Phrase, remove_tts_markup, remove_spaces,
)
from vins_core.dm.response import VinsResponse


@pytest.fixture
def dialog_history():
    return DialogHistory(max_turns=5)


def test_dialog_history(dialog_history):
    dialog_history.add(
        utterance=Utterance('привет'),
        response=VinsResponse().say('здравствуй')
    )
    dialog_history.add(
        utterance=Utterance('как дела?'),
        response=VinsResponse().ask('хорошо, а у тебя?')
    )

    last = iter(dialog_history.last(2))
    assert next(last).utterance.text == 'как дела?'
    assert next(last).utterance.text == 'привет'

    assert map(attrgetter('text'), dialog_history.last_phrases(count=4)) == [
        'привет',
        'здравствуй',
        'как дела?',
        'хорошо, а у тебя?',
    ]


def test_history_size(dialog_history):
    dialog_history._max_turns = 2

    for _ in xrange(10):
        dialog_history.add(
            utterance=Utterance('привет'),
            response=VinsResponse().say('здравствуй')
        )

    assert len(dialog_history) == 2


def test_response_text(dialog_history):
    dialog_history.add(
        utterance=Utterance('привет'),
        response=VinsResponse().say('Привет .sil<[1000]> Новость 1:\nВот она', '')
    )

    assert dialog_history.last().response_text == 'Привет Новость 1: Вот она'


@freeze_time('2016-10-26 21:13')
def test_to_dict(dialog_history):
    dialog_history.add(
        utterance=Utterance('привет'),
        response=VinsResponse().say('здравствуй'),
    )

    assert dialog_history.to_dict() == {
        'app_id': 'vins',
        'uuid': '__uuid:deadbeef-6898-4c0a-af93-7a8821aab244',

        'turns': [
            {
                'annotations': None,
                'dt': 1477516380,
                'utterance': {'input_source': 'text', 'text': 'привет'},
                'response': {
                    'cards': [{'type': 'simple_text', 'text': 'здравствуй', 'tag': None}],
                    'templates': {},
                    'directives': [],
                    'meta': [],
                    'should_listen': None,
                    'special_buttons': [],
                    'force_voice_answer': False,
                    'autoaction_delay_ms': None,
                    'suggests': [],
                    'voice_text': 'здравствуй',
                    'features': {},
                    'frame_actions': {},
                    'scenario_data': {},
                    'stack_engine': None,
                },
                'form': None,
                'response_text': 'здравствуй',
                'voice_text': 'здравствуй'
            }
        ]
    }


def test_from_dict():
    dh = DialogHistory.from_dict({
        'app_id': 'vins',
        'uuid': '123',

        'turns': [
            {
                # Add not registered annotation to check deserialization does not raise.
                'annotations': {
                    'jd12308dj19j__not_registered_annotation': {
                        'type': 'jd12308dj19j__not_registered_annotation',
                        'value': {
                            'some key': 'some value'
                        }
                    }
                },
                'dt': 1477516380,
                'utterance': {'input_source': 'text', 'text': 'привет'},
                'response': {
                    'cards': [{'type': 'simple_text', 'text': 'здравствуй', 'tag': None}],
                    'directives': [],
                    'meta': [],
                    'should_listen': None,
                    'force_voice_answer': False,
                    'autoaction_delay_ms': None,
                    'special_buttons': [],
                    'suggests': [],
                    'voice_text': 'здравствуй',
                    'features': {},
                    'frame_actions': {},
                    'scenario_data': {},
                    'stack_engine': None,
                },
                'form': None,
                'response_text': 'здравствуй',
                'voice_text': 'здравствуй',
                'sessions': None,
            }
        ]
    })

    assert dh.to_dict() == {
        'app_id': 'vins',
        'uuid': '__uuid:deadbeef-6898-4c0a-af93-7a8821aab244',

        'turns': [
            {
                'annotations': {},
                'dt': 1477516380,
                'utterance': {'input_source': 'text', 'text': 'привет'},
                'response': {
                    'cards': [{'type': 'simple_text', 'text': 'здравствуй', 'tag': None}],
                    'templates': {},
                    'directives': [],
                    'meta': [],
                    'should_listen': None,
                    'special_buttons': [],
                    'force_voice_answer': False,
                    'autoaction_delay_ms': None,
                    'suggests': [],
                    'voice_text': 'здравствуй',
                    'features': {},
                    'frame_actions': {},
                    'scenario_data': {},
                    'stack_engine': None,
                },
                'form': None,
                'response_text': 'здравствуй',
                'voice_text': 'здравствуй',
            }
        ]
    }


def test_last_phrases(dialog_history):
    for i in xrange(10):
        dialog_history.add(
            utterance=Utterance(str(i)),
            response=VinsResponse().say(str(i)),
        )

    assert dialog_history.last_phrases(3) == [
        Phrase(text='8', sender='vins'),
        Phrase(text='9', sender='user'),
        Phrase(text='9', sender='vins')
    ]


def test_last(dialog_history):
    for i in xrange(10):
        dialog_history.add(
            utterance=Utterance(str(i)),
            response=VinsResponse().say(str(i)),
        )

    assert [
        {'utt': turn.utterance.text, 'resp': turn.response_text}
        for turn in dialog_history.last(4)
    ] == [
        {'utt': '9', 'resp': '9'},
        {'utt': '8', 'resp': '8'},
        {'utt': '7', 'resp': '7'},
        {'utt': '6', 'resp': '6'},
    ]


@pytest.mark.parametrize('text,resp', [
    ('', ''),
    ('just text', 'just text'),
    ('a .sil<[1000]> b', 'a  b'),
    ('a #gen b', 'a  b'),
    ('goodbye <speacker voice="shitova.us"> cruel <speacker voice="valtz"> world', 'goodbye  cruel  world'),
    ('test <[123]> test', 'test  test'),
    ('рыба <[ mm aa s schwa | case=nom ]> моей мечты', 'рыба  моей мечты'),
    ('счастл+иво оставаться', 'счастливо оставаться'),
    ('рыба <[ mm aa | case=nom ] ненастоящая', 'рыба <[ mm aa | case=nom ] ненастоящая'),
])
def test_remove_tss_markup(text, resp):
    assert remove_tts_markup(text) == resp


@pytest.mark.parametrize('text,resp', [
    ('', ''),
    ('\t \n\n\n    ', ' '),
    ('a \nb', 'a b'),
    ('a  b', 'a b'),
    ('a\nb', 'a b'),
    ('a b', 'a b'),
])
def test_remove_spaces(text, resp):
    assert remove_spaces(text) == resp
