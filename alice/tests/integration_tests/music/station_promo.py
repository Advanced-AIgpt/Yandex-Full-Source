import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments('station_promo_score=1')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestStationPromo(object):

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_play_music_with_plus(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.text == 'Включаю. Хотя слушать музыку удобнее на Станции - и вы можете прямо сейчас получить ее, оформив специальную подписку.'

    @pytest.mark.oauth(auth.Yandex)
    def test_play_music_without_plus(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.text == ('Хорошо, включу для вас небольшой отрывок, потому что без подписки иначе не получится. '
                                 'Кстати, вы можете оформить подписку сейчас и получить вместе с ней Станцию. На ней слушать музыку гораздо удобнее.')
