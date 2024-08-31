import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestPushNotification(object):
    push_responses = [
        'Отправила вам пуш с дополнительной информацией. Чтобы его прочесть, зайдите в приложение Яндекс.',
        'Отправила уведомление, продолжим в приложении Яндекс, откройте его.',
        'Отправила вам сообщение, чтобы вы могли изучить вопрос самостоятельно. Чтобы его прочесть, зайдите в приложение Яндекс.',
        'Отправила ссылку с информацией, продолжим в приложении Яндекс, откройте его.',
    ]

    owners = ('fadeich', 'svetlana-yu')

    @pytest.mark.experiments('handoff_promo_proba_1.0')
    def test_promo_agree(self, alice):
        response = alice('расскажи какая калорийность у яблока')
        assert response.intent == intent.Factoid
        assert 'handoff_promo' in response.scenario_analytics_info.objects
        response = alice('да')
        assert response.text in self.push_responses

    def test_send_push(self, alice):
        response = alice('расскажи какая калорийность у яблока')
        assert response.intent == intent.Factoid
        response = alice('отправь пуш')
        assert response.text in self.push_responses

    @pytest.mark.experiments('handoff_promo_proba_1.0')
    def test_promo_disagree_do_nothing(self, alice):
        response = alice('расскажи какая калорийность у яблока')
        assert response.intent == intent.Factoid
        assert 'handoff_promo' in response.scenario_analytics_info.objects
        response = alice('нет')
        assert response.text is None
