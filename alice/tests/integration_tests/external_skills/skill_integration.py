import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .common import ExternalSkillIntents


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    pytest.param(
        surface.loudspeaker,
        marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-12'),
    ),
    surface.searchapp,
    surface.smart_tv,
    surface.station,
    surface.yabro_win,
])
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestSkillIntegration(object):
    """
    https://testpalm.yandex-team.ru/testcase/paskills-1132
    """

    owners = ('abc:yandexdialogs2',)

    def test_paskills_1132(self, alice):
        response = alice('запусти навык')
        assert response.scenario != scenario.Dialogovo
        assert response.intent != ExternalSkillIntents.Request

        response = alice('запусти навык несуществующий навык')
        assert response.scenario != scenario.Dialogovo
        assert response.intent != ExternalSkillIntents.Request

        response = alice('запусти навык угадай животное')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request


@pytest.mark.oauth(auth.Skills)
@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    surface.loudspeaker,
    surface.searchapp,
    surface.smart_tv,
    surface.station,
    surface.yabro_win,
])
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestSkillIntegrationAuth(object):
    """
    https://testpalm.yandex-team.ru/testcase/paskills-1128
    https://testpalm.yandex-team.ru/testcase/paskills-1150
    """

    owners = ('abc:yandexdialogs2', )

    def test_paskills_1128(self, alice):
        response = alice('запусти навык есть доступ')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.scenario_analytics_info.object('name') == 'есть доступ'

    def test_paskills_1150(self, alice):
        response = alice('запусти навык нет доступа')
        assert response.scenario != scenario.Dialogovo

    def test_save_session_state(self, alice):
        session_state = 46

        response = alice('запусти навык сохрани сессию')
        assert response.scenario == scenario.Dialogovo

        response = alice('0')
        assert response.scenario == scenario.Dialogovo
        assert response.text == f'состояние сессии - {session_state}'

        response = alice('0')
        assert response.scenario == scenario.Dialogovo
        assert response.text == f'состояние сессии - {session_state}'


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.smart_tv,
    surface.station,
])
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestSkillIntegrationSmartSpeaker(object):

    owners = ('abc:yandexdialogs2', )

    def test_allow_run_skill_inside_skill(self, alice):
        response = alice('запусти навык можно запустить навык внутри')
        assert response.scenario == scenario.Dialogovo
        assert response.scenario_analytics_info.object('name') == 'можно запустить навык внутри'

        response = alice('запусти навык города')
        assert response.scenario == scenario.Dialogovo
        assert response.scenario_analytics_info.object('name') == 'Города'

        response = alice('алиса, хватит')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Deactivate

    def test_not_allow_run_skill_inside_skill(self, alice):
        response = alice('запусти навык слова')
        assert response.scenario == scenario.Dialogovo
        assert response.scenario_analytics_info.object('name') == 'Слова'

        response = alice('запусти навык города')
        assert response.scenario == scenario.Dialogovo
        assert response.scenario_analytics_info.object('name') == 'Слова'


@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestSkillIntegrationTV(object):

    owners = ('pazus', )

    def test_authorization_tv(self, alice):
        response = alice('попроси олег дулин авторизация')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.text == \
            'Навык запрашивает авторизацию, но к сожалению, на этом устройстве такая возможность не поддерживается'

    def test_activation_tv(self, alice):
        response = alice('попроси олег дулин активируй игрушку')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.text == \
            'Навык требует музыкальную активацию, но к сожалению, на этом устройстве такая  возможность не поддерживается'

    def test_buy_tv(self, alice):
        response = alice('попроси олег дулин закажи пиццу')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.text == \
            'Простите, но здесь вы не сможете оплатить покупку, повторите попытку в приложении Яндекса.'
