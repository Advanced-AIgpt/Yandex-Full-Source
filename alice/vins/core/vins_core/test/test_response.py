# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.dm.response import (VinsResponse, ErrorMeta, FormRestoredMeta, FeaturesExtractorErrorMeta,
                                   LikeButton, DislikeButton, ClientActionDirective, ServerActionDirective,
                                   ThemedActionButton)


@pytest.fixture(scope='module')
def special_buttons():
    data = [
        {
            "text": "Нравится",
            "title": "",
            "directives": [
                {
                    "type": "server_action",
                    "name": "on_special_button",
                    "ignore_answer": True,
                    "payload": {
                        "type": "like_button",
                        "name": "feedback_positive",
                        "request_id": ""
                    }
                }
            ],
            "type": "like_button"
        },
        {
            "text": "Не нравится",
            "title": "",
            "directives": [
                {
                    "type": "server_action",
                    "name": "on_special_button",
                    "ignore_answer": True,
                    "payload": {
                        "type": "dislike_button",
                        "name": "feedback_negative",
                        "request_id": ""
                    }
                },
                {
                    "type": "client_action",
                    "name": "special_button_list",
                    "payload": {
                        "key": "value"
                    }
                }
            ],
            "type": "dislike_button"
        }
    ]

    return data


def test_to_dict_from_dict():
    response = VinsResponse()
    response.say('hello')
    response.add_meta(FeaturesExtractorErrorMeta(features_extractor='extractor_name'))
    response.add_meta(ErrorMeta(error_type='some_error'))
    response.add_meta(FormRestoredMeta(overriden_form='form_name'))
    data = response.to_dict()
    assert data == {
        'cards': [{'text': 'hello', 'type': 'simple_text', 'tag': None}],
        'directives': [],
        'templates': {},
        'meta': [
            {
                'error_type': 'features_extractor_error',
                'features_extractor': 'extractor_name',
                'type': 'error',
                'form_name': None
            },
            {'error_type': 'some_error', 'type': 'error', 'form_name': None},
            {'overriden_form': 'form_name', 'type': 'form_restored'}
        ],
        'should_listen': None,
        'special_buttons': [],
        'force_voice_answer': False,
        'autoaction_delay_ms': None,
        'suggests': [],
        'voice_text': 'hello',
        'features': {},
        'frame_actions': {},
        'scenario_data': {},
        'stack_engine': None,
    }
    assert VinsResponse.from_dict(data) == response


def test_special_buttons_from_dict(special_buttons):
    data = {
        'cards': [{'text': 'hello', 'type': 'simple_text', 'tag': None}],
        'directives': [],
        'meta': [],
        'should_listen': None,
        'special_buttons': special_buttons,
        'force_voice_answer': False,
        'suggests': [],
        'voice_text': 'hello'
    }
    response = VinsResponse.from_dict(data)
    assert len(response.special_buttons) == 2
    assert isinstance(response.special_buttons[0], LikeButton)
    assert isinstance(response.special_buttons[1], DislikeButton)
    assert len(response.special_buttons[1].directives) == 2
    assert isinstance(response.special_buttons[1].directives[0], ServerActionDirective)
    assert isinstance(response.special_buttons[1].directives[1], ClientActionDirective)
    assert response.special_buttons[1].directives[0].payload == {
        "type": "dislike_button",
        "name": "feedback_negative",
        "request_id": ""
    }
    assert response.special_buttons[1].directives[1].payload == {
        "key": "value",
    }


def test_themed_buttons_from_dict():
    data = {
        'cards': [{'text': 'hello', 'type': 'simple_text', 'tag': None}],
        'directives': [],
        'meta': [],
        'should_listen': None,
        'force_voice_answer': False,
        'suggests': [{
            "type": "themed_action",
            "theme": {
                "image_url": "avatar_url"
            },
            "directives": [],
            "title": "Искать в Яндексе"
        }],
        'voice_text': 'hello'
    }
    response = VinsResponse.from_dict(data)
    assert len(response.suggests) == 1
    assert isinstance(response.suggests[0], ThemedActionButton)
    assert response.suggests[0].theme['image_url'] == 'avatar_url'


@pytest.mark.parametrize('strict', [pytest.param(True, marks=pytest.mark.xfail()), False])
def test_fail_on_unknown_meta_if_strict(strict):
    data = {
        'cards': [{'text': 'hello', 'type': 'simple_text', 'tag': None}],
        'directives': [],
        'meta': [{'type': 'unknown_meta'}],  # There is no such meta registered
        'should_listen': None,
        'force_voice_answer': False,
        'suggests': [],
        'voice_text': 'hello',
    }

    VinsResponse.from_dict(data, strict=strict)


def test_error_meta_from_dict():
    data_empty_form_name = {
        'error_type': 'some_error'
    }
    error_meta = ErrorMeta.from_dict(data_empty_form_name)
    assert error_meta.form_name is None
    assert error_meta.error_type == 'some_error'
    assert error_meta.type == 'error'

    data_with_form_name = {
        'error_type': 'some_error',
        'form_name': 'some_form_name'
    }
    error_meta = ErrorMeta.from_dict(data_with_form_name)
    assert error_meta.form_name == 'some_form_name'
    assert error_meta.error_type == 'some_error'
    assert error_meta.type == 'error'
