import re

import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


def check_response(expected, response):
    assert re.fullmatch(expected, response.text)
    assert response.has_voice_response()
    assert re.fullmatch(expected, response.output_speech_text)


class ShoppingLabel(object):
    already_added = r'Такой товар уже есть в вашем списке покупок\.'
    already_added_multiple = r'Такие товары уже есть в вашем списке покупок\.'
    added_or_already_added = r'Добавила( \S.*)? в ваш список покупок( \S.*)?\.|(Такой товар уже есть в вашем списке покупок\.)|(Такие товары уже есть в вашем списке покупок\.)'
    all_was_deleted = r'Ваш список покупок теперь пуст\.'
    all_was_already_deleted = r'Ваш список покупок уже пуст\.'
    all_was_deleted_or_already_deleted = r'(Ваш список покупок теперь пуст\.)|(Ваш список покупок уже пуст\.)'
    what_add = r'Чтобы пополнить список покупок.*'
    empty = r'Ваш список покупок пока пуст\.'
    show_list_on_screenless_device = r'В вашем списке покупок сейчас лежит 2 товара\.\n1\) колбаса\.\n2\) майонез\.'
    show_list_on_screen_device = r'Вот что сейчас в вашем списке покупок\.'


@pytest.mark.skip
@pytest.mark.voice
@pytest.mark.oauth(auth.RobotTestShoppingList)
@pytest.mark.experiments('shopping_list')
class _TestMarketShoppingListBase(object):
    owners = ('mllnr',)


@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestMarketShoppingList(_TestMarketShoppingListBase):

    def test_add_external(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # exit the scenario
        response = alice('Привет')
        assert response.intent == intent.Hello
        # start test
        response = alice('Добавь литр молока в мой список покупок')
        check_response('Добавила литр молока в ваш список покупок.', response)

    def test_add_internal(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # start test
        response = alice('Добавь литр молока в список')
        check_response('Добавила литр молока в ваш список покупок.', response)

    def test_add_empty(self, alice):
        response = alice('Добавь в мой список покупок')
        assert response.intent == intent.ShoppingListAdd
        check_response(ShoppingLabel.what_add, response)

    def test_add_many_items(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # exit the scenario
        response = alice('Привет')
        assert response.intent == intent.Hello
        # start test
        response = alice('Добавь пшёнку, тушёнку и хлеб в мой список покупок')
        check_response('Добавила в ваш список покупок пшенку, тушенку и хлеб.', response)

    def test_item_already_added(self, alice):
        response = alice('Добавь картошку в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        response = alice('Добавь картошку в мой список покупок')
        check_response(ShoppingLabel.already_added, response)

    def test_already_added_multiple(self, alice):
        response = alice('Добавь щётку и пасту в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        response = alice('Добавь щётку и пасту в мой список покупок')
        check_response(ShoppingLabel.already_added_multiple, response)

    def test_show_list_empty(self, alice):
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        response = alice('Покажи мой список покупок')
        check_response(ShoppingLabel.empty, response)

    def test_delete_external(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # fill shopping_list
        response = alice('Добавь газировку в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # exit the scenario
        response = alice('Привет')
        assert response.intent == intent.Hello
        # start test
        response = alice('Удали газировку из моего списка покупок')
        check_response('Удалила газировку из вашего списка покупок.', response)

    def test_delete_internal(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # fill shopping_list
        response = alice('Добавь газировку в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # start test
        response = alice('Удали газировку')
        check_response('Удалила газировку из вашего списка покупок.', response)

    def test_delete_not_existing(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # fill shopping_list
        response = alice('Добавь газировку в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # start test
        response = alice('Удали яблоки')
        check_response('Не нашла такого товара в вашем списке покупок.', response)

    def test_delete_multiple(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # fill shopping_list
        response = alice('Добавь газировку, бананы в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # start test
        response = alice('Удали бананы и газировку')
        check_response('Удалила из вашего списка покупок бананы и газировку.', response)

    def test_delete_by_index_when_list_wasnt_shown_before(self, alice):
        # fill shopping_list
        response = alice('Добавь газировку, бананы, пирожки в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # start test
        response = alice('Удали пункт 2 из списка')
        check_response(r'Я уже и не помню что там было на какой позиции. Давайте вместе посмотрим в список покупок\?', response)

    def test_delete_all_external(self, alice):
        # fill shopping_list
        response = alice('Добавь газировку, бананы, пирожки в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # exit the scenario
        response = alice('Привет')
        assert response.intent == intent.Hello
        # start test
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted, response)

    def test_delete_all_internal(self, alice):
        # fill shopping_list
        response = alice('Добавь газировку, бананы, пирожки в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # start test
        response = alice('Удали всё')
        check_response(ShoppingLabel.all_was_deleted, response)

    def test_delete_all_from_empty_list(self, alice):
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_already_deleted, response)


@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
    surface.yabro_win,
])
class TestMarketShoppingListScreen(_TestMarketShoppingListBase):

    def test_show_list_on_screen_device(self, alice):
        response = alice('Добавь щётку и пасту в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        response = alice('Покажи мой список покупок')
        check_response(ShoppingLabel.show_list_on_screen_device, response)
        assert 'simple_text' in [c.type for c in response.cards]
        assert response.div_card

    def test_delete_by_index(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # fill shopping_list
        response = alice('Добавь газировку, бананы, пирожки в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # start test
        response = alice('Покажи мой список покупок')
        check_response(ShoppingLabel.show_list_on_screen_device, response)
        # start test
        response = alice('Удали пункт 2 из списка')
        check_response('Удалила бананы из вашего списка покупок.', response)

    def test_delete_multiple_by_index(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # fill shopping_list
        response = alice('Добавь газировку, бананы, пирожки в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        # start test
        response = alice('Покажи мой список покупок')
        check_response(ShoppingLabel.show_list_on_screen_device, response)
        response = alice('Удали из списка покупок пункт 2 и 3')
        check_response('Удалила из вашего списка покупок бананы и пирожки.', response)


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
])
class TestMarketShoppingListScreenless(_TestMarketShoppingListBase):

    def test_show_list_on_screenless_device(self, alice):
        # clear shopping list
        response = alice('Очисти мой список покупок')
        check_response(ShoppingLabel.all_was_deleted_or_already_deleted, response)
        # start test
        response = alice('Добавь колбасу и майонез в мой список покупок')
        check_response(ShoppingLabel.added_or_already_added, response)
        response = alice('Покажи мой список покупок')
        check_response(ShoppingLabel.show_list_on_screenless_device, response)
