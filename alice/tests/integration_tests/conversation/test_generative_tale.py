import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestGenerativeTale(object):

    owners = ('redin-d', )

    def test_generative_tale_full(self, alice):
        response = alice('придумай сказку')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GenerativeTaleAskCharacter
        assert 'сказк' in response.text_card.text

        response = alice('рыцарь')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GenerativeTale

        response = alice('хватит')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GenerativeTale

    def test_generative_tale_politics(self, alice):
        response = alice('придумай сказку')
        response = alice('про путина')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GenerativeTaleBanlistActivation
        assert 'не очень подходит' in response.text_card.text
        assert 'путин' not in response.text_card.text.lower()

        response = alice('хватит')
        response = alice('придумай сказку про путина')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GenerativeTaleBanlistActivation
        assert 'не очень подходит' in response.text_card.text

    def test_generative_tale_force_exit(self, alice):
        response = alice('придумай сказку про рыцаря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GenerativeTale

        response = alice('включи музыку')
        assert response.scenario != scenario.GeneralConversation

    def test_generative_tale_timeout_exit(self, alice):
        response = alice('придумай сказку про рыцаря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GenerativeTale
        assert 'Рыцар' in response.text_card.text

        alice.skip(minutes=3)
        response = alice('а дальше')
        assert response.intent != intent.GenerativeTale
