import collections.abc

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class Intent:

    def __init__(self, name):
        self.name = name

    def __eq__(self, other):
        valid_intent_names = (self.name, 'alice.recipes.' + self.name)
        if isinstance(other, str):
            return other in valid_intent_names
        elif isinstance(other, Intent):
            return other.name in valid_intent_names
        else:
            return False

    def __str__(self):
        return f'Intent(name = \'{self.name}\')'

    def __repr__(self):
        return str(self)


class Intents:
    NextStep = Intent('next_step')
    RecipeFinished = Intent('recipe_finished')
    SaveFeedback = Intent('save_feedback')
    SelectRecipe = Intent('select_recipe')
    StopCooking = Intent('stop_cooking')
    TimerStopPlaying = Intent('timer_stop_playing')
    WaitingForTimer = Intent('waiting_for_timer')
    Unknown = Intent('unknown')


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station(is_tv_plugged_in=False),
])
@pytest.mark.experiments('uniproxy_use_server_time_from_client')
class TestRecipes(object):

    owners = ('abc:yandexdialogs2',)

    @pytest.mark.parametrize('recipe, inflected_recipe', [
        ('сырники', 'сырников'),
        ('грибной суп', 'грибного супа'),
    ])
    def test_activation(self, alice, recipe, inflected_recipe):
        response = alice(f'давай приготовим {recipe}')
        self.validate_analytics_info(response, Intents.SelectRecipe)
        assert inflected_recipe in response.text

    @pytest.mark.parametrize('recipe, inflected_recipe, expected_timers, max_steps', [
        (
            'шарлотку',
            'шарлотки',
            [
                {
                    'duration': 2400,
                    'listening_is_possible': True,
                },
            ],
            30,
        ),
    ])
    def test_walk_recipe(self, alice, recipe, inflected_recipe, expected_timers, max_steps):
        response = alice(f'давай приготовим {recipe}')
        self.validate_analytics_info(response)
        assert inflected_recipe in response.text

        recipe_finished = False
        actual_timers = []
        for i in range(max_steps):
            response = alice('дальше')
            self.validate_analytics_info(response)
            set_timer = self.filter_set_timer_directive(response.directives)
            if set_timer:
                actual_timers.append({
                    'duration': set_timer.payload.duration,
                    'listening_is_possible': set_timer.payload.listening_is_possible
                })
            if response.intent == Intents.WaitingForTimer:
                assert len(alice.device_state.Timers.ActiveTimers) > 0, 'Recipe is blocked on timer, but no timers are active'
                duration = min([timer.Duration for timer in alice.device_state.Timers.ActiveTimers]) + 1
                alice.skip(seconds=duration)
                response = alice('хватит')
                self.validate_analytics_info(response, [Intents.TimerStopPlaying, Intents.NextStep])
            elif response.intent in (Intents.RecipeFinished, Intents.SaveFeedback):
                assert actual_timers == expected_timers
                recipe_finished = True
                break
        assert recipe_finished, f'Recipe did not finish after {max_steps} steps. Possible loop detected.'
        response = alice('дальше')
        assert response.intent != scenario.ExternalSkillRecipes, 'Recipe scenario did not return irrelevant after recipe has finished'

    def test_stop_cooking(self, alice):
        response = alice('давай приготовим шарлотку')
        self.validate_analytics_info(response, Intents.SelectRecipe)
        response = alice('я больше не хочу готовить')
        self.validate_analytics_info(response, Intents.StopCooking)
        response = alice('дальше')
        assert response.intent != scenario.ExternalSkillRecipes, 'Recipe scenario did not return irrelevant after recipe was stopped by user'

    @pytest.mark.oauth(auth.YandexPlus)
    def test_next_step_with_music_playing(self, alice):
        response = alice('давай приготовим шарлотку')
        self.validate_analytics_info(response, Intents.SelectRecipe)

        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('следующий шаг')
        self.validate_analytics_info(response, Intents.NextStep)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_next_step_with_music_playing_doesnt_react_on_generic_command(self, alice):
        response = alice('давай приготовим шарлотку')
        self.validate_analytics_info(response, Intents.SelectRecipe)

        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('дальше')
        assert response.scenario != scenario.ExternalSkillRecipes, 'Recipe scenario should not answer to generic next step/track command with music playing'

    def validate_analytics_info(self, response, expected_intent=None):
        assert response.scenario == scenario.ExternalSkillRecipes
        assert response.intent is not None
        assert response.intent != Intents.Unknown
        if isinstance(expected_intent, collections.abc.Sequence) and not isinstance(expected_intent, str):
            assert any(response.intent == i for i in expected_intent)
        elif expected_intent is not None:
            assert response.intent == expected_intent

    def filter_set_timer_directive(self, response_directives):
        set_timer_directives = filter(lambda d: d.name == directives.names.SetTimerDirective, response_directives)
        # currently scenario can set at most one timer
        return next(set_timer_directives, None)

    @pytest.mark.parametrize('command', ['что приготовить', 'что приготовить на завтрак', 'какие ты знаешь рецепты'])
    def test_onboarding(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.ExternalSkillRecipes
        recipe_name = response.scenario_analytics_info.objects['recipe']['human_readable']

        response = alice(recipe_name)
        assert response.scenario == scenario.ExternalSkillRecipes
        assert response.scenario_analytics_info.objects['recipe']['human_readable'] == recipe_name

    @pytest.mark.parametrize('command_next', ['что еще', 'еще', 'дальше'])
    def test_onboarding_next(self, alice, command_next):
        response = alice('что приготовить на ужин')
        assert response.scenario == scenario.ExternalSkillRecipes
        recipe_name = response.scenario_analytics_info.object('human_readable')

        response = alice(command_next)
        assert response.scenario == scenario.ExternalSkillRecipes
        assert response.scenario_analytics_info.objects['recipe']['human_readable'] != recipe_name
        recipe_name = response.scenario_analytics_info.object('human_readable')

        response = alice(command_next)
        assert response.scenario == scenario.ExternalSkillRecipes
        assert response.scenario_analytics_info.objects['recipe']['human_readable'] != recipe_name
