import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station, surface.searchapp])
class TestRadioOnboarding(object):

    owners = ('olegator',)

    @pytest.mark.parametrize('command', [
        'какие радиостанции ты знаешь',
        'какие радиостанции у тебя есть',
        'какое радио у тебя есть',
        'порекомендуй радиостанцию',
    ])
    def test_radio_onboarding(self, alice, command):
        response = alice(command)
        assert response.intent == intent.RadioPlayOnboarding

        response = alice('а еще')
        assert response.intent == intent.RadioPlayOnboardingNext

        response = alice('давай еще')
        assert response.intent == intent.RadioPlayOnboardingNext

        response = alice('другие')
        assert response.intent == intent.RadioPlayOnboardingNext
