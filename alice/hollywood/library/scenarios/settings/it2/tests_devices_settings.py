import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['settings']


EXPECTED_OPEN_TANDEM_SETTING_TEXT = 'открываю настройки тандема.'
EXPECTED_OPEN_TANDEM_SETTING_SPEAKER_TEXT = 'Отлично! Приступаю к созданию тандема между вашей Станцией и телевизором, чтобы вы могли запускать видео голосом без пульта. Открываю настройки'.lower()


def _check_tandem_android_intent_directive(r):
    for directive in r.run_response.ResponseBody.Layout.Directives:
        android_directive = directive.SendAndroidAppIntentDirective
        if android_directive:
            assert 'TANDEM_SETUP' in android_directive.Action
            assert android_directive.Flags.FlagActivityNewTask
            return
    assert False, 'SendAndroidAppIntentDirective not found in response'


def _check_open_uri_directive(r, link_part: str):
    for directive in r.run_response.ResponseBody.Layout.Directives:
        open_uri_directive = directive.OpenUriDirective
        if open_uri_directive:
            assert link_part in open_uri_directive.Uri
            return
    assert False, 'OpenUriDirective not found in response'


def _check_analytics_action(r, action_id):
    for action in r.run_response.ResponseBody.AnalyticsInfo.Actions:
        if action.Id == action_id:
            return
    assert False, f'Action {action_id} not found in response'


def _check_tandem_push_directive(r):
    for directive in r.run_response.ResponseBody.ServerDirectives:
        send_push_directive = directive.SendPushDirective
        if send_push_directive:
            assert send_push_directive.PushId == 'open_site_or_app'
            assert send_push_directive.PushTag == 'open_site_or_app'
            assert 'yellowskin' in send_push_directive.Settings.Link
            assert 'тандем' in send_push_directive.Settings.Title.lower()
            assert 'тандем' in send_push_directive.Settings.Text.lower()
            return
    assert False, 'SendPushDirective not found in response'


def _check_voice_and_text(r, expected_text):
    assert r.run_response.ResponseBody.Layout.OutputSpeech.lower() == expected_text
    assert r.run_response.ResponseBody.Layout.Cards[0].Text.lower() == expected_text


def _check_voice_and_text_with_buttons(r, expected_voice, expected_text=None, button_title='открыть'):
    if expected_text is None:  # there is no overload in python :(
        expected_text = expected_voice
    assert r.run_response.ResponseBody.Layout.OutputSpeech.lower() == expected_voice
    assert r.run_response.ResponseBody.Layout.Cards[0].TextWithButtons.Text.lower() == expected_text
    assert r.run_response.ResponseBody.Layout.Cards[0].TextWithButtons.Buttons[0].Title.lower() == button_title


@pytest.mark.scenario(name='Settings', handle='settings')
@pytest.mark.experiments(
    'hw_music_announce',
    'bg_fresh_granet_prefix=alice.music.announce.',
    'bg_fresh_granet_prefix=alice.setting.',
    'hw_enable_smart_speaker_setting',
    'hw_enable_tandem_setting',
)
class TestBase:
    pass


class TestDevicesSettingsNoAuth(TestBase):

    @pytest.mark.parametrize('surface', [surface.smart_tv])
    def test_open_tandem_setting_screen(self, alice):
        r = alice(voice('настрой тандем'))
        _check_tandem_android_intent_directive(r)
        _check_voice_and_text(r, EXPECTED_OPEN_TANDEM_SETTING_TEXT)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_open_tandem_setting_story(self, alice):
        r = alice(voice('настрой тандем'))
        _check_voice_and_text_with_buttons(
            r, EXPECTED_OPEN_TANDEM_SETTING_TEXT)
        _check_open_uri_directive(r, 'yellowskin')

    @pytest.mark.parametrize('surface', [surface.navi, surface.station])
    def test_dont_send_push_tandem_setting(self, alice):
        r = alice(voice('настрой тандем'))
        assert 'авториз' in r.run_response.ResponseBody.Layout.OutputSpeech
        assert not r.run_response.ResponseBody.ServerDirectives

    @pytest.mark.xfail(reason="under flag 'use_tandem_in_tv'")
    @pytest.mark.parametrize('surface', [surface.smart_tv])
    def test_open_tandem_smart_speaker_setting_tv(self, alice):
        r = alice(voice('настрой колонку'))
        _check_tandem_android_intent_directive(r)
        _check_voice_and_text(
            r, EXPECTED_OPEN_TANDEM_SETTING_SPEAKER_TEXT)

    @pytest.mark.experiments('use_tandem_in_tv')
    @pytest.mark.parametrize('surface', [surface.smart_tv])
    def test_open_tandem_smart_speaker_setting_tv_with_flag(self, alice):
        r = alice(voice('настрой колонку'))
        _check_tandem_android_intent_directive(r)
        _check_voice_and_text(
            r, EXPECTED_OPEN_TANDEM_SETTING_SPEAKER_TEXT)

    @pytest.mark.parametrize('surface', [surface.module_2])
    def test_open_tandem_smart_speaker_setting_module(self, alice):
        r = alice(voice('настрой колонку'))
        _check_tandem_android_intent_directive(r)
        _check_voice_and_text(
            r, EXPECTED_OPEN_TANDEM_SETTING_SPEAKER_TEXT)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_open_smart_speaker_setting(self, alice):
        r = alice(voice('настрой колонку'))
        _check_open_uri_directive(r, 'opensettings')
        _check_voice_and_text_with_buttons(
            r, expected_voice='открываю.', expected_text='настройка умных устройств яндекса.')

    @pytest.mark.parametrize('surface', [surface.navi, surface.station])
    def test_open_smart_speaker_setting_unsupported_surfaces(self, alice):
        r = alice(voice('настрой колонку'))
        assert 'не могу открыть страницу настройки' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert not r.run_response.ResponseBody.ServerDirectives
        assert not r.run_response.ResponseBody.Layout.Directives


@pytest.mark.oauth(auth.Yandex)
class TestDevicesSettingsAuth(TestBase):

    @pytest.mark.parametrize('surface', [surface.navi, surface.station])
    def test_send_push_tandem_setting(self, alice):
        r = alice(voice('настрой тандем'))
        assert 'отправила вам ссылку в приложение' in r.run_response.ResponseBody.Layout.OutputSpeech
        _check_tandem_push_directive(r)
        _check_analytics_action(r, action_id='send_tandem_settings')
