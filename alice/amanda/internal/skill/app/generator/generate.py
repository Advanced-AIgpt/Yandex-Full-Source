import os
import sys
from alice.acceptance.modules.request_generator.lib.app_presets import APP_PRESET_CONFIG


APP_MAPPING = {
    'AppID': 'app_id',
    'AppVersion': 'app_version',
    'OSVersion': 'os_version',
    'Platform': 'platform',
    'DeviceID': 'device_id',
    'DeviceModel': 'device_model',
    'DeviceManufacturer': 'device_manufacturer',
}


def main():
    with open(os.path.realpath(os.path.join(os.path.dirname(sys.argv[0]), "../presets.go")), 'w') as out:
        out.write(
            """package app

// see: https://a.yandex-team.ru/arc/trunk/arcadia/alice/acceptance/modules/request_generator/lib/app_presets.py

var (
presets = map[string]appPreset{
"""
        )
        for name, preset in APP_PRESET_CONFIG.items():
            out.write('"%s": {\n' % name)
            for go, python in APP_MAPPING.items():
                out.write('{}: "{}",\n'.format(go, preset.application.get(python, '')))
            for key, value in {
                'AuthToken': preset.auth_token,
                'ASRTopic': preset.asr_topic,
                'UserAgent': preset.user_agent,
            }.items():
                out.write('{}: "{}",\n'.format(key, value or ''))
            out.write('Features: []string{')

            all_features = (preset.supported_features if preset.supported_features else []) + list(
                map(lambda x: '!' + x, (preset.unsupported_features if preset.unsupported_features else []))
            )

            for feature in all_features:
                out.write('\n"{}",'.format(feature))

            out.write('\n},\n},\n')

        out.write('}\n)\n')


if __name__ == '__main__':
    main()
