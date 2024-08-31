import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestPlayVsDescription(object):

    owners = ('akormushkin',)

    @pytest.mark.experiments('video_webview_video_entity')
    @pytest.mark.parametrize('command', ['покажи описание', 'открой описание'])
    @pytest.mark.parametrize('film', ['фильм хатико', 'доктор хаус'])
    def test_description_webview(self, alice, command, film):
        response = alice(f'{command} {film}')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert 'videoEntity' in response.directive.payload.url

    @pytest.mark.experiments('video_disable_webview_video_entity')
    @pytest.mark.parametrize('command', ['покажи описание', 'открой описание'])
    @pytest.mark.parametrize('film', ['фильм хатико', 'доктор хаус'])
    def test_description_native(self, alice, command, film):
        response = alice(f'{command} {film}')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective

    @pytest.mark.parametrize('command', ['покажи', 'открой', 'включи'])
    @pytest.mark.parametrize('film', ['хатико', 'доктора хауса'])
    def test_play(self, alice, command, film):
        response = alice(f'{command} {film}')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.experiments('video_use_pure_hw')
class TestTvPlayVsDescription(object):

    owners = ('dandex',)

    @pytest.mark.parametrize('command', ['покажи', 'открой', 'включи'])
    @pytest.mark.parametrize('film', ['хатико', 'доктор хаус'])
    def test_play(self, alice, command, film):
        response = alice(f'{command} {film}')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
