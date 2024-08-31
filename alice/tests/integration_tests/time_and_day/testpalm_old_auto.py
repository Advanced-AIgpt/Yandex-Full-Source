import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.old_automotive])
class TestShowWatch(object):
    """
    Частично https://testpalm.yandex-team.ru/testcase/alice-1800
    """

    owners = ('mihajlova',)

    def test_alice_1800(self, alice):
        response = alice('Покажи часы')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.OpenSiteOrApp
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.intent == 'launch'
        assert response.directive.payload.params.widget == 'clock'

        assert response.text == 'Открываю'
        assert response.text == response.output_speech_text
