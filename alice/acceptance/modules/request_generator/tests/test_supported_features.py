# coding: utf-8

import json

import pytest
import yatest.common as yc
from alice.acceptance.modules.request_generator.lib import app_presets


@pytest.fixture
def app_preset(app_preset_name):
    return app_presets.APP_PRESET_CONFIG[app_preset_name]


@pytest.fixture
def platform_config(config_path):
    with open(yc.build_path(f'smart_devices/platforms/{config_path}')) as f:
        return json.loads(f.read())


@pytest.mark.parametrize('app_preset_name, config_path', [
    ('quasar', 'yandexstation/config/quasar-prod.cfg'),
    ('yandexmax', 'yandexstation_2/config/quasar-prod.cfg'),
    ('yandexmini', 'yandexmini/config/quasar.cfg'),
    ('yandexmicro_beige', 'yandexmicro/config/quasar.cfg'),
    ('yandexmicro_red', 'yandexmicro/config/quasar.cfg'),
    ('yandexmicro_pink', 'yandexmicro/config/quasar.cfg'),
    ('yandexmicro_purple', 'yandexmicro/config/quasar.cfg'),
    ('yandexmicro_yellow', 'yandexmicro/config/quasar.cfg'),
    ('yandexmicro_green', 'yandexmicro/config/quasar.cfg'),
])
def test_supported_features(app_preset, platform_config):
    assert set(app_preset.supported_features) == set(platform_config['aliced']['supportedFeatures'])
