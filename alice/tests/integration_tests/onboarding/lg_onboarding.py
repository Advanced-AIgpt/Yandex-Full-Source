import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestOnboarding(object):

    owners = ('amullanurov',)

    def test_onboarding(self, alice):
        response = alice('Что ты ты умеешь')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.Onboarding
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'yandex.alice'
        assert response.directive.payload.params_json == '{\"request\": \"launch\", \"data\": {\"mode\": \"onboarding\"}}'
        assert response.text == 'Сейчас покажу.'
