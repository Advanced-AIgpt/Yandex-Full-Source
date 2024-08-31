import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.yabro_win])
class TestPalmYaBroWatchTv(object):
    """
        https://testpalm2.yandex-team.ru/testcase/alice-2174
    """

    owners = ('akormushkin', )

    @pytest.mark.parametrize('command', [
        'Смотреть ТВ',
        'Включи врямой эфир',
    ])
    def test_alice_2174(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvStream
        assert not response.directive
        assert response.text in [
            'Смотрите прямо сейчас.',
            'Вот что можно посмотреть. Выбирайте.',
        ]
        assert response.div_card
