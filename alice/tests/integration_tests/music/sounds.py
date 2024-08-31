import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
class TestSounds(object):

    owners = ('igor-darov',)

    @pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
    @pytest.mark.parametrize('command', [
        'включи звуки барабана',
        'алиса включи звуки пердежа',
        'включи звук пианино',
        'включи лай собак'
    ])
    def test_has_answer(self, alice, command):
        response = alice(command)
        assert response.intent in [intent.MusicPlay, intent.MusicAmbientSound]
        assert response.text
