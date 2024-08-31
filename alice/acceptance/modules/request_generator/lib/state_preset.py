import attr


@attr.s
class StatePreset():
    device_state = attr.ib(default=attr.Factory(dict))
    environment_state = attr.ib(default=attr.Factory(dict))
    screenshot_url = attr.ib(default=attr.Factory(str))


DEFAULT_STATE_PRESET = StatePreset()
TEST_STATE_PRESET = StatePreset(
    device_state={
        'sound_max_level': 1234,
    },
    environment_state={
        'devices': {
            'supported_features': [
                'supports_any_feature'
            ],
        },
    },
    screenshot_url='http://div2html.s3.yandex.net/div12_2.html?url=http://div2html.s3.yandex.net/nirvana/e8a60aa4-e88f-3ab9-a7a2-1ebb831aeced.json',
)


# ====================== CONFIGS ======================
STATE_PRESETS_CONFIG = {
    'default': DEFAULT_STATE_PRESET,
    'test_for_test': TEST_STATE_PRESET,
}

APP_PRESET_TO_STATES = {
    'centaur': {
        'default',
        'test_for_test',
    },
}


def get_preset_attr(state_preset_key, app_preset_key, attr_name):
    states = APP_PRESET_TO_STATES.get(app_preset_key, {'default'})
    if state_preset_key not in states:
        raise ValueError(f'Error: not found state_preset {state_preset_key} for app_preset {app_preset_key}')

    state_preset_obj = STATE_PRESETS_CONFIG.get(state_preset_key)
    return getattr(state_preset_obj, attr_name) if state_preset_obj else None
