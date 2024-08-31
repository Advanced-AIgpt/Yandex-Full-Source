import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexStaff)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestZeroTesting(object):

    owners = ('vitvlkv',)

    def test_activate(self, alice):
        response = alice('Активируй эксперимент 123')
        assert response.scenario == scenario.ZeroTesting
        assert response.text == 'Активировано залипание в эксперименте 123. Для завершения скажите "выключи эксперимент".'

    def test_activate_two_exps(self, alice):
        response = alice('Активируй эксперимент 123')
        assert response.scenario == scenario.ZeroTesting
        assert response.text == 'Активировано залипание в эксперименте 123. Для завершения скажите "выключи эксперимент".'

        response = alice('Активируй эксперимент 456')
        assert response.scenario == scenario.ZeroTesting
        assert response.text == 'Активировано залипание в 2 экспериментах 123, 456. Для завершения скажите "выключи эксперимент".'

    @pytest.mark.oauth(auth.Yandex)
    def test_activate_not_staff(self, alice):
        response = alice('Активируй эксперимент 123')
        assert response.scenario != scenario.ZeroTesting

    def test_deactivate(self, alice):
        response = alice('Выключи эксперимент')
        assert response.scenario == scenario.ZeroTesting
        assert response.text == 'Залипание в эксперименте выключено.'

    def test_session(self, alice):
        # 404902 is a real experiment https://ab.yandex-team.ru/testid/404902
        response = alice('Активируй эксперимент 404902')
        assert response.scenario == scenario.ZeroTesting
        assert response.text == 'Активировано залипание в эксперименте 404902. Для завершения скажите "выключи эксперимент".'

        response = alice('Скажи код эксперимента')
        assert response.scenario == scenario.ZeroTesting
        assert response.text == 'Текущий код Foo123, эксперимент 404902.'

        response = alice('Выключи эксперимент')
        assert response.scenario == scenario.ZeroTesting
        assert response.text == 'Залипание в эксперименте выключено.'

        response = alice('Скажи код эксперимента')
        assert response.scenario == scenario.ZeroTesting
        assert response.text == 'Код не найден, эксперименты не найдены.'
