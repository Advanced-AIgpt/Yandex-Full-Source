import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from external_skills.common import ExternalSkillIntents


def _check_suggests(response):
    suggests = {s.title for s in response.suggests}
    assert 'Покажи другую' in suggests
    assert 'Включи игру' in suggests


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(f'mm_enable_protocol_scenario={scenario.GameSuggest}')
@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.station,
])
class TestGameSuggest(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-2620
    '''

    owners = ('dan-anastasev',)

    def test_game_suggest_scenario(self, alice):
        response = alice('Посоветуй в какую игру мне поиграть')
        assert response.scenario == scenario.GameSuggest
        _check_suggests(response)

        response = alice('Хватит')
        assert response.scenario not in [scenario.GameSuggest, scenario.Dialogovo]

        response = alice('Посоветуй в какую игру мне поиграть')
        assert response.scenario == scenario.GameSuggest
        _check_suggests(response)

        response = alice('Покажи другую')
        assert response.scenario == scenario.GameSuggest
        _check_suggests(response)

        response = alice('Включи игру')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request

    @pytest.mark.parametrize('start', ['Давай'])
    def test_game_suggest_other_phrases(self, alice, start):
        response = alice('Посоветуй в какую игру мне поиграть')
        assert response.scenario == scenario.GameSuggest
        _check_suggests(response)

        response = alice(start)
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
