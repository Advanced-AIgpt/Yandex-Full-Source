import re
from urllib.parse import unquote

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import route.div_card
import poi.div_card


full_star_url = 'https://avatars.mds.yandex.net/get-bass/895508/poi_32x32_996ab8b5c5fbdd35642f9ec5341a8efde5111b1a25a1a0d566baefae8cf82a34.png/orig'
half_star_url = 'https://avatars.mds.yandex.net/get-bass/787408/poi_32x32_9c26a0ffb0fa60e4cb528321dab52398780fd84b5dd49e03834f80113c897eaa.png/orig'
empty_star_url = 'https://avatars.mds.yandex.net/get-bass/397492/poi_32x32_206f51af609c5e88a0e473e05ec6f01d3fbc4ca2dc3ff18b7ee68e72def0b815.png/orig'
star_urls = [full_star_url, half_star_url, empty_star_url]


def _assert_button(button, assert_url_func):
    assert button
    assert assert_url_func(unquote(button.action_url))


def _try_assert_button(button, assert_url_func):
    if button:
        _assert_button(button, assert_url_func)


def _assert_open_hours(open_hours, open_slot=None):
    if open_slot == '"open_24h"':
        assert open_hours == 'Круглосуточно'
    elif open_slot == '"open"':
        assert re.search('Открыто|Круглосуточно', open_hours)
    else:
        assert re.search('Открыто|Круглосуточно|До открытия|Перерыв|До перерыва|Закрыто|До закрытия|не работает', open_hours)


def _assert_action_url(action_obj, uri):
    assert action_obj.action_url
    assert action_obj.directives[0]['name'] == directives.names.OpenUriDirective
    assert uri in unquote(action_obj.directives[0]['payload']['uri'])


@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
])
class TestPalmGalleryPoi(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-335
    https://testpalm.yandex-team.ru/testcase/alice-1093
    https://testpalm.yandex-team.ru/testcase/alice-1546
    https://testpalm.yandex-team.ru/testcase/alice-1547
    https://testpalm.yandex-team.ru/testcase/alice-1549
    https://testpalm.yandex-team.ru/testcase/alice-1670
    https://testpalm.yandex-team.ru/testcase/alice-1671
    """

    owners = ('mihajlova', )

    @pytest.mark.parametrize('command, what_slot, where_slot, open_slot', [
        ('Найти кафе рядом', 'кафе', '"nearby"', 'null'),
        ('Найди кафе открытое сейчас', 'кафе', 'null', '"open_now"'),
        ('Найди ближайшую аптеку', 'аптеку', '"nearest"', 'null'),
        ('Найди ближайшую круглосуточную аптеку', 'аптеку', '"nearest"', '"open_24h"'),
        ('Куда сходить в кино?', 'сходить в кино', 'null', 'null'),
        ('Магазины одежды в Калининграде', 'магазины одежды', 'калининграде', 'null'),
        ('Где тут рядом бассейн?', 'бассейн', '"nearby"', 'null'),
        ('Театр рядом', 'театр', '"nearby"', 'null'),
        ('Гостиный двор', 'гостиный двор', 'null', 'null'),
        ('Где мне пообедать', 'пообедать', 'null', 'null'),
        pytest.param(
            'Где выпить коктейль', 'выпить коктейль', 'null', 'null',
            marks=pytest.mark.xfail(reason='Ошибка классификации в поиск. Для Алисы в облачке не критично')
        ),
    ])
    def test_gallery(self, alice, command, what_slot, where_slot, open_slot):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi
        assert response.text in [
            'Вот что я нашла.',
            'Вот, смотрите.',
            'Всё для вас.',
            'Вот варианты.',
            'Нашла.',
            'Смотрите, выбирайте.',
            'Выбирайте.',
            'Нашла разные варианты.',
        ]

        # Проверка тематики галереи
        assert response.slots['what'].string == what_slot
        assert response.slots['where'].string == where_slot
        assert response.slots['open'].string == open_slot

        gallery = poi.div_card.PoiGallery(response.div_card)
        assert len(gallery) > 1, 'there must be items in the gallery'
        assert gallery.tail.text == 'Все результаты поиска'
        assert gallery.tail.log_id == 'poi_gallery__serp'
        _assert_action_url(gallery.tail, what_slot)

        for card in gallery:
            assert card.name
            assert card.log_id == 'poi_gallery_card__whole_card'
            assert card.action_url
            assert card.address
            _assert_action_url(card.map, 'intent://yandex.ru/maps?')
            assert re.search(r'pt=(\d|\.|,)+pm2rdl', unquote(card.map.image_url))
            assert card.has_distance_cursor
            if card.distance:
                assert re.search(r'(\d+ )?(\d+,)?\d+ к?м', card.distance)
            if card.reviews:
                assert 'отзыв' in card.reviews_text
                for star in card.reviews_stars:
                    assert star.image_url in star_urls
                _assert_action_url(card.reviews, 'intent=reviews')
            if card.open_hours:
                _assert_open_hours(card.open_hours, open_slot)

            _assert_button(card.button('Маршрут'), lambda url: 'intent://yandex.ru/maps?' in url)
            _assert_button(card.button('Такси'), lambda url: 'intent://route?' in url)
            _try_assert_button(card.button('Телефон'), lambda url: 'tel:' in url)
            _try_assert_button(card.button('Сайт'), lambda url: re.search('https?://', url))

    @pytest.mark.parametrize('command, what_slot, where_slot', [
        ('Домодедово', 'домодедово', '')
    ])
    def test_found_one(self, alice, command, what_slot, where_slot):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi
        assert response.text in [
            'Смотрите, что нашлось.',
            'Кажется, вам нужно это место.',
            'Смотрите.',
            'Вот оно!',
            'Всё для вас.',
        ]

        # Проверка тематики галереи
        assert response.slots['what'].string == what_slot
        assert response.slots['where'].string == where_slot

        card = poi.div_card.PoiCard(response.div_cards)
        assert what_slot in card.info.name.lower()
        _assert_action_url(card.info, 'https://yandex.ru/profile/45745811341?lr=213')

        _assert_action_url(card.images.map, 'intent://yandex.ru/maps?')
        assert re.search(r'pt=(\d|\.|,)+pm2rdl', unquote(card.images.map_image_url))
        for foto in card.images.fotos():
            _assert_action_url(foto, 'https://yandex.ru/profile/45745811341?intent=photo')

        assert card.info.has_distance_cursor
        assert card.info.address == 'Россия, Московская область, городской округ Домодедово, аэропорт Домодедово имени М.В. Ломоносова, 1'
        assert re.search(r'(\d+ )?(\d+,)?\d+ к?м', card.info.distance)

        assert card.info.descripsion
        assert 'Туалет для инвалидов, комната отдыха, wi-fi, парковка для инвалидов, автоматическая дверь' in card.info.descripsion

        assert 'отзыв' in card.info.reviews_text
        for star in card.info.reviews_stars:
            assert star.image_url in star_urls
        _assert_action_url(card.info.reviews, 'https://yandex.ru/profile/45745811341?intent=reviews')

        _assert_open_hours(card.info.open_hours)

        _assert_button(card.info.button('Маршрут'), lambda url: 'intent://yandex.ru/maps?' in url)
        _assert_button(card.info.button('Такси'), lambda url: 'intent://route?' in url)
        _assert_button(card.info.button('Телефон'), lambda url: 'tel:+74959336666' in url)
        _assert_button(card.info.button('Сайт'), lambda url: 'https://www.dme.ru' in url)

        response = alice.click(card.info.button('Маршрут'))
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert what_slot in response.text.lower()

        routes = route.div_card.RouteGallery(response.div_card)
        assert len(routes) == 3, 'Expect three routes'
        for route_card in routes:
            assert route_card.map
            map_url = unquote(route_card.map.image_url)
            assert map_url.startswith('https://static-maps.yandex.ru')
            assert re.search(r'pt=(\d|\.|,)+ya_ru~(\d|\.|,)+round', map_url), 'Expext start/finish route points'
            assert re.search(r'pl=c:8822DDC0,w:5(\d|\.|,)', map_url), 'Expect route line'
            assert route_card.icon.image_url.startswith('https://avatars.mds.yandex.net')
            assert re.search(r'МАРШРУТ (ПЕШКОМ|НА ТРАНСПОРТЕ|НА АВТО)', route_card.footer.text)
            assert route_card.footer.action_url, 'Expect link to Ya.Maps'
            if 'ПЕШКОМ' in route_card.footer.text:
                _assert_action_url(route_card.footer, 'intent://yandex.ru/maps?rtext=')
                assert re.search(r'\d+ мин, \d+ км', route_card.distance)
            elif 'НА ТРАНСПОРТЕ' in route_card.footer.text:
                _assert_action_url(route_card.footer, 'intent://yandex.ru/maps?rtext=')
                assert re.search(r'\d+ мин', route_card.distance)
            elif 'НА АВТО' in route_card.footer.text:
                _assert_action_url(route_card.footer, 'intent://build_route_on_map?')
                assert re.search(r'\d+ мин, \d+ км', route_card.distance)

    def test_not_exists(self, alice):
        response = alice('морской порт город великий новгород')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi
        assert response.text in [
            'К сожалению, ничего не удалось найти.',
            'Ничего не нашлось.',
            'Боюсь, что ничего не нашлось.',
            'К сожалению, я ничего не нашла.',
        ]

    def test_one_more(self, alice):
        response = alice('Где мне пообедать')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoi

        cafes = poi.div_card.PoiGallery(response.div_card)
        assert len(cafes) > 1, 'there must be items in the gallery'
        for cafe in cafes:
            assert cafe.name
            assert cafe.action_url
            assert cafe.address
        first_cafe = cafes[0]

        response = alice('А ещё?')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.FindPoiScrollNext

        cafes = poi.div_card.PoiGallery(response.div_card)
        assert len(cafes) > 1, 'there must be items in the gallery'
        for cafe in cafes:
            assert cafe.name
            assert cafe.action_url
            assert cafe.address
        assert first_cafe.name != cafes[0].name, 'Expect different first items, but got the same'
