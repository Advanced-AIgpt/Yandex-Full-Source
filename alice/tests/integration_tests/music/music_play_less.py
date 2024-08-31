import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


# TODO(a-square): extend to all surfaces
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.smart_speakers + [surface.searchapp])
class TestMusicPlayLess(object):
    owners = ('vitvlkv', 'zhigan', 'sparkle')

    @pytest.mark.parametrize('command', [
        'меньше рэпа',
        'меньше сексуальной музыки',
        'меньше музыки 2000 х',
    ])
    def test_music_play_less(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert not response.directive
        assert response.text in [
            'Упс. Я так не умею, но можете попросить меня включить музыку по настроению или жанру.',
            'Извините, но я не могу включить то, что вы просите. ' +
                'Зато умею включать музыку по жанрам: рок, джаз, рэп, электроника.',
            'Простите, я бы с радостью, но так не умею. Давайте послушаем что-нибудь ещё.',
        ]
