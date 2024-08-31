import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from external_skills.common import ExternalSkillIntents


class TestStop(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-2627
    """

    owners = ('nkodosov',)

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.loudspeaker,
        surface.navi,
        surface.searchapp,
        surface.watch,
        surface.yabro_win,
        surface.legatus,
    ])
    @pytest.mark.parametrize('stop_command', ['хватит', 'Алиса хватит'])
    def test_stop_skill(self, alice, stop_command):
        response = alice('запусти навык города')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.scenario_analytics_info.object('name') == 'Города'

        response = alice(stop_command)
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.loudspeaker,
        surface.navi,
        surface.searchapp,
        surface.station,
        surface.watch,
        surface.yabro_win,
        surface.legatus,
    ])
    @pytest.mark.parametrize('stop_command', ['хватит', 'Алиса хватит', 'хватит болтать'])
    def test_stop_gc(self, alice, stop_command):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert response.scenario_analytics_info.object('gc_response_info')['gc_intent'] == intent.PureGeneralConversation

        response = alice(stop_command)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.PureGeneralConversationDeactivation


@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.oauth(auth.YandexPlus)
class TestStopLegatus(object):

    owners = ('amullanurov',)

    @pytest.mark.xfail(reason='waiting for https://st.yandex-team.ru/MEDIAALICE-252')
    def test_stop(self, alice):
        response = alice('поставь на паузу')
        assert response.scenario == scenario.Commands
        assert response.text in [
            'Я еще не научилась этому. Давно собираюсь, но все времени нет.',
            'Я пока это не умею.',
            'Я еще не умею это.',
            'Я не могу пока, но скоро научусь.',
            'Меня пока не научили этому.',
            'Когда-нибудь я смогу это сделать, но не сейчас.',
            'Надеюсь, я скоро смогу это делать. Но пока нет.',
            'Я не знаю, как это сделать. Извините.',
            'Так делать я еще не умею.',
            'Программист Алексей обещал это вскоре запрограммировать. Но он мне много чего обещал.',
            'К сожалению, этого я пока не умею. Но я быстро учусь.',
        ]
