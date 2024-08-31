# coding: utf-8

from vins_core.dm.request_events import VoiceInputEvent
from vins_api.speechkit.schemas import header_schema, voice_input_event_schema

import pytest
import jsonschema


@pytest.mark.parametrize('header,is_valid', [
    # old speechkit
    ({}, False),
    ({'request_id': 'qwe'}, True),
    ({'request_id': None}, False),

    # sequence
    ({'request_id': '', 'prev_req_id': None, 'sequence_number': 0}, True),
    ({'request_id': '', 'prev_req_id': 'qwe', 'sequence_number': 0}, True),
    ({'request_id': '', 'prev_req_id': '', 'sequence_number': 0}, True),

    ({'request_id': '', 'prev_req_id': '', 'sequence_number': -1}, False),
    ({'request_id': '', 'prev_req_id': '', 'sequence_number': -1}, False),
    ({'request_id': '', 'sequence_number': 0}, False),
    ({'sequence_number': 0}, False),
])
def test_header_validation(header, is_valid):
    if is_valid:
        jsonschema.validate(header, header_schema)
    else:
        with pytest.raises(jsonschema.ValidationError):
            jsonschema.validate(header, header_schema)


@pytest.mark.parametrize('kwargs', [
    {},
    {'hypothesis_number': 0},
    {'end_of_utterance': False},
    {'biometry_scoring': {'scores': [{'user_id': '123', 'score': 1}], 'status': 'ok'}},
    pytest.param({'biometry_scoring': {'scores': []}}, marks=pytest.mark.xfail(raises=jsonschema.ValidationError, strict=True)),
])
def test_voice_event(kwargs):
    event = VoiceInputEvent.from_utterance('test', **kwargs)
    jsonschema.validate(event.to_dict(), voice_input_event_schema)
