import re
import json

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmPoi(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-11
    https://testpalm.yandex-team.ru/testcase/alice-26
    https://testpalm.yandex-team.ru/testcase/alice-29
    https://testpalm.yandex-team.ru/testcase/alice-40
    https://testpalm.yandex-team.ru/testcase/alice-655
    """

    owners = ('zhigan')
    prohibition_error_phrases = [
        'В часах такое провернуть сложновато.',
        'Я бы и рада, но здесь не могу. Эх.',
        'Здесь точно не получится.',
    ]

    def _assert_prohibition(self, response):
        assert response.intent == intent.ProhibitionError
        assert response.text in self.prohibition_error_phrases
        assert not response.directive

    def test_alice_11(self, alice):
        response = alice('Где пообедать?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi

        response = alice('Покажи на карте')
        self._assert_prohibition(response)

        response = alice('Где купить продукты?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi

        response = alice('Где пообедать?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi

        response = alice('Подробнее')
        assert response.intent == intent.FindPoiDetails
        assert response.text in [
            'Подробнее в двух словах не расскажешь. Попробуйте спросить в Яндексе.',
            'Я справлюсь с этим лучше на мобильном.',
            'Спросите меня об этом на телефоне.',
            'Попросите меня об этом на телефоне.',
            'Спросите меня на телефоне, там как-то привычнее.',
        ]
        assert not response.directive

        response = alice('Где пообедать?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi

        response = alice('Открой сайт')
        self._assert_prohibition(response)

        response = alice('Где пообедать?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi
        company_name = json.loads(response.slots['last_found_poi'].string)['company_name']
        received_company_names = [company_name]

        response = alice('А ещё?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoiScrollNext
        company_name = json.loads(response.slots['last_found_poi'].string)['company_name']
        assert company_name not in received_company_names
        received_company_names.append(company_name)

        response = alice('А ещё?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoiScrollNext
        company_name = json.loads(response.slots['last_found_poi'].string)['company_name']
        assert company_name not in received_company_names

    def test_alice_29(self, alice):
        response = alice('Как доехать до ленинского проспекта 22')
        assert response.intent == intent.ShowRoute
        assert re.search(
            r'^(Путь|Маршрут|Дорога) займет .+ на машине или .+ на транспорте. Это путь до адреса Ленинский проспект 22.$',
            response.text
        )

        response = alice('А на машине?')
        assert response.intent == intent.ShowRouteEllipsis
        assert re.search(
            r'^.+ с учетом пробок. Это путь до адреса Ленинский проспект 22.$',
            response.text
        )

        response = alice('Покажи на карте')
        self._assert_prohibition(response)

    def test_alice_40(self, alice):
        response = alice('Где купить секс-игрушки?')
        assert response.scenario == scenario.Search
        assert any(phrase in response.text for phrase in [
            'я ничего не нашла',
            'ничего не нашлось',
            'не получилось ничего найти',
            'ничего найти не получилось',
        ])

    @pytest.mark.parametrize('command', [
        'Ломоносовский проспект',
        'Ленинские горы дом 1',
    ])
    def test_alice_655(self, alice, command):
        response = alice(command)
        assert response.intent == intent.FindPoi
        assert response.text in [
            'Без карты я не справлюсь. Увы.',
            'Для этого мне нужны Яндекс.Карты, а здесь их нет.',
            'Я бы и рада помочь, но без карты никак.',
        ]

    @pytest.mark.parametrize('command', [
        'Поехали домой',
        'Поехали на работу',
    ])
    def test_alice_26(self, alice, command):
        response = alice(command)
        assert response.intent == intent.RememberNamedLocation
        assert response.text in [
            'И где находится дом?',
            'Окей, где находится дом?',
            'Окей, какой адрес у дома?',
            'И какой адрес у дома?',
            'И где находится работа?',
            'Окей, где находится работа?',
            'Окей, какой адрес у работы?',
            'И какой адрес у работы?',
        ]

        response = alice('Санкт-Петербург, Невский проспект дом 10')
        assert response.intent == intent.RememberNamedLocationEllipsis

        response = alice('Да')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert re.search(
            r'^(Путь|Маршрут|Дорога) займет .+ на машине. Это путь до адреса Санкт-Петербург, Невский проспект 10.$',
            response.text
        )
