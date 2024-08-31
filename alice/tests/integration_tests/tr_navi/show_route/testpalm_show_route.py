import re
from typing import Optional

import alice.tests.library.directives as directives
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.region(region.Istanbul)
@pytest.mark.parametrize('surface', [surface.navi_tr])
@pytest.mark.experiments('bg_fresh_granet_form=alice.navi.show_route')
class TestPalmShowRouteTr(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2092
    https://testpalm.yandex-team.ru/testcase/alice-2098
    https://testpalm.yandex-team.ru/testcase/alice-2117
    """

    owners = ('flimsywhimsy', )

    route_build_success_regex = r'(Tamam\.|Yola çıkalım\!|Peki\.|Yaşasın\!)'

    route_confirmed_commands = [
        'Gidelim',  # Поехали
        'Tamam',  # Хорошо
        'Olur',  # Ладно
    ]

    route_declined_commands = [
        'İptal',  # Отменить
        'Vazgeçtim',  # Я передумал
        'Yok',  # Нет
        'İstemiyorum',  # Не хочу
    ]

    route_confirmation_undefined_commands = [
        'Ne var ki',  # Что такое
        'Anlamadım',  # Я не понял
        'Sen kimsin',  # Ты кто
    ]

    remember_named_location_text = {
        'Bu konum ne yazık ki kayıtlı değil. Eklemek için Yerlerim sekmesini tıklayarak Kayıtlı Yerler’in altına ev ve iş adreslerini yazman yeterli.',
        'Söylediğin konum maalesef kayıtlı değil. Eklemek için Yerlerim sekmesini tıklayarak Kayıtlı Yerler’in altına ev ve iş adreslerini yazman yeterli.',
        'Maalesef söylediğin konumun nerede olduğunu şimdilik bilmiyorum. Yerlerim sekmesine girerek Kayıtlı Yerler’in altına ev ve iş adreslerini ekleyebilirsin.',
        'Gitmek istediğin konum maalesef kayıtlı değil. Yerlerim sekmesini tıklayıp Kayıtlı Yerler’in altına ev ve iş adreslerini ekleyebilirsin.',
    }

    route_work_regex = r'(İş yerine rota oluşturuyorum|İşe gidiyoruz)'

    route_home_regex = r'Eve gidiyoruz'

    @pytest.mark.parametrize('confirmation_command, confirmed', [
        (command, confirmed)
        for (confirmation_commands, confirmed) in [
            (route_declined_commands, False),
            (route_confirmed_commands, True),
            (route_confirmation_undefined_commands, None),
        ]
        for command in confirmation_commands
    ])
    def test_alice_2092(self, alice, confirmation_command, confirmed):
        response = alice('Geniş sokak, 1 taksim 1’e gidelim')  # Поехали на улицу Гениш, 1/1
        self._assert_route_build(
            response,
            r'.*?Geniş Sok\., No\:1[\w\d, ]*, İstanbul, Türkiye konumuna gidiyoruz',
        )

        response = alice(confirmation_command)
        self._assert_route_confirmation(response, confirmed)

    def test_alice_2092_delayed_confirmation(self, alice):
        response = alice('Meşrutiyet caddesi 51’e gitmek istiyorum')  # Хочу поехать на проспект Мешрутиет, 51
        self._assert_route_build(
            response,
            r'.*?Meşrutiyet Cad\., No\:51[\w\d, ]*, İstanbul, Türkiye konumuna gidiyoruz',
        )

        response = alice(self.route_confirmation_undefined_commands[0])
        self._assert_route_confirmation(response, None)

        response = alice(self.route_confirmation_undefined_commands[1])
        self._assert_route_confirmation(response, None)

        response = alice(self.route_confirmed_commands[0])
        self._assert_route_confirmation(response, True)

    @pytest.mark.parametrize('command, target', [
        (
            'Adıyaman’a gidelim',  # Поехали в Адыяман
            r'Adıyaman, Türkiye konumuna gidiyoruz',
        ),
        (
            'Migros süpermarketine nasıl gidilir?',  # Как проехать к супермаркету Мигрос?
            r'.*?Migros.*?, İstanbul adresine konumuna gidiyoruz',
        ),
        (
            'Benzin istasyonuna gidelim',  # Поехали на заправку
            r'.*?, İstanbul, Türkiye adresine konumuna gidiyoruz',
        ),
        (
            'Bağdat caddesi numara 12ye gidelim',  # Поехали на проспект Багдат, номер 12
            r'Bağdat Cad\., No\:12, Kadıköy, İstanbul, Türkiye konumuna gidiyoruz',
        ),
    ])
    def test_alice_2098(self, alice, command, target):
        response = alice(command)
        self._assert_route_build(response, target)

    @pytest.mark.parametrize('command_home, command_work', [
        (
            'Alisa eve',  # Алиса домой
            'Alisa işe gidelim',  # Алиса поехали на работу
        ),
        (
            'Haydi artık eve götür',  # Давай уже домой
            'Haydi iş yerime götür',  # Давай вези на работу
        ),
        (
            'Alisa haydi eve',  # Алиса Давай домой
            'Alisa haydi işe',  # Алиса давай на работу
        ),
        (
            'Alisa beni eve götür',  # Алиса отвези меня домой
            'Alisa beni işe götür',  # Алиса отвези меня на работу
        ),
        (
            'Eve kadar güzergahı belirle',  # Построй маршрут до дома
            'İş yerime kadar güzergahı belirle',  # Построй маршрут до работы
        ),
        (
            'Eve gidelim',  # Поехали домой
            'İşe gidelim',  # Поехали на работу
        ),
    ])
    @pytest.mark.device_state(navigator={
        "home": {
            "lat": 41.089437,
            "lon": 28.919446,
        },
        "work": {
            "lat": 41.061117,
            "lon": 28.952127,
        }
    })
    def test_alice_2117(self, alice, command_home, command_work):
        response = alice(command_home)
        self._assert_route_build(response, self.route_home_regex)

        response = alice(self.route_declined_commands[0])
        self._assert_route_confirmation(response, False)

        response = alice(command_work)
        self._assert_route_build(response, self.route_work_regex)

        response = alice(self.route_confirmed_commands[0])
        self._assert_route_confirmation(response, True)

    @pytest.mark.parametrize('command', [
        'Eve',  # Домой
        'İşe gidelim',  # Поехали на работу
        'İşe gitmek istiyorum',  # Хочу поехать на работу
    ])
    def test_alice_2117_no_saved_locations(self, alice, command):
        response = alice(command)
        assert response.text in self.remember_named_location_text
        assert not response.directives

    def _assert_route_build(self, response, adress_regex: str):
        assert response.scenario == scenario.ShowRouteTr
        assert re.match(rf'{self.route_build_success_regex} {adress_regex}\.', response.text)
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('yandexnavi://build_route_on_map?')

    def _assert_route_confirmation(self, response, confirmed: Optional[bool]):
        assert response.scenario == scenario.NaviExternalConfirmationTr
        assert response.text == {
            True: 'Yola çıkalım!',  # Поехали!
            False: 'Rota iptal edildi',  # Маршрут отменен
            None: 'Rotayı onaylıyor musun?'  # Вы подтверждаете маршрут?
        }[confirmed]
        if confirmed is not None:
            assert response.directive.name == directives.names.OpenUriDirective
            assert response.directive.payload.uri == f'yandexnavi://external_confirmation?confirmed={int(confirmed)}'
        else:
            assert not response.directives
