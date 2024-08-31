import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestMarketRecurringPurchase(object):

    owners = ('mllnr',)

    @pytest.mark.voice
    @pytest.mark.oauth(auth.Yandex)
    def test_offer_not_in_history(self, alice):
        alice('Купить туалетную бумагу как обычно')
        response = alice('шестислойную')
        assert response.intent == intent.RecurringPurchaseEllipsis
        assert response.text == 'Не нашла похожих товаров в вашей истории заказов. Зато смотрите, что можно купить на Яндекс.Маркете.'
        assert response.output_speech_text == 'Не нашла похожих товаров в вашей истории заказов. Зато смотрите что можно купить на яндекс маркете.'

    @pytest.mark.no_oauth
    def test_ask_login(self, alice):
        response = alice('купить молоко как обычно')
        assert response.intent == intent.RecurringPurchase
        assert response.text == 'Чтобы продолжить шопинг, вам необходимо авторизоваться. Затем просто повторите ваш запрос.'

        response = alice('я залогинился')
        assert response.intent == intent.RecurringPurchaseLogin
        assert response.text in (
            'К сожалению, вы все еще не залогинены. Войдите в свой аккаунт в приложении Яндекс.',
            'К сожалению, вы все еще не залогинены. Пожалуйста, авторизуйтесь в приложении Яндекс.',
            'К сожалению, вы все еще не залогинены. Вам нужно войти в свой аккаунт в приложении Яндекс.',
        )
