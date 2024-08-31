import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


SCENARIO_NAME = 'SmartDeviceExternalApp'
SCENARIO_HANDLE = 'smart_device_external_app'

YOUTUBE_DIV_CARD_ID = 'youtube.webview'


@pytest.fixture(scope='module')
def enabled_scenarios():
    return [SCENARIO_HANDLE]


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.experiments(f'mm_enable_protocol_scenario={SCENARIO_NAME}', 'bg_fresh_granet')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestOpenExternalApp:
    @pytest.mark.device_state(packages={
        "installed": [
            {
                "main_activity": "dev.cobalt.app.MainActivity",
                "package_info": {
                    "name": "com.yandex.tv.ytplayer",
                    "human_readable_name": "YouTube"
                }
            }
        ]
    })
    def test_open_youtube(self, alice):
        r = alice(voice('открой приложение youtube'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech == "Открываю."
        response_directive = r.run_response.ResponseBody.Layout.Directives[0].SendAndroidAppIntentDirective
        assert response_directive.Action == "android.intent.action.MAIN"
        assert response_directive.Category == "android.intent.category.LAUNCHER"
        assert response_directive.Component.Pkg == "com.yandex.tv.ytplayer"
        assert response_directive.Component.Cls == "dev.cobalt.app.MainActivity"

        return str(r)

    @pytest.mark.device_state(packages={
        "installed": [
            {
                "main_activity": "dev.cobalt.app.MainActivity",
                "package_info": {
                    "name": "com.yandex.tv.ytplayer",
                    "human_readable_name": "YouTube"
                }
            }
        ]
    })
    @pytest.mark.experiments('bg_fresh_granet_form=alice.open_smart_device_exact_external_app')
    def test_open_youtube_exact(self, alice):
        r = alice(voice('ютуб'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech == "Открываю."
        response_directive = r.run_response.ResponseBody.Layout.Directives[0].SendAndroidAppIntentDirective
        assert response_directive.Action == "android.intent.action.MAIN"
        assert response_directive.Category == "android.intent.category.LAUNCHER"
        assert response_directive.Component.Pkg == "com.yandex.tv.ytplayer"
        assert response_directive.Component.Cls == "dev.cobalt.app.MainActivity"

        return str(r)

    @pytest.mark.device_state(packages={"installed": []})
    def test_open_store_youtube(self, alice):
        r = alice(voice('открой приложение youtube'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech == "Необходимо сначала установить приложение."
        response_directive = r.run_response.ResponseBody.Layout.Directives[0].SendAndroidAppIntentDirective
        assert response_directive.Action == "android.intent.action.VIEW"
        assert response_directive.Uri == "home-app://market_item?package=com.yandex.tv.ytplayer"

        return str(r)

    @pytest.mark.supported_features(tv_open_store=None)
    @pytest.mark.device_state(packages={"installed": []})
    def test_app_not_recognized_store_not_available(self, alice):
        r = alice(voice('открой приложение youtube'))

        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech == "Кажется, такое приложение не установлено."

        return str(r)

    def test_try_open_unlisted_app(self, alice):
        r = alice(voice('открой приложение twitch'))

        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        assert r.run_response.ResponseBody.Layout.OutputSpeech == "Упс, это приложение пока недоступно."

        return str(r)


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestShowView:
    def test_show_view_youtube(self, alice):
        r = alice(voice('открой youtube'))
        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('ShowViewDirective')
        assert directives[0].ShowViewDirective.ActionSpaceId == YOUTUBE_DIV_CARD_ID

        action_spaces = r.run_response.ResponseBody.ActionSpaces
        assert len(action_spaces) == 1
        actions = action_spaces[YOUTUBE_DIV_CARD_ID].Actions
        assert len(actions) == 2
