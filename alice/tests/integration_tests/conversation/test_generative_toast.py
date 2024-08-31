import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import alice.tests.library.surface as surface
import pytest


def get_original_intent(response):
    return response.scenario_analytics_info.object('gc_response_info').get('original_intent')


DEFAULT_EXPERIMENTS = (
    'hw_gc_enable_generative_toast',
    'bg_fresh_granet_form=alice.generative_toast',
    'mm_postclassifier_gc_force_intents=alice.generative_toast'
)
EDITORIAL_ONLY_EXPERIMENTS = DEFAULT_EXPERIMENTS + ('hw_gc_generative_toast_proba=0.0',)

INTENT_TOAST = 'alice.generative_toast.generative_toast'
INTENT_TOAST_TOPIC = 'alice.generative_toast.generative_toast.topic'
INTENT_TOAST_EDITORIAL = 'alice.generative_toast.generative_toast.editorial'
GREETINGS = 'С удовольствием, предложите тему для тоста'


def start_lets_talk(alice, lets_talk):
    if lets_talk:
        alice('давай поболтаем')


@pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestToasts(object):

    owners = ('nzinov',)

    @pytest.mark.parametrize('lets_talk', [True, False])
    def test_generative_toast_change_topic(self, alice, lets_talk):
        start_lets_talk(alice, lets_talk)
        alice('скажи тост про мандарины')
        response = alice('давай скажи тост')
        assert response.text == GREETINGS
        assert get_original_intent(response) == INTENT_TOAST_TOPIC

    @pytest.mark.parametrize('lets_talk', [True, False])
    def test_generative_toast(self, alice, lets_talk):
        start_lets_talk(alice, lets_talk)
        response = alice('давай скажи тост')
        assert response.text == GREETINGS
        assert get_original_intent(response) == INTENT_TOAST_TOPIC
        response = alice('мандарины')
        assert get_original_intent(response) == INTENT_TOAST
        response = alice('еще')
        assert get_original_intent(response) == INTENT_TOAST_TOPIC

    @pytest.mark.parametrize('lets_talk', [True, False])
    def test_generative_toast_full(self, alice, lets_talk):
        start_lets_talk(alice, lets_talk)
        response = alice('скажи тост про деда мороза')
        assert get_original_intent(response) == INTENT_TOAST

    @pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.safe})
    @pytest.mark.parametrize('lets_talk', [True, False])
    def test_generative_toast_safe(self, alice, lets_talk):
        start_lets_talk(alice, lets_talk)
        response = alice('давай скажи тост')
        assert get_original_intent(response) == INTENT_TOAST_EDITORIAL
        response = alice('скажи тост про кошек')
        assert get_original_intent(response) == INTENT_TOAST_EDITORIAL

    @pytest.mark.experiments(*EDITORIAL_ONLY_EXPERIMENTS)
    @pytest.mark.parametrize('lets_talk', [True, False])
    def test_generative_toast_editorial_only(self, alice, lets_talk):
        start_lets_talk(alice, lets_talk)
        response = alice('давай скажи тост')
        assert get_original_intent(response) == INTENT_TOAST_EDITORIAL
        response = alice('скажи тост про кошек')
        assert get_original_intent(response) == INTENT_TOAST_EDITORIAL

    def test_generative_toast_bad(self, alice):
        alice('скажи тост')
        response = alice('путин')
        assert 'toast' not in response.intent
        alice('скажи тост про путина')
        response = alice('путин')
        assert 'toast' not in response.intent
