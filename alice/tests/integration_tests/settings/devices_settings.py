import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments('bg_fresh_granet_prefix=alice.setting.')
@pytest.mark.experiments('hw_enable_tandem_setting')
@pytest.mark.experiments('hw_enable_smart_speaker_setting')
@pytest.mark.experiments('bg_fresh_granet_prefix=alice.apps_fixlist')
@pytest.mark.experiments('bg_disable_tandem_settings_shortcut')
@pytest.mark.experiments('bg_disable_yandex_devices_settings_shortcut')
@pytest.mark.experiments('mm_enable_protocol_scenario=Settings')
@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.version(hollywood=194)
class TestDevicesSettings(object):

    owners = ('yagafarov',)

    @pytest.mark.parametrize('command, answer, surface', [
        ('настрой колонку', 'открываю', surface.searchapp),
        ('настрой колонку', 'Приступаю к созданию тандема между вашей Станцией и телевизором', surface.smart_tv),
        ('настрой тандем', 'отправила вам ссылку в приложение', surface.station),
        ('настрой тандем', 'открываю настройки тандема', surface.searchapp),
        ('настрой тандем', 'открываю настройки тандема', surface.smart_tv),
        ('настрой тандем', 'отправила вам ссылку в приложение', surface.legatus),
    ])
    def test_devices_settings_output_speech(self, alice, command, answer):
        r = alice(command)
        assert answer.lower() in r.output_speech_text.lower()
        assert r.scenario == scenario.Settings

    @pytest.mark.parametrize('surface', [surface.station])
    def test_send_push(self, alice):
        r = alice('настрой тандем')
        found_send_push_directive = False
        for directive in r.voice_response['directives']:
            if directive['name'] == 'send_push_directive':
                found_send_push_directive = True
        assert found_send_push_directive
