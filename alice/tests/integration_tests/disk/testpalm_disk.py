import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.yabro_win])
class TestPalmDisk(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2493
    """

    owners = ('sparkle',)

    command = 'Покажи мои фотографии'
    expected_answer = \
        'Не могу открыть ваши фото. Чтобы всё получилось, авторизуйтесь в Яндексе и загрузите фотографии на Яндекс Диск'

    @pytest.mark.no_oauth
    def test_alice_2493_no_oauth(self, alice):
        response = alice(self.command)
        assert response.scenario == scenario.DiskMyPhotos
        assert self.expected_answer in response.text

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/MEGAMIND-2034')
    @pytest.mark.oauth(auth.Yandex)
    def test_alice_2493(self, alice):
        response = alice(self.command)
        assert response.scenario == scenario.DiskMyPhotos
        assert self.expected_answer in response.text
