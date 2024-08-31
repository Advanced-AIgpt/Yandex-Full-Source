import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.smart_tv,
    surface.station,
])
class TestFixlistPlayer(object):

    owners = ('ardulat', )

    @staticmethod
    def _assert_fixlist_response(response, text):
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == text

    def test_fixlist(self, alice):
        response = alice('включи фикслист для тестов это не должны спросить в проде кейс трек по запросу')
        self._assert_fixlist_response(response, 'Включаю трек по запросу: Дора, Не исправлюсь.')


@pytest.mark.oauth(auth.YandexPlus)
class TestFixlistDeeplink(object):

    owners = ('ardulat', )

    @staticmethod
    def _assert_fixlist_response(response):
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.OpenUriDirective

    @pytest.mark.parametrize('surface', [surface.launcher, surface.searchapp, surface.yabro_win, surface.navi])
    def test_fixlist(self, alice):
        response = alice('включи фикслист для тестов это не должны спросить в проде кейс трек по запросу')
        self._assert_fixlist_response(response)
        if surface.is_launcher(alice) or surface.is_yabro_win(alice):
            assert response.text == 'Включаю'
        elif not surface.is_searchapp(alice):
            assert response.text == 'Включаю трек по запросу: Дора, Не исправлюсь.'


@pytest.mark.oauth(auth.YandexPlus)
class TestFixlistUnsupported(object):

    owners = ('ardulat', )

    @staticmethod
    def _assert_fixlist_response(response):
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent in (intent.MusicPlay, intent.ProhibitionError)
        assert not response.directive

    @pytest.mark.parametrize('surface', [surface.automotive])
    def test_fixlist_auto(self, alice):
        response = alice('включи фикслист для тестов это не должны спросить в проде кейс трек по запросу')
        self._assert_fixlist_response(response)
        # TODO(ardulat): fix nlg to music_play_not_supported_on_device (to be fixed)
        assert response.text == 'Включаю трек по запросу: Дора, Не исправлюсь.'

    @pytest.mark.parametrize('surface', [surface.watch])
    def test_fixlist_watch(self, alice):
        response = alice('включи фикслист для тестов это не должны спросить в проде кейс трек по запросу')
        self._assert_fixlist_response(response)
        assert response.text in [
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        ]
