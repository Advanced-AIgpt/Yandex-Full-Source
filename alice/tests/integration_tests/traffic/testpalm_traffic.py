import re

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


def _assert_traffic_level(response, geo):
    assert response.intent == intent.ShowTraffic
    assert re.search(
        r'^' + geo + r' в настоящее время [0-9]+ балл.+$',
        response.text
    )


def _assert_traffic_level_with_card(response, geo):
    assert response.intent == intent.ShowTraffic
    assert response.has_voice_response()
    assert re.search(
        r'^(' + geo + r'|Загруженность) — [0-9]+ балл.+$',
        response.output_speech_text
    )


def _assert_traffic_without_level(response, geo):
    assert response.intent == intent.ShowTraffic
    assert response.text == f'К сожалению, у меня нет информации о дорожной ситуации {geo}'


def _assert_traffic_without_level_with_card(response, geo):
    assert response.intent == intent.ShowTraffic
    assert response.has_voice_response()
    assert response.output_speech_text in [
        f'Вот пробки {geo} на карте.',
        f'Вот пробки {geo}. Внимание на карту.',
        f'Загруженность дорог {geo}. Вот как это выглядит.',
    ]


def _assert_traffic_without_level_with_card_not_support(response):
    assert response.intent == intent.ShowTraffic
    assert response.text in [
        'Я не могу показать загруженность дорог на этом устройстве.',
        'К сожалению, показать здесь карту пробок не получится.',
    ]


def _assert_traffic_nogeo_error(response, geo):
    assert response.intent == intent.ShowTraffic
    assert re.search(
        r'^(К сожалению,|Извините,|Простите,|Увы, но) (я не могу понять,|я не знаю,) где это "' + geo + r'".+$',
        response.text
    )


def _assert_has_map_and_text_show_map(cards):
    assert 'ПОСМОТРЕТЬ ПРОБКИ В КАРТАХ' in str(cards)


def _assert_traffic_on_map_suggest(response):
    traffic_on_map = response.suggest('Пробки на карте')
    assert traffic_on_map

    open_uri = [d for d in traffic_on_map.directives if d.name == directives.names.OpenUriDirective]
    re_expected_open_uri = [
        r'^yandexnavi\://show_point_on_map.+$',
        r'yandexnavi\://traffic\?traffic_on=1',
    ]
    assert len(open_uri) == len(re_expected_open_uri)
    for actual, expected in zip(open_uri, re_expected_open_uri):
        assert re.search(expected, actual.payload.uri)


@pytest.mark.voice
class _TestPalmVoiceTraffic(object):
    owners = ('ikorobtsev',)


class TestPalmTextTraffic(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-30
        https://testpalm.yandex-team.ru/testcase/alice-418
    """

    owners = ('ikorobtsev',)

    @pytest.mark.parametrize('surface', [surface.watch])
    def test_alice_30(self, alice):
        response = alice('Какие пробки в городе?')
        _assert_traffic_level(response, geo='В Москве')

        response = alice('Какие пробки в городе Урюпинске?')
        _assert_traffic_without_level(response, geo='в Урюпинске')

    @pytest.mark.parametrize('surface', [surface.station])
    def test_alice_418(self, alice):
        # Step 1
        response = alice('Сколько баллов пробки в Питере')
        _assert_traffic_level(response, geo='В Санкт-Петербурге')

        # Step 2
        # response = alice('Что с пробками на МКАДе?')
        # BUG: https://st.yandex-team.ru/ALICE-2120
        # _assert_traffic_level(response, geo='В Москве')

        # Step 3
        response = alice('Пробки в Прохоровке')
        _assert_traffic_nogeo_error(response, geo='в прохоровке')

        # Step 4
        # BUG: https://st.yandex-team.ru/ALICE-494
        # response = alice('Пробки в Атлантиде')
        # _assert_traffic_nogeo_error(response, geo='в Атлантиде')


class TestPalmTraffic(_TestPalmVoiceTraffic):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1088
    """

    @pytest.mark.parametrize('command', [
        'Какая ситуация на дорогах?',
        'Какие сейчас пробки?',
        'Как там на дорогах?',
    ])
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.yabro_win])
    def test_alice_1088(self, alice, command):
        response = alice(command)
        _assert_traffic_level_with_card(response, geo='В Москве')
        _assert_has_map_and_text_show_map(response.cards)


@pytest.mark.parametrize('surface', [surface.searchapp, surface.yabro_win, surface.launcher])
class TestPalmAllSurfaceTraffic(_TestPalmVoiceTraffic):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1248
        https://testpalm.yandex-team.ru/testcase/alice-1249
        https://testpalm.yandex-team.ru/testcase/alice-1250
        https://testpalm.yandex-team.ru/testcase/alice-1251
    """

    def test_alice_1248(self, alice):
        # Step 1
        response = alice('Пробки в Новосибирске')
        _assert_traffic_level_with_card(response, geo='В Новосибирске')
        _assert_has_map_and_text_show_map(response.cards)

        # Step 2
        response = alice('Яндекс пробки')
        _assert_traffic_level_with_card(response, geo='В Москве')
        _assert_has_map_and_text_show_map(response.cards)

        # Step 3
        # BUG: https://st.yandex-team.ru/ALICE-494
        # response = alice('Пробки в Атлантиде')
        # _assert_traffic_nogeo_error(response, geo='в Атлантиде')

        # Step 4
        response = alice('Пробки в Урюпинске')
        _assert_traffic_without_level_with_card(response, geo='в Урюпинске')
        _assert_has_map_and_text_show_map(response.cards)

    def test_alice_1249_1250_1251(self, alice):
        response = alice('Пробки в Москве')
        _assert_traffic_level_with_card(response, geo='В Москве')
        _assert_has_map_and_text_show_map(response.cards)


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestPalmAllStationTraffic(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-583
        https://testpalm.yandex-team.ru/testcase/alice-668
        https://testpalm.yandex-team.ru/testcase/alice-2052
    """

    def test_alice_583_668(self, alice):
        # Step 1
        response = alice('Пробки')
        _assert_traffic_level(response, geo='В Москве')

        # Step 2
        response = alice('Ситуация на дорогах')
        _assert_traffic_level(response, geo='В Москве')

    def test_alice_2052(self, alice):
        # Step 1
        response = alice('Пробки')
        _assert_traffic_level(response, geo='В Москве')

        # Step 2
        response = alice('Пробки в Питере')
        _assert_traffic_level(response, geo='В Санкт-Петербурге')


@pytest.mark.parametrize('surface', [surface.automotive])
class TestPalmAutomotiveTraffic(_TestPalmVoiceTraffic):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1561
        https://testpalm.yandex-team.ru/testcase/alice-1956
    """

    @pytest.mark.parametrize('command', [
        'Какая ситуация на дорогах?',
        'Какие сейчас пробки?',
        'Как там на дорогах?',
    ])
    def test_alice_1561_1956(self, alice, command):
        response = alice(command)
        _assert_traffic_level(response, geo='В Москве')


@pytest.mark.parametrize('surface', [surface.navi])
class TestPalmNaviTraffic(_TestPalmVoiceTraffic):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1522
        https://testpalm.yandex-team.ru/testcase/alice-2023
    """

    @pytest.mark.parametrize('command', [
        'Какая ситуация на дорогах?',
        'Какие сейчас пробки?',
        'Как там на дорогах?',
    ])
    def test_alice_1522(self, alice, command):
        response = alice(command)
        _assert_traffic_level(response, geo='В Москве')
        _assert_traffic_on_map_suggest(response)

    @pytest.mark.parametrize('command, geo', [
        ('Какая ситуация на дорогах?', 'В Москве'),
        ('Какие пробки в Питере?', 'В Санкт-Петербурге'),
    ])
    def test_alice_2023(self, alice, command, geo):
        response = alice(command)
        _assert_traffic_level(response, geo=geo)
        _assert_traffic_on_map_suggest(response)
