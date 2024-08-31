# coding: utf-8

from vins_core.config.schemas import special_buttons_schema

import pytest
import jsonschema


@pytest.mark.parametrize('special_button,is_valid', [
    ({'type': 'like_button', 'text': 'some_text', 'name': 'feedback_positive'}, True),
    ({'type': 'repost_button', 'text': 'some_text', 'name': 'repost'}, False),
    ({'title': 'some_title', 'text': 'some_text', 'name': 'feedback_some'}, True),
    ({
        "type": "dislike_button",
        "text": "Не нравится.",
        "name": "feedback_negative",
        "sub_list": {
            "title": "Что вам не понравилось?",
            "default_buttons": [
                {
                    "title": "Ответ не соответствует запросу",
                    "text": "Не нравится. Не соответсвует запросу.",
                    "name": "feedback_negative__bad_answer"
                }
            ],
            "default":
            {
                "title": "Моя речь не распозналась",
                "text": "Не нравится. Моя речь не распозналась.",
                "name": "feedback_negative__asr_error",
            }
        }
    }, True),
    ({'title': 'some_title', 'name': 'feedback_some'}, False),
    ({
        "type": "dislike_button",
        "text": "Не нравится.",
        "name": "feedback_negative",
        "sub_list": {
            "title": "Что вам не понравилось?",
            "default_buttons": [
                {
                    "title": "Моя речь не распозналась",
                    "text": "Не нравится. Моя речь не распозналась.",
                    "name": "feedback_negative__asr_error",
                }
            ]
        }
    }, False),
])
def test_special_buttons_validation(special_button, is_valid):
    special_buttons_json = {"special_buttons": [special_button]}
    if is_valid:
        jsonschema.validate(special_buttons_json, special_buttons_schema)
    else:
        with pytest.raises(jsonschema.ValidationError):
            jsonschema.validate(special_buttons_json, special_buttons_schema)
