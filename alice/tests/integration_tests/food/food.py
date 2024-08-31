import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.RobotEater)
@pytest.mark.region(region.Moscow)
@pytest.mark.experiments(f'mm_enable_protocol_scenario={scenario.Food}', 'bg_fresh_granet_prefix=alice.food.form_order', 'hw_food_hardcoded_menu')
class TestFood(object):
    owners = ('samoylovboris', 'flimsywhimsy')

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-724')
    @pytest.mark.parametrize('surface', [surface.station])
    def test_make_order_station(self, alice):
        response = alice('закажи в макдональдсе бигмак')
        assert response.scenario == scenario.Food
        assert response.text == 'Что-то ещё?'
        assert len(response.suggests) == 2
        assert response.suggests[0].title == 'Да'
        assert re.match(r'Добавь .+', response.suggests[1].title)

        response = alice('всё')
        assert response.scenario == scenario.Food
        assert re.match(r'В корзине:\n\* Биг Мак – \d+ р.\nСумма вашего заказа \d+ р.\nЗаказываем\?', response.text)

        response = alice('да')
        assert response.scenario == scenario.Food
        assert response.text == 'Откройте Яндекс.Еду для оплаты. Прислала вам ссылку в телефон.'
        assert len(response.suggests) == 1
        assert response.suggests[0].title == 'Что ты умеешь?'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-724')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_make_order_search_app(self, alice):
        response = alice('закажи в макдональдсе бигмак')
        assert response.scenario == scenario.Food
        assert response.text == 'Что-то ещё?'
        assert len(response.suggests) == 2
        assert response.suggests[0].title == 'Да'
        assert re.match(r'Добавь .+', response.suggests[1].title)

        response = alice('всё')
        assert response.scenario == scenario.Food
        assert re.match(r'В корзине:\n\* Биг Мак – \d+ р.\nСумма вашего заказа \d+ р.\nЗаказываем\?', response.text)

        response = alice('да')
        assert response.scenario == scenario.Food
        assert response.text == 'Осталось совсем чуть-чуть. Откройте Яндекс.Еду и оплатите заказ.'
        assert response.directive.name == directives.names.OpenUriDirective
        assert re.match(r'https://eda\.yandex/cart\?lat=55\.73\d\d\d\d&lon=37\.58\d\d\d\d', response.directive.payload.uri)
        assert len(response.suggests) == 1
        assert response.suggests[0].title == 'Что ты умеешь?'

    @pytest.mark.parametrize('surface', [surface.station])
    def test_unknown_item_station(self, alice):
        response = alice('закажи в макдональдсе пиццу')
        assert response.scenario == scenario.Food
        assert response.text == 'Не могу найти в меню:\n* «пицца»\nМожет продолжить заказ в телефоне?'
        assert len(response.suggests) == 1
        assert response.suggests[0].title == 'Что ты умеешь?'

        response = alice('да')
        assert response.scenario == scenario.Food
        assert response.text == 'Отправила вам пуш в приложении Яндекса. Завершите заказ там.'
        assert len(response.suggests) == 1
        assert response.suggests[0].title == 'Что ты умеешь?'

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_unknown_item_search_app(self, alice):
        response = alice('закажи в макдональдсе пиццу')
        assert response.scenario == scenario.Food
        assert response.text == 'Не могу найти в меню:\n* «пицца»\nСейчас открою Яндекс.Еду, продолжите заказ там.'
        assert response.directive.name == directives.names.OpenUriDirective
        assert re.match(r'https://eda\.yandex/cart\?lat=55\.73\d\d\d\d&lon=37\.58\d\d\d\d', response.directive.payload.uri)
        assert len(response.suggests) == 1
        assert response.suggests[0].title == 'Что ты умеешь?'

    @pytest.mark.version(hollywood=185, megamind=218)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_fall_out_from_scenario(self, alice):
        response = alice('закажи в макдональдсе бигмак')
        assert response.scenario == scenario.Food
        assert response.text == 'Что-то ещё?'

        response = alice('какая погода?')
        response = alice('какая погода?')
        response = alice('какая погода?')
        assert response.scenario != scenario.Food

        response = alice('откуда еда')
        assert response.scenario == scenario.Food
        assert response.text == 'Заказ будет доставлен из ближайшего Макдональдса.'
