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
@pytest.mark.parametrize('command', ['давай сыграем в', 'поиграем в', 'запусти навык'])
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestSkillGames(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-457
        https://testpalm.yandex-team.ru/testcase/alice-1328
    """

    owners = ('abc:yandexdialogs2', )

    @pytest.mark.parametrize('skill', ['Угадай песню'])
    def test_alice_457(self, alice, command, skill):
        response = alice(f'{command} {skill}')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.scenario_analytics_info.object('name') == skill

        response = alice('в лесу родилась елочка')
        assert 'В лесу родилась ёлочка' in response.text
        assert 'В лесу она росла' in response.text

        assert response.suggest('Другой вариант')
        assert response.suggest('Новая строчка')
        assert response.suggest('Правила')
        if surface.is_smart_speaker(alice) or surface.is_auto(alice):
            assert response.suggest('Закончить ❌')

        response = alice('группа крови на рукаве')
        assert 'Группа крови' in response.text
        assert 'Мой порядковый номер' in response.text

        assert response.suggest('Другой вариант')
        assert response.suggest('Новая строчка')
        assert response.suggest('Правила')
        if surface.is_smart_speaker(alice) or surface.is_auto(alice):
            assert response.suggest('Закончить ❌')

        response = alice('алиса, хватит')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

        response = alice('привет')
        assert response.intent == intent.Hello

    @pytest.mark.parametrize('skill', ['Верю — не верю'])
    def test_alice_1328(self, alice, command, skill):
        response = alice(f'{command} {skill}')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.scenario_analytics_info.object('name') == skill

        response = alice('да')
        answer_text = response.text

        assert response.suggest('Верю')
        assert response.suggest('Не верю')
        if surface.is_smart_speaker(alice) or surface.is_auto(alice):
            assert response.suggest('Закончить ❌')

        response = alice('верю')
        assert response.text != answer_text
        answer_text = response.text

        assert response.suggest('Верю')
        assert response.suggest('Не верю')
        if surface.is_smart_speaker(alice) or surface.is_auto(alice):
            assert response.suggest('Закончить ❌')

        response = alice('не верю')
        assert response.text != answer_text
        answer_text = response.text

        assert response.suggest('Верю')
        assert response.suggest('Не верю')
        if surface.is_smart_speaker(alice) or surface.is_auto(alice):
            assert response.suggest('Закончить ❌')

        response = alice('верю')
        assert response.text != answer_text
        answer_text = response.text

        response = alice('не верю')
        assert response.text != answer_text

        response = alice('верю')
        assert 'из' in response.text
        assert '?' in response.text

        assert response.suggest('Да')
        if surface.is_smart_speaker(alice) or surface.is_auto(alice):
            assert response.suggest('Закончить ❌')

        response = alice('алиса, хватит')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

        response = alice('привет')
        assert response.intent == intent.Hello
