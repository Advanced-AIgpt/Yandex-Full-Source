import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.region(region.Moscow)
class TestPalmWhereAmI(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-14
    https://testpalm.yandex-team.ru/testcase/alice-617
    https://testpalm.yandex-team.ru/testcase/alice-1091
    https://testpalm.yandex-team.ru/testcase/alice-1364
    https://testpalm.yandex-team.ru/testcase/alice-1567
    https://testpalm.yandex-team.ru/testcase/alice-2163
    https://testpalm.yandex-team.ru/testcase/alice-2198
    """

    owners = ('nkodosov')
    address = 'Москва, улица Льва Толстого 16'
    nlg = [
        f'Я думаю, примерно тут: {address}.',
        f'По моим данным, вы находитесь по адресу {address}.',
        f'Если карта верна, вы по адресу {address}.',
    ]

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.maps,
        surface.navi,
    ])
    @pytest.mark.parametrize('command', [
        'Алиса, где я',
        'Адрес моего местоположения',
        'В каком я городе?',
        'Найди меня',
        'Где я сейчас нахожусь?',
    ])
    def test_alice_1364_1567(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.GetMyLocation
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://show_user_position'
        assert response.text in self.nlg

    @pytest.mark.parametrize('surface', surface.smart_speakers + [surface.smart_tv])
    @pytest.mark.parametrize('command', [
        'Алиса, где я',
        'Адрес моего местоположения',
        'В каком я городе?',
        'Где я сейчас нахожусь?',
    ])
    def test_alice_617(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.GetMyLocation
        assert not response.directive
        assert response.text in self.nlg

    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
        surface.yabro_win,
    ])
    @pytest.mark.parametrize('command', [
        'Где я',
        'Адрес моего местоположения',
        'В каком я городе?',
    ])
    def test_alice_1091(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.GetMyLocation
        assert not response.directive
        assert response.text in self.nlg

        protocol = 'https' if surface.is_yabro_win(alice) else 'intent'
        show_card = response.button('Показать на карте')
        assert show_card
        assert show_card.directives[0].name == directives.names.OpenUriDirective
        assert show_card.directives[0].payload.uri.startswith(f'{protocol}://yandex.ru/maps?')

    @pytest.mark.parametrize('surface', [surface.watch])
    def test_alice_14(self, alice):
        response = alice('Где я нахожусь?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.GetMyLocation
        assert not response.directive
        assert response.text in self.nlg

        response = alice('Покажи на карте')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ProhibitionError
        assert not response.directive
        assert response.text in [
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        ]
