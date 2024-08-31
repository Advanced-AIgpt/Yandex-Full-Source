import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestOnboardingGreeting(object):

    owners = ('sparkle',)

    def _check_show_buttons_directive(self, response_directives, buttons_count, button_directives):
        directive = next(d for d in response_directives if d.name == directives.names.ShowButtonsDirective)
        assert directive
        assert directive.payload.screen_id == 'cloud_ui'

        buttons = directive.payload.buttons
        assert len(buttons) == buttons_count
        for button in buttons:
            assert button.title
            assert button.text

            button_dirs = button.directives
            assert len(button_dirs) == len(button_directives)

            for i, bd in enumerate(button_directives):
                assert button_dirs[i].type == bd['type']
                assert button_dirs[i].name == bd['name']
                if bd.get('ignore_answer'):
                    assert button_dirs[i].ignore_answer

    def _check_fill_cloud_ui_directive(self, response_directives):
        directive = next(d for d in response_directives if d.name == directives.names.FillCloudUiDirective)
        assert directive
        assert directive.payload.text == 'Чем могу помочь?'

    @pytest.mark.experiments('onboarding_use_cloud_ui')
    @pytest.mark.supported_features('cloud_ui', 'cloud_ui_filling')
    @pytest.mark.parametrize('is_first_start, buttons_count', [
        pytest.param(True, 5, id='first_start'),
        pytest.param(False, 3, id='usual_start'),
    ])
    def test_with_cloud_ui(self, alice, is_first_start, buttons_count):
        response = alice.greet(first_start=is_first_start)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.SkillRecommendation
        assert response.card
        assert len(response.directives) == 2
        self._check_show_buttons_directive(response.directives, buttons_count, [{
            'type': 'client_action',
            'name': directives.names.TypeTextDirective,
        }, {
            'type': 'server_action',
            'name': 'external_source_action',
            'ignore_answer': True,
        }, {
            'type': 'server_action',
            'name': 'on_card_action',
            'ignore_answer': True,
        }])
        self._check_fill_cloud_ui_directive(response.directives)

    @pytest.mark.experiments('onboarding_use_cloud_ui')
    @pytest.mark.supported_features('cloud_ui', 'cloud_ui_filling')
    def test_image_search_onboarding_with_cloud_ui(self, alice):
        response = alice.image_search_onboarding()
        assert response.scenario == scenario.Vins
        assert response.intent == intent.OnboardingImageSearch
        assert response.card
        assert len(response.directives) == 2
        self._check_show_buttons_directive(response.directives, 9, [{
            'type': 'client_action',
            'name': directives.names.StartImageRecognizerDirective,
        }, {
            'type': 'server_action',
            'name': 'on_card_action',
            'ignore_answer': True,
        }])
        self._check_fill_cloud_ui_directive(response.directives)
