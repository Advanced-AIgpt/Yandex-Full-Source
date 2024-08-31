import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .common import ExternalSkillIntents


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
class TestUniversalSkill(object):
    '''
    https://testpalm.yandex-team.ru/testcase/paskills-1015
    https://testpalm.yandex-team.ru/testcase/paskills-1018
    https://testpalm.yandex-team.ru/testcase/paskills-1019
    https://testpalm.yandex-team.ru/testcase/paskills-1020
    https://testpalm.yandex-team.ru/testcase/paskills-1021
    https://testpalm.yandex-team.ru/testcase/paskills-1052
    https://testpalm.yandex-team.ru/testcase/paskills-1053
    https://testpalm.yandex-team.ru/testcase/paskills-1058
    https://testpalm.yandex-team.ru/testcase/paskills-1060
    https://testpalm.yandex-team.ru/testcase/paskills-1061
    https://testpalm.yandex-team.ru/testcase/paskills-1062
    https://testpalm.yandex-team.ru/testcase/paskills-1063
    https://testpalm.yandex-team.ru/testcase/paskills-1071
    https://testpalm.yandex-team.ru/testcase/paskills-1080
    https://testpalm.yandex-team.ru/testcase/paskills-1084
    https://testpalm.yandex-team.ru/testcase/paskills-1107
    https://testpalm.yandex-team.ru/testcase/paskills-1108
    https://testpalm.yandex-team.ru/testcase/paskills-1111
    https://testpalm.yandex-team.ru/testcase/paskills-1161
    https://testpalm.yandex-team.ru/testcase/paskills-1073
    https://testpalm.yandex-team.ru/testcase/alice-2140
    '''

    owners = ('abc:yandexdialogs2',)

    start_command = 'запусти навык олег дулин'

    @pytest.mark.parametrize('command', ['давай сыграем в', 'поиграем в', 'запусти навык'])
    @pytest.mark.parametrize('skill', ['олег дулин', 'один день ивана', 'еще один диалог облака', 'проверка нативный'])
    def test_paskills_1060_1061_1062_1080_1107(self, alice, command, skill):
        response = alice(f'{command} {skill}')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.scenario_analytics_info.object('name') == skill
        if surface.is_smart_speaker(alice):
            assert 'Запускаю навык' in response.text

    @pytest.mark.parametrize('command', ['давай сыграем в', 'поиграем в', 'запусти навык'])
    @pytest.mark.parametrize('dssm', ['угадай мелодию', 'быстрее высокий сильнее', 'что было ранее'])
    def test_paskills_1058(self, alice, command, dssm):
        response = alice(f'{command} {dssm}')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        if surface.is_smart_speaker(alice):
            assert 'Запускаю навык' in response.text

    @pytest.mark.parametrize('command', ['хватит', 'алиса хватит', 'яндекс хватит'])
    def test_paskills_1108(self, alice, command):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice(command)
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

        response = alice('привет')
        assert response.intent == intent.Hello

    def test_paskills_1111(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('0')
        assert response.text == 'добры дзень для тэсту'
        assert response.intent == ExternalSkillIntents.Request

    def test_paskills_1015(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('1')
        assert response.text == 'отсутствует session'
        assert response.intent == ExternalSkillIntents.Request

    def test_paskills_1018(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('4')
        assert response.text == 'отсутствует session_id'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('400')
        session_id = response.text
        assert response.scenario == scenario.Dialogovo

        response = alice('400')
        assert response.text == session_id
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request

    def test_paskills_1019(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('5')
        assert response.text == 'отсутствует message_id'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('500')
        message_id = int(response.text)
        assert response.scenario == scenario.Dialogovo

        response = alice('500')
        assert int(response.text) == message_id + 1
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request

    def test_paskills_1020(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('6')
        assert response.text == 'отсутствует user_id'
        assert response.intent == ExternalSkillIntents.Request

    def test_paskills_1021_1189(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('7')
        assert response.text == 'отсутствует end_session'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('400')
        assert response.scenario == scenario.Dialogovo
        session_id = response.text

        response = alice('44')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.text == 'end_session = true'

        response = alice('привет')
        assert response.intent == intent.Hello

        response = alice('попроси олег дулин 500')
        assert response.scenario == scenario.Dialogovo
        assert response.text == '0'

        response = alice('400')
        assert response.scenario == scenario.Dialogovo
        assert response.text != session_id

    def test_paskills_1052(self, alice):
        session_state = 0

        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('46')
        assert response.text == f'состояние сессии - {session_state}'
        assert response.scenario == scenario.Dialogovo
        session_state += 1

        response = alice('46')
        assert response.text == f'состояние сессии - {session_state}'
        assert response.scenario == scenario.Dialogovo

        response = alice('0')
        assert response.scenario == scenario.Dialogovo
        session_state = 0

        response = alice('46')
        assert response.text == f'состояние сессии - {session_state}'
        assert response.scenario == scenario.Dialogovo

    def test_paskills_1053(self, alice):
        user_state = 0

        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('47')
        assert response.text == f'состояние пользователя - {user_state}'
        assert response.scenario == scenario.Dialogovo

        response = alice('47')
        assert response.text == f'состояние пользователя - {user_state}'
        assert response.scenario == scenario.Dialogovo

    def test_paskills_1063(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo

        response = alice('эй включи свет в ванной')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.text == 'Интенты: свет,эй,в ванной'

        response = alice('число 13')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.text == 'Интенты: 13'

    def test_paskills_1071(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo

        response = alice('19')
        assert response.text == 'Получи смайлик 🙂🙂🙂'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('20')
        assert response.text == 'M'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('🙂')
        assert response.text == 'привет я тестовый навык\n'
        assert response.intent == ExternalSkillIntents.Request

    def test_paskills_1084_alice_2140(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo

        response = alice('2 секунды')
        assert response.text == '2 секунды'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('3 секунды')
        assert response.text == '3 секунды'
        assert response.intent == ExternalSkillIntents.Request

    def test_paskills_1161(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo

        response = alice('мат')
        assert response.text == '<censored>'
        assert response.intent == ExternalSkillIntents.Request

    def test_paskills_1173(self, alice):
        response = alice('попроси олег дулин сложный числовой запрос')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.text == 'сложный числовой запрос'


@pytest.mark.oauth(auth.Skills)
@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    surface.loudspeaker,
    surface.searchapp,
    surface.station,
    surface.yabro_win,
    surface.smart_tv,
])
class TestUniversalSkillAuth(object):
    '''
    https://testpalm.yandex-team.ru/testcase/paskills-1047
    https://testpalm.yandex-team.ru/testcase/paskills-1067
    '''

    owners = ('abc:yandexdialogs2',)

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-1031')
    def test_paskills_1047(self, alice):
        user_state = 0

        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('49')
        assert response.text == 'Удаляю состояние пользователя'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('47')
        assert response.text == f'состояние пользователя - {user_state}'
        assert response.scenario == scenario.Dialogovo
        user_state += 1

        response = alice('47')
        assert response.text == f'состояние пользователя - {user_state}'
        assert response.scenario == scenario.Dialogovo

    def test_paskills_1067(self, alice):
        session_state = 0

        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('46')
        assert response.text == f'состояние сессии - {session_state}'
        assert response.scenario == scenario.Dialogovo
        session_state += 1

        response = alice('46')
        assert response.text == f'состояние сессии - {session_state}'
        assert response.scenario == scenario.Dialogovo

        response = alice('0')
        assert response.scenario == scenario.Dialogovo
        session_state = 0

        response = alice('46')
        assert response.text == f'состояние сессии - {session_state}'
        assert response.scenario == scenario.Dialogovo


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
])
class TestUniversalSkillSmartSpeakers(object):
    '''
    https://testpalm.yandex-team.ru/testcase/paskills-1017
    https://testpalm.yandex-team.ru/testcase/paskills-1031
    '''

    owners = ('abc:yandexdialogs2',)

    def test_paskills_1017(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('2')
        assert response.text == 'Извините, навык не отвечает'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('привет')
        assert response.intent == intent.Hello

        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('3')
        assert response.text == 'Извините, навык не отвечает'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('привет')
        assert response.intent == intent.Hello

    def test_paskills_1031(self, alice):
        response = alice('запусти навык проверка с экраном')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.text == 'Я это, конечно, умею. Но в другом приложении.'


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    surface.searchapp,
    surface.yabro_win,
])
class TestUniversalSkillNotSmartSpeakers(object):
    '''
    https://testpalm.yandex-team.ru/testcase/paskills-973
    '''

    owners = ('abc:yandexdialogs2',)

    def test_paskills_973(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('2')
        assert response.text == 'Извините, навык не отвечает'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('привет')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request

        response = alice('3')
        assert response.text == 'Извините, навык не отвечает'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('привет')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request


@pytest.mark.xfail(reason='https://st.yandex-team.ru/PASKILLS-7197')
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.smart_tv,
    surface.station,
])
class TestUniversalSkillTimeoutsSmartSpeakers(object):

    owners = ('svintenok',)

    def test_paskills_first_timeout(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('4 секунды')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Извините, навык не отвечает'
        response = alice('0')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'добры дзень для тэсту'

    def test_paskills_two_timeouts_in_row(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('4 секунды')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Извините, навык не отвечает'
        response = alice('4 секунды')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Извините, навык не отвечает'
        response = alice('привет')
        assert response.intent == intent.Hello
        assert response.scenario != scenario.Dialogovo

    def test_paskills_timeout_after_success_request(self, alice):
        response = alice(TestUniversalSkill.start_command)
        assert response.scenario == scenario.Dialogovo
        response = alice('4 секунды')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Извините, навык не отвечает'
        response = alice('0')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'добры дзень для тэсту'
        response = alice('4 секунды')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Извините, навык не отвечает'
        response = alice('0')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'добры дзень для тэсту'
