import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
class TestPalmMusicDiscovery(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-589
    https://testpalm.yandex-team.ru/testcase/alice-667
    '''

    owners = ('vitvlkv',)

    @pytest.mark.parametrize('surface', [
        surface.station,
        surface.station_pro,
    ])
    def test_discovery_and_home(self, alice):
        response = alice('давай послушаем музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice('домой')
        assert response.intent == intent.ProtocolGoHome
        assert response.directive.name == directives.names.MordoviaShowDirective

        response = alice('порекомендуй мне музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
    def test_discovery_and_stop(self, alice):
        response = alice('давай послушаем музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice('стоп')
        assert response.intent == intent.PlayerPause
        assert response.directives[-1].name == directives.names.ClearQueueDirective

        response = alice('порекомендуй мне музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'
