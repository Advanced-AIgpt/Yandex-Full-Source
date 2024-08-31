# coding: utf-8

import pytest

from alice.acceptance.modules.request_generator.lib import vins


def test_state_preset():
    row = {
        'app_preset': 'centaur',
        'state_preset': 'test_for_test',
    }
    req = vins.get_vins_request(row)['request']['request']
    assert req['device_state'] == {'sound_max_level': 1234}
    assert req['environment_state'] == {'devices': {'supported_features': ['supports_any_feature']}}


def test_state_preset_invalid():
    row = {
        'app_preset': 'quasar',
        'state_preset': 'test_for_test',
    }
    with pytest.raises(ValueError, match='Error: not found state_preset test_for_test for app_preset quasar'):
        vins.get_vins_request(row)


def test_state_default():
    row = {
        'app_preset': 'quasar',
        'state_preset': 'default',
    }
    req = vins.get_vins_request(row)['request']['request']
    assert req['device_state'] == {}
    assert req['environment_state'] == {}


def test_state_no_patch():
    row = {
        'app_preset': 'legatus',
        'state_preset': 'default',
        'device_state': {
            'sound_max_level': 1234
        },
    }
    req = vins.get_vins_request(row)['request']['request']
    assert not req['device_state'] == {}
    assert not req['environment_state'] == {}
