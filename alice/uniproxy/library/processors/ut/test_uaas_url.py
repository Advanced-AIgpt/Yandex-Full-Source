from alice.uniproxy.library.events.event import Event
from alice.uniproxy.library.utils.experiments import safe_experiments_vins_format
from alice.uniproxy.library.processors import create_event_processor
from alice.uniproxy.library.processors.vins import VinsProcessor
from alice.uniproxy.library.processors.base_event_processor import EventProcessor

import logging
import common
import time

EVENT_DATA = {
    "header": {
        "namespace": "Vins",
        "name": "TextInput",
        "messageId": "2d175502-58a2-11ea-a82c-525400123456",
    },
    "payload": {
        "application": {
            "app_id": "ru.yandex.searchplugin.beta",
            "app_version": "1.2.3",
            "os_version": "5.0",
            "platform": "android",
            "lang": "ru-RU",
            "timezone": "Europe/Moscow",
        },
        "request": {
            "additional_options": {
                "bass_options": {
                    "filtration_level": 1,
                    "user_agent": "monitoring"
                }
            },
            "event": {
                "type": "text_input"
            },
            "experiments": [
                "UaasVinsUrl_aHR0cDovL3ZpbnMuYWxpY2UueWFuZGV4Lm5ldC9hcHBob3N0"
            ],
            "location": {
                "accuracy": 140,
                "lat": 52.26052093505859,
                "lon": 104.1884078979492,
                "recency": 192321
            },
            "reset_session": False,
            "voice_session": True
        },
    }
}


def test_create_vins_processor():
    ev = Event(EVENT_DATA)
    text_input = create_event_processor(common.FakeSystem(), ev)
    assert text_input.event_type == "vins.textinput"
    EventProcessor.process_event(text_input, ev)
    assert 'uaasVinsUrl' not in text_input.event.payload
    assert 'uaasVinsUrl' not in text_input.payload_with_session_data

    text_input.payload_with_session_data['request']['experiments'] = safe_experiments_vins_format(text_input.payload_with_session_data['request']['experiments'], logging.warn)

    VinsProcessor._try_url_experiments(text_input)
    assert 'uaasVinsUrl' not in text_input.event.payload
    assert 'uaasVinsUrl' in text_input.payload_with_session_data


def test_not_force_update_uaas_vins_url():
    ev = Event(EVENT_DATA)
    text_input = create_event_processor(common.FakeSystem(), ev)
    assert text_input.event_type == "vins.textinput"
    EventProcessor.process_event(text_input, ev)
    time.sleep(0.5)
    assert 'uaasVinsUrl' not in text_input.event.payload
    assert 'uaasVinsUrl' not in text_input.payload_with_session_data

    text_input.payload_with_session_data['request']['experiments'] = safe_experiments_vins_format(text_input.payload_with_session_data['request']['experiments'], logging.warn)

    VinsProcessor._try_url_experiments(text_input)
    assert 'uaasVinsUrl' not in text_input.event.payload
    assert 'uaasVinsUrl' in text_input.payload_with_session_data
    old_value = text_input.payload_with_session_data.get('uaasVinsUrl')
    VinsProcessor._try_use_vins_url(text_input, text_input.payload_with_session_data, 'UaasVinsUrl_aHR0cDovL21lZ2FtaW5kLXJjLmFsaWNlLnlhbmRleC5uZXQ=', force=False)

    assert 'uaasVinsUrl' not in text_input.event.payload
    assert 'uaasVinsUrl' in text_input.payload_with_session_data
    assert text_input.payload_with_session_data.get('uaasVinsUrl') == old_value


def test_force_update_uaas_vins_url():
    ev = Event(EVENT_DATA)
    text_input = create_event_processor(common.FakeSystem(), ev)
    assert text_input.event_type == "vins.textinput"
    EventProcessor.process_event(text_input, ev)
    time.sleep(0.5)
    assert 'uaasVinsUrl' not in text_input.event.payload
    assert 'uaasVinsUrl' not in text_input.payload_with_session_data

    text_input.payload_with_session_data['request']['experiments'] = safe_experiments_vins_format(text_input.payload_with_session_data['request']['experiments'], logging.warn)

    VinsProcessor._try_url_experiments(text_input)
    assert 'uaasVinsUrl' not in text_input.event.payload
    assert 'uaasVinsUrl' in text_input.payload_with_session_data
    old_value = text_input.payload_with_session_data.get('uaasVinsUrl')
    VinsProcessor._try_use_vins_url(text_input, text_input.payload_with_session_data, 'UaasVinsUrl_aHR0cDovL21lZ2FtaW5kLXJjLmFsaWNlLnlhbmRleC5uZXQ=', force=True)

    assert 'uaasVinsUrl' not in text_input.event.payload
    assert 'uaasVinsUrl' in text_input.payload_with_session_data
    assert text_input.payload_with_session_data.get('uaasVinsUrl') != old_value
