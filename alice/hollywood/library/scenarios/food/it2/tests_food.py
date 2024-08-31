import re

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, HttpResponseStub, StubberEndpoint


food_pa_stubber = create_stubber_fixture(
    'tc.mobile.yandex.net',
    80,
    [
        StubberEndpoint('/4.0/startup', ['POST']),
        StubberEndpoint('/4.0/eda-superapp/api/v2/catalog', ['GET']),
        StubberEndpoint('/4.0/eda-superapp/api/v1/user/addresses', ['GET']),
        StubberEndpoint('/4.0/eda-superapp/api/v1/orders', ['GET']),
        StubberEndpoint('/4.0/eda-superapp/api/v2/catalog/{place_slug}/menu', ['GET']),
        StubberEndpoint('/4.0/eda-superapp/api/v1/cart/sync', ['POST']),
    ],
    scheme='http',
    stubs_subdir='food_pa',
)


def get_frozen_stubs(menu_stubs):
    return {
        '/4.0/startup': [
            HttpResponseStub(
                200,
                'freeze_stubs/pa_startup.json',
                headers={'X-YaTaxi-UserId': '09057f41a8814bdaadb9550a26cd77ac'},
            ),
        ],
        '/4.0/eda-superapp/api/v2/catalog': [
            HttpResponseStub(200, 'freeze_stubs/pa_catalog.json'),
        ],
        '/4.0/eda-superapp/api/v1/user/addresses': [
            HttpResponseStub(200, 'freeze_stubs/pa_user_addresses.json'),
        ],
        '/4.0/eda-superapp/api/v2/catalog/{place_slug}/menu': [
            HttpResponseStub(200, stub_name)
            for stub_name in menu_stubs
        ],
        '/4.0/eda-superapp/api/v1/cart/sync': [
            HttpResponseStub(200, 'freeze_stubs/pa_sync_cart.json'),
        ],
    }


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['food']


@pytest.fixture(scope='function')
def srcrwr_params(food_pa_stubber):
    return {
        'HOLLYWOOD_PASSENGER_AUTHORIZER': f'localhost:{food_pa_stubber.port}',

        # Avoid Eda backend timeouts
        'FOOD_GET_TAXI_UID_PROXY': '::3000',
        'FOOD_FIND_PLACE_PA_PROXY': '::3000',
        'FOOD_GET_MENU_PA_PROXY': '::3000',
        'FOOD_GET_MENU_PA_PREPARE': '::300',
        'SCENARIO_FOOD_RUN': '::4000',
        'SCENARIO_FOOD_COMMIT': '::4000',
    }


@pytest.mark.freeze_stubs(food_pa_stubber=get_frozen_stubs(['freeze_stubs/pa_menu.json']))
@pytest.mark.oauth(auth.RobotEater)
@pytest.mark.scenario(name='Food', handle='food')
@pytest.mark.experiments('mm_enable_protocol_scenario=Food', 'bg_fresh_granet_prefix=alice.food.form_order')
class TestFood(object):
    owners = ('the0', 'samoylovboris', 'flimsywhimsy')

    @pytest.mark.parametrize('surface', [surface.station])
    def test_make_order_station(self, alice):
        r = alice(voice('закажи в макдональдсе бигмак'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-то ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        suggests = layout.SuggestButtons
        assert len(suggests) == 2
        assert suggests[0].ActionButton.Title == 'Всё'
        assert re.match(r'Добавь .+', suggests[1].ActionButton.Title)

        r = alice(voice('всё'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert re.match(
            r'В корзине:\nБиг Мак\.\nСумма вашего заказа \d+ р\.\nЗаказываем\?',
            layout.OutputSpeech,
        )
        assert re.match(
            r'В корзине:\n\* Биг Мак – \d+ р\.\nСумма вашего заказа \d+ р\.\nЗаказываем\?',
            layout.Cards[0].Text,
        )

        r = alice(voice('да'))
        assert r.scenario_stages() == {'run', 'commit'}

        layout = r.run_response.CommitCandidate.ResponseBody.Layout
        assert layout.Cards[0].Text == 'Откройте Яндекс.Еду для оплаты. Прислала вам ссылку в телефон.'
        assert layout.OutputSpeech == 'Откройте Яндекс Еду для оплаты. Прислала вам ссылку в телефон.'

        suggests = layout.SuggestButtons
        assert len(suggests) == 1
        assert suggests[0].ActionButton.Title == 'Что ты умеешь?'

        server_directives = r.run_response.CommitCandidate.ResponseBody.ServerDirectives
        assert len(server_directives) == 1

        push_directive = server_directives[0].PushMessageDirective
        assert push_directive.Title == 'Оплата заказа'
        assert push_directive.Body == 'Оплатите заказ в приложении'
        assert re.match(r'https://eda\.yandex/cart\?lat=55\.73\d\d\d\d&lon=37\.58\d\d\d\d', push_directive.Link)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_make_order_search_app(self, alice):
        r = alice(voice('закажи в макдональдсе бигмак'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-то ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        suggests = layout.SuggestButtons
        assert len(suggests) == 2
        assert suggests[0].ActionButton.Title == 'Всё'
        assert re.match(r'Добавь .+', suggests[1].ActionButton.Title)

        r = alice(voice('всё'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Вот какие блюда в вашей корзине. Заказываем?'
        assert re.match(
            r'В корзине:\n\* Биг Мак – \d+ р\.\nСумма вашего заказа \d+ р\.\nЗаказываем\?',
            layout.Cards[0].Text,
        )

        r = alice(voice('да'))
        assert r.scenario_stages() == {'run', 'commit'}

        layout = r.run_response.CommitCandidate.ResponseBody.Layout
        assert layout.Cards[0].Text == 'Осталось совсем чуть-чуть. Откройте Яндекс.Еду и оплатите заказ.'
        assert layout.OutputSpeech == 'Осталось совсем чуть-чуть. Откройте Яндекс Еду и оплатите заказ.'

        suggests = layout.SuggestButtons
        assert len(suggests) == 1
        assert suggests[0].ActionButton.Title == 'Что ты умеешь?'

    @pytest.mark.parametrize('surface', [surface.station, surface.searchapp])
    def test_start_mc(self, alice):
        r = alice(voice('закажи еду'))

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in {
            'Что закажем?',
            'Чтобы сделать заказ, скажите, например: «Алиса, закажи чизбургер и колу». Что будем заказывать?',
        }
        assert len(layout.SuggestButtons) == 3
        assert len(r.run_response.ResponseBody.FrameActions) == 5
        suggests = layout.SuggestButtons
        assert suggests[0].ActionButton.Title == 'Биг Мак'
        assert suggests[1].ActionButton.Title == 'Картофель Фри'
        assert suggests[2].ActionButton.Title == 'Кока-Кола'

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_keep_old_cart(self, alice):
        r = alice(voice('Закажи биг мак в макдональдсе'))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-то ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('отмена'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Отменила заказ.'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('Закажи в макдональдсе'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert re.match(
            r'В вашей корзине уже есть заказ\. Продолжим\?',
            layout.OutputSpeech,
        )
        assert re.match(
            r'В корзине:\n\* Биг Мак – \d+ р\.\nПродолжим\?',
            layout.Cards[0].Text,
        )
        assert len(layout.SuggestButtons) == 2
        assert layout.SuggestButtons[0].ActionButton.Title == 'Да'
        assert layout.SuggestButtons[1].ActionButton.Title == 'Очисти корзину'

        r = alice(voice('да'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-нибудь ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('всё'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Вот какие блюда в вашей корзине. Заказываем?'
        assert re.match(
            r'В корзине:\n\* Биг Мак – \d+ р\.\nСумма вашего заказа \d+ р\.\nЗаказываем\?',
            layout.Cards[0].Text,
        )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_keep_old_cart_add_item(self, alice):
        r = alice(voice('Закажи биг мак в макдональдсе'))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-то ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('отмена'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Отменила заказ.'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('Закажи чизбургер в макдональдсе'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'В вашей корзине уже есть заказ. Продолжим?'

        r = alice(voice('да'))

        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что ещё вы хотите заказать?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('всё'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Вот какие блюда в вашей корзине. Заказываем?'
        assert re.match(
            r'В корзине:\n\* Биг Мак – \d+ р\.\n\* Чизбургер – \d+ р\.\nСумма вашего заказа \d+ р\.\nЗаказываем\?',
            layout.Cards[0].Text,
        )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_join_repeated_items(self, alice):
        r = alice(voice('Закажи бигмак в макдональдсе'))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-то ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('Закажи картошку'))
        r = alice(voice('Закажи бигмак'))
        r = alice(voice('добавь бигмак и картошку'))

        r = alice(voice('всё'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Вот какие блюда в вашей корзине. Заказываем?'
        assert re.match(
            r'В корзине:\n\* 3 x Биг Мак – \d+ р\.\n\* 2 x Картофель Фри – \d+ р\.\nСумма вашего заказа \d+ р\.\nЗаказываем\?',
            layout.Cards[0].Text,
        )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_where_from(self, alice):
        r = alice(voice('закажи в макдональдсе бигмак'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-то ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        suggests = layout.SuggestButtons
        assert len(suggests) == 2
        assert suggests[0].ActionButton.Title == 'Всё'

        r = alice(voice('откуда еда'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Заказ будет доставлен из ближайшего Макдональдса.'
        assert layout.Cards[0].Text == layout.OutputSpeech

        suggests = layout.SuggestButtons
        assert len(suggests) == 2
        assert suggests[0].ActionButton.Title == 'Всё'

        r = alice(voice('закажи биг мак'))
        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что ещё вы хотите заказать?'


@pytest.mark.oauth(auth.RobotEater)
@pytest.mark.scenario(name='Food', handle='food')
@pytest.mark.experiments('mm_enable_protocol_scenario=Food', 'bg_fresh_granet_prefix=alice.food.form_order')
class TestFoodOutdated(object):
    owners = ('the0', 'samoylovboris', 'flimsywhimsy')

    @pytest.mark.freeze_stubs(food_pa_stubber=get_frozen_stubs(['freeze_stubs/pa_menu.json', 'freeze_stubs/pa_menu_outdated.json']))
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_all_cart_oudated(self, alice):
        r = alice(voice('Закажи биг мак в макдональдсе'))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-то ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('всё'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Блюд из вашей корзины больше нет в меню. Что хотите заказать?'
        assert layout.Cards[0].Text == layout.OutputSpeech

    @pytest.mark.freeze_stubs(food_pa_stubber=get_frozen_stubs(['freeze_stubs/pa_menu.json', 'freeze_stubs/pa_menu_price_updated.json']))
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_cart_updated_price(self, alice):
        r = alice(voice('Закажи биг мак в макдональдсе'))

        assert r.scenario_stages() == {'run'}

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Что-то ещё?'
        assert layout.Cards[0].Text == layout.OutputSpeech

        r = alice(voice('всё'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Вот какие блюда в вашей корзине. Заказываем?'
        assert re.match(
            r'В корзине:\n\* Биг Мак – 100 р\.\nСумма вашего заказа 100 р\.\nЗаказываем\?',
            layout.Cards[0].Text,
        )

    @pytest.mark.freeze_stubs(food_pa_stubber=get_frozen_stubs(['freeze_stubs/pa_menu_unavailable.json']))
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_cart_add_unknown_anavailable_items(self, alice):
        r = alice(voice('Закажи картошку в макдональдсе'))

        assert r.scenario_stages() == {'run'}

        r = alice(voice('Закажи биг мак, чизбургер и макароны'))

        assert r.scenario_stages() == {'run', 'commit'}
        layout = r.run_response.CommitCandidate.ResponseBody.Layout
        assert re.match(
            r'Не могу найти в меню:\n\* «макароны»\nСейчас в Макдональдсе нет:\n\* Биг Мак\nСейчас открою Яндекс.Еду, продолжите заказ там.',
            layout.Cards[0].Text,
        )
