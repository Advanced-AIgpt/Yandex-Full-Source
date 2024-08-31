import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .common import ExternalSkillIntents


_cities = {
    'а': 'Арзамас',
    'е': 'Ереван',
    'к': 'Кунгур',
    'н': 'Норильск',
    'о': 'Орск',
    'р': 'Рим',
    'с': 'Саратов',
}


@pytest.mark.parametrize('surface', surface.actual_surfaces)
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestPalm(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-467
    https://testpalm.yandex-team.ru/testcase/alice-1134
    https://testpalm.yandex-team.ru/testcase/alice-1399
    https://testpalm.yandex-team.ru/testcase/alice-1868
    https://testpalm.yandex-team.ru/testcase/alice-2750
    '''

    owners = ('abc:yandexdialogs2',)

    @pytest.mark.parametrize('command', ['давай сыграем в города', 'играть в города', 'запусти навык города'])
    def test(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.scenario_analytics_info.object('name') == 'Города'

        if surface.is_smart_speaker(alice):
            assert response.text.startswith('Запускаю навык «Города».')
            assert 'Для того, чтобы закончить, скажите «Хватит».' in response.text

        assert 'Вы называете город, я говорю город на последнюю букву — и так далее.' in response.text
        assert 'Только учтите — мягкий знак и буква «ы» не считаются.' in response.text
        assert 'Если вам нужна будет моя помощь, скажите «дай подсказку».' in response.text

        assert response.suggest('Подсказка')
        assert response.suggest('На какую букву ходить')
        assert response.suggest('Правила')
        if surface.is_smart_speaker(alice) or surface.is_auto(alice):
            assert response.suggest('Закончить ❌')

        response = alice('Быдгощ')
        assert response.scenario == scenario.Dialogovo
        assert response.text.startswith('Щ')
        city = response.text

        response = alice('Быдгощ')
        assert response.scenario == scenario.Dialogovo
        assert response.text == city

        response = alice('На какую букву ходить')
        assert response.scenario == scenario.Dialogovo
        assert f'на букву «{city[-1]}»' in response.text

        city = _cities[city[-1]]
        response = alice(city)
        assert response.scenario == scenario.Dialogovo

        # TODO(mihajlova) add @pytest.mark.experiments('no_div_card')
        # after https://st.yandex-team.ru/ALICE-5606
        if response.text_card:
            assert response.text.startswith(city[-1].upper())
        else:
            assert response.div_card

        response = alice('Цзяи')
        assert response.scenario == scenario.Dialogovo
        assert response.text.startswith((
            'Вы назвали город не на ту букву', 'Этот город не подходит', 'Нет, так нельзя',
        ))

        response = alice('подсказка')
        assert response.scenario == scenario.Dialogovo
        assert response.text

        response = alice('алиса, хватит')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

        response = alice('привет')
        assert response.intent == intent.Hello
