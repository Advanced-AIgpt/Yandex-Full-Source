# coding: utf-8

import pytest

from hamcrest import assert_that, has_entry, has_item, has_items, not_none, all_of, contains_inanyorder

from alice.acceptance.modules.request_generator.lib import app_presets
from alice.acceptance.modules.request_generator.lib import vins


@pytest.mark.parametrize('app_preset', [k if k != 'yandexmidi' and k != 'legatus' else pytest.param(k, marks=pytest.mark.skip)
                                        for k in app_presets.APP_PRESET_CONFIG.keys()])
def test_old_presets_hasnot_environment_state(app_preset):
    row = dict(
        app=app_presets.get_preset_attr(app_preset, 'application'),
        app_preset=app_preset
    )
    assert 'environment_state' not in vins.get_vins_request(row)['request']['request']
    assert app_presets.get_preset_attr(row.get('app_preset'), 'capabilities') is None


def test_yandexmidi_has_environment_state():
    app_preset = 'yandexmidi'
    app=app_presets.get_preset_attr(app_preset, 'application')
    row = dict(
        app=app,
        app_preset=app_preset
    )
    assert_that(app_presets.get_preset_attr(row.get('app_preset'), 'capabilities'), not_none())

    vins_request = vins.get_vins_request(row)['request']['request']
    assert_that(vins_request, has_entry('environment_state',
                                        has_entry('endpoints',
                                                   has_item(all_of(
                                                            has_entry('id', app['device_id']),
                                                            has_entry('capabilities', has_item(all_of(
                                                                                               has_entry('parameters', has_entry("supported_protocols", contains_inanyorder("Zigbee", "WiFi"))),
                                                                                               has_entry('@type', "type.googleapis.com/NAlice.TIotDiscoveryCapability")))))))))


def test_legatus_has_environment_state():
    app_preset = 'legatus'
    app=app_presets.get_preset_attr(app_preset, 'application')
    row = dict(
        app=app,
        app_preset=app_preset
    )
    assert_that(app_presets.get_preset_attr(row.get('app_preset'), 'capabilities'), not_none())

    vins_request = vins.get_vins_request(row)['request']['request']
    assert_that(vins_request, has_entry('environment_state',
                                        has_entry('endpoints',
                                                  has_item(all_of(
                                                      has_entry('id', app['device_id']),
                                                      has_entry('capabilities', has_item(all_of(
                                                          has_entry('parameters', has_entry("available_apps", has_items(has_entry("name", "kinopoisk"), has_entry("name", "youtube")))),
                                                          has_entry('@type', "type.googleapis.com/NAlice.TWebOSCapability")))))))))
