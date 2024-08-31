import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .common import ExternalSkillIntents


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestThinPlayer(object):
    '''
    https://testpalm.yandex-team.ru/testcase/paskills-1135
    https://testpalm.yandex-team.ru/testcase/paskills-1136
    https://testpalm.yandex-team.ru/testcase/paskills-1137
    https://testpalm.yandex-team.ru/testcase/paskills-1143
    https://testpalm.yandex-team.ru/testcase/paskills-1149
    https://testpalm.yandex-team.ru/testcase/paskills-1151
    https://testpalm.yandex-team.ru/testcase/paskills-1174
    '''

    owners = ('kuptservol',)

    start_skill = 'запусти навык олег дулин'

    def test_paskills_1135(self, alice):
        response = alice('попроси олег дулин 50')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('0')
        assert response.text == 'добры дзень для тэсту'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('53')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('погода')
        assert response.scenario != scenario.Dialogovo
        assert response.intent.startswith(intent.GetWeather)

    def test_paskills_1136(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('попроси олег дулин 53')
        assert response.scenario == scenario.Dialogovo
        assert response.directives[2].name == directives.names.AudioPlayDirective

        response = alice('продолжи музыку')
        assert response.scenario != scenario.Dialogovo

    @pytest.mark.voice
    def test_paskills_1137(self, alice):
        response = alice('включи радио')
        assert response.directive.name == directives.names.RadioPlayDirective

        response = alice('попроси олег дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('продолжи радио')
        assert response.scenario != scenario.Dialogovo
        assert response.directive.name == directives.names.RadioPlayDirective

    def test_paskills_1143(self, alice):
        response = alice('попроси олег дулин 53')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('давай поиграем в города')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.scenario_analytics_info.object('name') == 'Города'

        response = alice('алиса, хватит')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

    def test_paskills_1149(self, alice):
        response = alice('попроси олег дулин 50')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('пожалуйста хватит')
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

        response = alice('привет')
        assert response.scenario != scenario.Dialogovo
        assert response.intent == intent.Hello

        response = alice('попроси олег дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Dialogovo
        assert response.directives[0].name == directives.names.AudioStopDirective

        response = alice('привет')
        assert response.scenario != scenario.Dialogovo
        assert response.intent == intent.Hello

    def test_paskills_1151(self, alice):
        response = alice(TestThinPlayer.start_skill)
        assert response.scenario == scenario.Dialogovo
        response = alice('53')
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('продолжи музыку')
        assert response.scenario != scenario.Dialogovo

        response = alice(TestThinPlayer.start_skill)
        assert response.scenario == scenario.Dialogovo
        response = alice('56')
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('продолжи радио')
        assert response.scenario != scenario.Dialogovo

        response = alice(TestThinPlayer.start_skill)
        assert response.scenario == scenario.Dialogovo
        response = alice('53')
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('продолжи видео')
        assert response.scenario != scenario.Dialogovo

    def test_paskills_1174(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('включи робот пылесос')
        assert response.scenario != scenario.Dialogovo

    def test_player_modality_while_playing(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('число 13')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 13'

    def test_player_modality_on_stopped(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Dialogovo
        assert response.directives[0].name == directives.names.AudioStopDirective

        response = alice('число 13')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 13'

    @pytest.mark.xfail(reason='PASKILLS-6116')
    def test_player_modality_on_stopped_can_resume_multiple_times_inside_timeout(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Dialogovo
        assert response.directives[0].name == directives.names.AudioStopDirective

        response = alice('число 14')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 14'

        response = alice('число 14')
        assert response.scenario == scenario.Dialogovo

    def test_player_modality_while_playing_can_resume_multiple_times_on_end_session_answer(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('число 14')
        assert response.scenario == scenario.Dialogovo

        assert response.text == 'Интенты: 14'
        response = alice('число 14')
        assert response.scenario == scenario.Dialogovo

    @pytest.mark.xfail(reason='PASKILLS-6116')
    def test_player_modality_on_paused_can_resume_multiple_times_on_end_session_answer(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Dialogovo
        assert response.directives[0].name == directives.names.AudioStopDirective

        response = alice('число 14')
        assert response.scenario == scenario.Dialogovo

        assert response.text == 'Интенты: 14'
        response = alice('число 14')
        assert response.scenario == scenario.Dialogovo

    def test_player_modality_on_stopped_refused_after_timeout(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Dialogovo
        assert response.directives[0].name == directives.names.AudioStopDirective

        response = alice('число 14')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 14'
        alice.skip(seconds=60)

        response = alice('число 14')
        assert response.scenario != scenario.Dialogovo

    def test_player_modality_on_stopped_resume_play(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Dialogovo
        assert response.directives[0].name == directives.names.AudioStopDirective

        response = alice('число 15')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        alice.skip(seconds=60)

        response = alice('число 14')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 14'

    def test_player_modality_while_playing_with_other_scenarios_in_the_middle(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('какая погода')
        assert response.scenario != scenario.Dialogovo

        response = alice('сколько времени')
        assert response.scenario != scenario.Dialogovo

        response = alice('число 13')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 13'

        response = alice('56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('число 13')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 13'

    def test_player_modality_on_stopped_with_other_scenarios_in_the_middle(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Dialogovo
        assert response.directives[0].name == directives.names.AudioStopDirective

        response = alice('какая погода')
        assert response.scenario != scenario.Dialogovo

        response = alice('сколько времени')
        assert response.scenario != scenario.Dialogovo

        response = alice('число 13')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 13'

        response = alice('56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('число 13')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 13'

    def test_player_modality_with_exit(self, alice):
        response = alice('попроси Олег Дулин 56')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('число 13')
        assert response.scenario == scenario.Dialogovo
        assert response.text == 'Интенты: 13'

        response = alice('Алиса, хватит')
        assert response.scenario == scenario.Dialogovo

        response = alice('число 13')
        assert response.scenario != scenario.Dialogovo

    def test_player_modality_iot(self, alice):
        response = alice('попроси Олег Дулин 51')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('включи свет')
        assert response.scenario == scenario.IoT


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.station,
])
class TestThinPlayerStation(object):
    '''
    https://testpalm.yandex-team.ru/testcase/paskills-1139
    https://testpalm.yandex-team.ru/testcase/paskills-1152
    '''

    owners = ('kuptservol',)

    def test_paskills_1139(self, alice):
        response = alice('включи достать ножи')
        assert response.directive.name == directives.names.VideoPlayDirective

        response = alice('попроси олег дулин 53')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('продолжить смотреть')
        assert response.scenario != scenario.Dialogovo

    def test_paskills_1152(self, alice):
        response = alice('включи рик и морти')
        assert response.directive.name == directives.names.VideoPlayDirective

        response = alice('попроси Олег Дулин 50')
        assert response.scenario == scenario.Dialogovo
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('хватит')
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

        response = alice('домой')
        assert response.directives[0].name == directives.names.MordoviaShowDirective
        assert response.directives[-1].name == directives.names.ClearQueueDirective

        response = alice('продолжить смотреть')
        assert response.scenario != scenario.Dialogovo
