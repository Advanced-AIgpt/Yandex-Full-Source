import random

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .common import ExternalSkillIntents


@pytest.mark.parametrize('surface', [surface.watch])
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestPalmExternalSkillsWatch(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-12
    https://testpalm.yandex-team.ru/testcase/alice-46
    https://testpalm.yandex-team.ru/testcase/alice-47
    https://testpalm.yandex-team.ru/testcase/alice-49
    https://testpalm.yandex-team.ru/testcase/alice-50
    https://testpalm.yandex-team.ru/testcase/alice-51
    https://testpalm.yandex-team.ru/testcase/alice-52
    https://testpalm.yandex-team.ru/testcase/alice-1869
    https://testpalm.yandex-team.ru/testcase/alice-1870
    https://testpalm.yandex-team.ru/testcase/alice-1871
    https://testpalm.yandex-team.ru/testcase/alice-1872
    https://testpalm.yandex-team.ru/testcase/alice-2140
    https://testpalm.yandex-team.ru/testcase/alice-2608
    https://testpalm.yandex-team.ru/testcase/paskills-1042
    https://testpalm.yandex-team.ru/testcase/paskills-1084
    """

    owners = ('abc:yandexdialogs2',)

    def test_alice_12(self, alice):
        expected_phrases = [
            'Можем сыграть в загадки или в города. Хочешь узнать ещё?.',
            'В «Найди лишнее» или «Что было раньше». Скажи да, если нужна другая игра.',
            'Могу сыграть в «Угадай животное» или «Угадай число». Хочешь что-нибудь другое?',
            'Можем сыграть в загадки или в города. Или вообще послушать сказку! Хочешь узнать ещё?',
        ]

        response = alice('Во что ты умеешь играть?')

        assert response.text in expected_phrases
        assert response.intent == intent.Onboarding
        first_reply = response.text

        response = alice('Да')

        assert response.text in expected_phrases
        assert response.intent == intent.OnboardingNext
        second_reply = response.text
        assert first_reply != second_reply

        response = alice('Да')

        assert response.text in expected_phrases
        assert response.intent == intent.OnboardingNext
        third_reply = response.text
        assert first_reply != third_reply
        assert second_reply != third_reply

        response = alice('Нет')
        assert response.intent == intent.OnboardingCancel

    @pytest.mark.parametrize('command', [
        'Давай сыграем в «Угадай животное»'
        'Давай сыграем в «Зоологию»',
        'Давай сыграем в «Что было раньше»',
        'Давай сыграем в «Угадай число»',
        'Давай сыграем в загадки',
        'Давай сыграем в города',
    ])
    def test_alice_46(self, alice, command):
        response = alice(command)

        assert response.intent == ExternalSkillIntents.Request
        assert response.text
        assert ('Алиса, хватит' in response.text) or ('скажите «Хватит»' in response.text)

    def test_alice_46_tale(self, alice):
        response = alice('Расскажи сказку')

        assert response.intent == ExternalSkillIntents.Request
        assert response.text

        suggests = [s.title for s in response.suggests]
        assert len(suggests) > 0

        response = alice(suggests[random.randint(0, len(suggests) - 1)])

        assert response.intent == ExternalSkillIntents.Request
        assert response.text
        assert 'Алиса, хватит' in response.text

    def test_alice_47(self, alice):
        response = alice('Запусти навык "Сбербанк"')

        assert response.scenario == scenario.Dialogovo
        assert response.text in [
            'Я бы и рада помочь, но тут голосом не обойдёшься.',
            'Я это, конечно, умею. Но в другом приложении.',
        ]

    def test_alice_49(self, alice):
        response = alice('Расскажи про этот день в истории')
        assert response.scenario == scenario.Dialogovo
        assert response.scenario_analytics_info.object('name') == 'День в истории'
        reply = response.text

        response = alice('еще событие')
        assert response.scenario == scenario.Dialogovo
        assert response.text != reply

        response = alice('Алиса, хватит')
        assert response.intent == ExternalSkillIntents.Deactivate
        assert response.text == 'Отлично, будет скучно — обращайтесь.'

        response = alice('еще событие')
        assert response.scenario != scenario.Dialogovo

    @pytest.mark.parametrize('command', [
        'Расскажи анекдот',
        'Расскажи шутку',
    ])
    def test_alice_50(self, alice, command):
        response = alice(command)

        assert response.intent == intent.TellMeAJoke
        assert response.text
        first_reply = response.text

        response = alice(command)

        assert response.intent == intent.TellMeAJoke
        assert response.text
        assert response.text != first_reply

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-9639')
    @pytest.mark.parametrize('command', [
        'Расскажи сказку',
        'Хочу сказку',
    ])
    def test_alice_51(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request

        response = alice('про репку')
        assert 'Если надоест, скажите: «хватит». «Репка». Читает Александра Иванова' in response.text
        assert response.intent == ExternalSkillIntents.Request

        response = alice('хватит')
        assert response.intent == ExternalSkillIntents.Deactivate

        response = alice('расскажи сказку про красную шапочку')
        assert 'Если надоест, скажите: «хватит». «Красная шапочка». Читает Ольга Свирина' in response.text
        assert response.intent == ExternalSkillIntents.Request

    def test_alice_52(self, alice):
        response = alice('Давай играть в загадки')
        assert response.scenario == scenario.Dialogovo
        assert response.scenario_analytics_info.object('name') == 'Загадки'
        reply = response.text

        response = alice('муха')
        assert response.scenario == scenario.Dialogovo
        assert response.text != reply

        response = alice('муха')
        assert response.scenario == scenario.Dialogovo
        assert response.text != reply

        response = alice('Алиса, угадай мою загадку')
        assert response.scenario == scenario.Dialogovo

        response = alice('Сто одежек и все без застежек')
        assert response.scenario == scenario.Dialogovo
        assert 'капуста' in response.text

    @pytest.mark.parametrize('command', [
        'Давай играть в города Москва',
        'Давай играть в угадай животное',
        'Алиса, давай сыграем в Найди лишнее',
        'Алиса, давай сыграем в Что было раньше',
        'Алиса, расскажи про этот день в истории',
        'Давай сыграем в загадки',
        'Расскажи сказку',
        'Давай играть в угадай число',
        'Алиса, давай сыграем в Зоологию',
    ])
    def test_alice_1869_1870_1871_1872(self, alice, command):
        response = alice(command)
        assert response.intent == ExternalSkillIntents.Request
        assert response.text

        response = alice('хватит')
        assert response.intent == ExternalSkillIntents.Deactivate
        assert not response.suggests

    def test_paskills_1084_alice_2140(self, alice):
        response = alice('запусти навык олег дулин')
        assert response.scenario == scenario.Dialogovo

        response = alice('2 секунды')
        assert response.text == '2 секунды'
        assert response.intent == ExternalSkillIntents.Request

        response = alice('3 секунды')
        assert response.text == '3 секунды'
        assert response.intent == ExternalSkillIntents.Request


@pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
class TestPalmExternalSkillsNavi(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1529
    https://testpalm.yandex-team.ru/testcase/alice-2228
    """

    owners = ('abc:yandexdialogs2',)

    @pytest.mark.parametrize('command', [
        'Алиса, давай обсудим секреты блогеров',
        'Алиса, расскажи секреты блогеров',
        'Включи секреты блогеров',
    ])
    def test_alice_1529(self, alice, command):
        response = alice(command)

        assert 'Чей секрет вы хотите узнать? Например, вы можете спросить,' in response.text
        assert response.intent == ExternalSkillIntents.Request
        assert response.suggests
        assert response.suggest('Закончить ❌')

        response = alice('Алиса, хватит')
        assert response.intent == ExternalSkillIntents.Deactivate
        assert not response.suggests

    def test_alice_2228(self, alice):
        response = alice('Запусти навык олег дулин')
        assert response.intent == ExternalSkillIntents.Request
        response = alice('Авторизация')
        assert response.text == \
            'Навык запрашивает авторизацию, но к сожалению, на этом устройстве такая возможность не поддерживается'
        assert response.intent == ExternalSkillIntents.Request


@pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-9639')
@pytest.mark.parametrize('surface', [surface.automotive])
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestPalmExternalSkillsAutomotive(object):
    """
    https://testpalm.yandex-team.ru/testcase/paskills-1042
    https://testpalm.yandex-team.ru/testcase/paskills-1397
    """

    owners = ('abc:yandexdialogs2',)

    @pytest.mark.parametrize('command', [
        'Расскажи сказку',
        'Хочу сказку',
    ])
    def test_paskills_1042(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request
        assert response.suggests
        assert response.suggest('Закончить ❌')

        response = alice('про репку')
        assert response.intent == ExternalSkillIntents.Request
        assert 'Если надоест, скажите: «хватит». «Репка». Читает Александра Иванова' in response.text

        response = alice('хватит')
        assert response.intent == ExternalSkillIntents.Deactivate
        assert not response.suggests

        response = alice('расскажи сказку про красную шапочку')
        assert response.intent == ExternalSkillIntents.Request
        assert 'Если надоест, скажите: «хватит». «Красная шапочка». Читает Ольга Свирина' in response.text

    @pytest.mark.parametrize('command', [
        'Расскажи анекдот',
        'Расскажи шутку',
    ])
    def test_alice_1397(self, alice, command):
        response = alice(command)

        assert response.intent == intent.TellMeAJoke
        assert response.text

        response = alice('Расскажи сказку')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request

        response = alice('про репку')
        assert response.intent == ExternalSkillIntents.Request
        assert 'Если надоест, скажите: «Алиса, хватит». «Репка». Читает Александра Иванова' in response.text

        response = alice('хватит')
        assert response.intent == ExternalSkillIntents.Deactivate

        response = alice('расскажи сказку про красную шапочку')
        assert response.intent == ExternalSkillIntents.Request
        assert 'Если надоест, скажите: «Алиса, хватит». «Красная шапочка». Читает Ольга Свирина' in response.text


@pytest.mark.parametrize('surface', [surface.navi])
@pytest.mark.parametrize('', [
    pytest.param(id='http_adapter'),
    pytest.param(id='apphost', marks=pytest.mark.experiments('use_app_host_pure_Dialogovo_scenario'))
])
class TestPalmExternalSkillsLetsPlay(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2754
    """

    owners = ('abc:yandexdialogs2',)

    def test_alice_2754(self, alice):
        response = alice('давай поиграем')
        assert response.intent == intent.GamesOnboarding
        assert len(response.suggests) > 0
        assert any(text in response.text for text in ['«Давай сыграем в', 'Скажите мне «Давай', 'Например, скажите «'])
        response = alice('давай сыграем в города')
        assert response.scenario == scenario.Dialogovo
