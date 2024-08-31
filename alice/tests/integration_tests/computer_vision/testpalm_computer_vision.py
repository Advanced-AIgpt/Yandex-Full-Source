import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmComputerVision(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-18
    """

    owners = ('polushkin', 'g:cv-search',)

    def test_alice_18(self, alice):
        response = alice('Что на фото')
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.product_scenario == 'images_what_is_this'
        assert response.text in [
            'С этим в часах у меня пока туговато. Но я научусь, обещаю.',
            'Я обязательно научусь это делать. Ещё не до конца тут освоилась, такие дела.',
        ]
        assert not response.directive
