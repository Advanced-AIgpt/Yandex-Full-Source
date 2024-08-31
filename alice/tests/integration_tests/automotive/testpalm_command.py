import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.automotive])
class TestPalmIrrelCommand(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1390
    """

    owners = ('nkodosov')

    def test_alice_1390(self, alice):
        response = alice('Вызови такси')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TaxiNewDisabled
        assert not response.directive
        assert response.text == 'Здесь я не справлюсь. Лучше приложение Яндекс или Яндекс.Станция, давайте там.'

        response = alice('Позвони бабушке')
        assert response.scenario == scenario.MessengerCall
        assert response.intent is None
        assert not response.directive
        assert response.text in [
            'Я справлюсь с этим лучше на телефоне.',
            'Это я могу, но лучше с телефона.',
            'Для звонков телефон как-то удобнее, давайте попробую там.',
        ]

        response = alice('Открой Одноклассники')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.OpenSiteOrApp
        assert not response.directive
        assert response.text == 'К сожалению я не могу это открыть. Если вы сейчас не едете по дороге — можете сделать это вручную.'


@pytest.mark.parametrize('surface', [surface.old_automotive])
class TestPalmCommand(object):
    """
    Частично https://testpalm.yandex-team.ru/testcase/alice-1802
    """

    owners = ('nkodosov')

    def test_alice_1802(self, alice):
        response = alice('Открой навигатор')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.OpenSiteOrApp
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.application == 'car'
        assert response.directive.payload.intent == 'launch'
        assert response.directive.payload.params.app == 'navi'
        assert response.text == 'Открываю'
