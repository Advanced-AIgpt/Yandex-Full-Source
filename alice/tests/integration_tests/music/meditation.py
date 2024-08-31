import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestMeditation(object):

    owners = ('olegator', 'mike88', )

    def test_meditation(self, alice):
        response = alice('запусти медитацию')
        assert response.scenario == scenario.HollywoodHardcodedMusic
        assert response.intent == intent.Meditation
        assert response.directive.name == directives.names.MusicPlayDirective
