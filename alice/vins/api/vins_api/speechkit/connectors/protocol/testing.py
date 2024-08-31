# coding: utf-8

from __future__ import unicode_literals

import logging
import re
import pytz

from datetime import datetime
from uuid import uuid4

from google.protobuf import json_format

from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest, TScenarioApplyRequest

from personal_assistant.testing_framework import TEST_REQUEST_ID

from vins_api.speechkit.connectors.protocol import match
from vins_api.speechkit.connectors.protocol.directives import serialize_directive
from vins_api.speechkit.resources.protocol import _FORBIDDEN_DIRECTIVE_NAMES
from vins_core.dm.request_events import (
    TextInputEvent, VoiceInputEvent, SuggestedInputEvent, ServerActionEvent, ImageInputEvent, MusicInputEvent
)
from vins_core.utils.datetime import datetime_to_timestamp
from vins_sdk.connectors import YANDEX_OFFICE_LOCATION

logger = logging.getLogger(__name__)


def _fill_base_request(base_request, utterance_data, test_data, uuid=None, state=None):
    base_request.ClientInfo.Uuid = uuid or str(uuid4())
    dt = datetime.now(tz=pytz.UTC)
    base_request.ClientInfo.ClientTime = dt.strftime('%Y%m%dT%H%M%S')
    base_request.ClientInfo.Epoch = str(datetime_to_timestamp(dt))
    base_request.ClientInfo.Timezone = str(pytz.UTC)
    base_request.ClientInfo.Lang = utterance_data.lang
    base_request.RequestId = TEST_REQUEST_ID

    app_info = test_data.app_info
    if app_info is not None:
        base_request.ClientInfo.AppId = app_info.app_id
        base_request.ClientInfo.AppVersion = app_info.app_version
        base_request.ClientInfo.OsVersion = app_info.os_version
        base_request.ClientInfo.Platform = app_info.platform
        base_request.ClientInfo.DeviceManufacturer = app_info.device_manufacturer or ''
        base_request.ClientInfo.DeviceModel = app_info.device_model or ''
    else:
        base_request.ClientInfo.AppId = 'NoneApp'

    geo_info = test_data.geo_info
    if geo_info is not None:
        base_request.Location.Lat = geo_info.get('lat', YANDEX_OFFICE_LOCATION['lat'])
        base_request.Location.Lon = geo_info.get('lon', YANDEX_OFFICE_LOCATION['lon'])

    json_format.ParseDict(utterance_data.experiments or {}, base_request.Experiments)

    device_state = test_data.device_state or {}
    if 'filtration_level' in device_state:
        base_request.Options.FiltrationLevel = device_state['filtration_level']
        del device_state['filtration_level']
    json_format.ParseDict(device_state, base_request.DeviceState)

    for station in test_data.additional_options.get('radiostations', []):
        base_request.Options.RadioStations.append(station)

    if state is not None:
        base_request.State.Pack(state)

    # All functional/integration tests check should_listen before voice_session logic in connector.
    base_request.Interfaces.VoiceSession = True


def _fill_input(request, event):
    if isinstance(event, (TextInputEvent, SuggestedInputEvent)):
        request.Input.Text.RawUtterance = event.utterance.text
        request.Input.Text.Utterance = event.utterance.text
        request.Input.Text.FromSuggest = isinstance(event, SuggestedInputEvent)
    elif isinstance(event, VoiceInputEvent):
        request.Input.Voice.Utterance = event.utterance.text
        event_dict = event.to_dict()
        asr_data = request.Input.Voice.AsrData.add()
        json_format.ParseDict(event_dict['asr_result'][0], asr_data)
        if 'biometry_scoring' in event_dict:
            json_format.ParseDict(event_dict['biometry_scoring'], request.Input.Voice.BiometryScoring)
    elif isinstance(event, ServerActionEvent):
        request.Input.Callback.Name = event.action_name
        json_format.ParseDict(event.payload, request.Input.Callback.Payload)
    elif isinstance(event, ImageInputEvent):
        request.Input.Image.Url = event.utterance.payload['data'].get('img_url', '')
    elif isinstance(event, MusicInputEvent):
        data = event.utterance.payload.get('data')
        result = event.utterance.payload.get('result')
        error_text = event.utterance.payload.get('error_text')
        if data:
            request.Input.Music.MusicResult.Data.Engine = data.get('engine', '')
            request.Input.Music.MusicResult.Data.RecognitionId = data.get('recognition-id', '')
            request.Input.Music.MusicResult.Data.Url = data.get('url', '')
            match = data.get('match')
            if match:
                json_format.ParseDict(match, request.Input.Music.MusicResult.Data.Match)
        if result:
            request.Input.Music.MusicResult.Result = result
        if error_text:
            request.Input.Music.MusicResult.ErrorText = error_text


def make_run_request(utterance_data, test_data, uuid=None, state=None):
    request = TScenarioRunRequest()
    logging.debug('Event: %s Utt: %s', utterance_data.event.event_type, utterance_data.event.utterance)
    _fill_base_request(request.BaseRequest, utterance_data, test_data, uuid, state)
    _fill_input(request, utterance_data.event)
    return request


def _make_post_run_request(utterance_data, test_data, uuid, state, apply_arguments):
    request = TScenarioApplyRequest()
    _fill_base_request(request.BaseRequest, utterance_data, test_data, uuid, state)
    _fill_input(request, utterance_data.event)
    request.Arguments.CopyFrom(apply_arguments)
    return request


def make_apply_request(utterance_data, test_data, uuid, state, apply_arguments):
    return _make_post_run_request(utterance_data, test_data, uuid, state, apply_arguments)


def make_continue_request(utterance_data, test_data, uuid, state, apply_arguments):
    return _make_post_run_request(utterance_data, test_data, uuid, state, apply_arguments)


def _get_text_response(layout):
    text_response = []
    for card in layout.Cards:
        card_type = card.WhichOneof('Card')
        if card_type == 'Text':
            text_response.append(card.Text.decode('utf-8'))
        elif card_type == 'TextWithButtons':
            text_response.append(card.TextWithButtons.Text.decode('utf-8'))
        elif card_type == 'DivCard':
            text_response.append('...')
    text_response = '\n'.join(text_response)
    return text_response


def match_directive(expected, actual):
    return expected == actual


def match_text(pattern, text, hint, request):
    unexpected = 'Unexpected {} response "{}" for request "{}" instead of "{}"'
    if pattern and not re.match(pattern, text, re.UNICODE | re.DOTALL):
        return unexpected.format(hint, text, request, pattern)
    return None


def match_bool(expected, actual, hint, request):
    unexpected = '{} is {} for request "{}", while the opposite is expected'
    if expected is not None and expected != actual:
        return unexpected.format(hint, actual, request)
    return None


def match_response(test_data, response):
    layout = response.ResponseBody.Layout

    error = match_text(test_data.text_regexp, _get_text_response(layout), 'text', test_data.request)
    if error is not None:
        return False, error

    error = match_text(test_data.voice_regexp, layout.OutputSpeech.decode('utf-8'), 'voice', test_data.request)
    if error is not None:
        return False, error

    error = match_bool(test_data.should_listen, layout.ShouldListen, 'should_listen', test_data.request)
    if error is not None:
        return False, error

    expected_directives = test_data.directives or []
    expected_directives = [serialize_directive(directive) for directive in expected_directives]
    if test_data.exact_directives_match:
        for expected, actual in zip(expected_directives, layout.Directives):
            if not match_directive(expected, actual):
                return False, 'Exact match failed on directive "{}" for request "{}" while "{}" is expected'.format(
                    actual, test_data.request, expected)
    else:
        for expected in expected_directives:
            for actual in layout.Directives:
                if match_directive(expected, actual):
                    break
            else:
                return False, 'Unable to find directive "{}" in directive set for request "{}"'.format(
                    expected, test_data.request)
    actions = response.ResponseBody.FrameActions
    error = match.suggests(test_data.suggests,
                           test_data.exact_suggests_match,
                           [e.ActionButton for e in layout.SuggestButtons],
                           actions,
                           _FORBIDDEN_DIRECTIVE_NAMES)
    if error is not None:
        return False, error

    error = match.button_actions(test_data.button_actions, layout.Cards, actions, _FORBIDDEN_DIRECTIVE_NAMES)
    if error is not None:
        return False, error

    return True, None


def process_request(vins_app, utterance_data, dialog_test_data, uuid, state):
    request = make_run_request(utterance_data, dialog_test_data, uuid, state)
    response = vins_app.handle_run(request)
    if response.WhichOneof('Response') == 'ApplyArguments':
        logger.debug('DeferApply Response: %s', response)
        request = make_apply_request(utterance_data, dialog_test_data, uuid, state, response.ApplyArguments)
        response = vins_app.handle_apply(request)

    elif response.WhichOneof('Response') == 'ContinueArguments':
        logger.debug('Continue Response: %s', response)
        request = make_continue_request(utterance_data, dialog_test_data, uuid, state, response.ApplyArguments)
        response = vins_app.handle_continue(request)

    return response
