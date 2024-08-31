import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestPalmSleepTimers(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1755
    """

    owners = ('alkapov',)

    def test_alice_1755(self, alice):
        alice('поставь таймер на 5 минут')
        alice('поставь таймер на 15 минут')
        alice('поставь таймер сна на 20 минут')

        response = alice('Какие у меня есть таймеры?')

        for timer in ['таймер на 5 минут', 'таймер на 15 минут', 'таймер сна на 20 минут']:
            assert timer in response.text
