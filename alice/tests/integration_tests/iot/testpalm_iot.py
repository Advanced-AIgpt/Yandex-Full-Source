import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from external_skills.common import ExternalSkillIntents
from iot.common import is_iot, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


def _assert_its_not_a_city(text):
    assert text.startswith((
        'Это, похоже, не город',
        'Как-то не очень похоже на город',
        'Не уверена, что это город',
        'Кажется, это не город',
        'Или я вас не расслышала, или это не город',
    ))


@pytest.mark.oauth(auth.Smarthome)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.navi,
    surface.searchapp,
    surface.station,
    surface.yabro_win,
])
class TestPalmIotAuthorization(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1939
        https://testpalm.yandex-team.ru/testcase/alice-2030
        https://testpalm.yandex-team.ru/testcase/alice-2031
        https://testpalm.yandex-team.ru/testcase/alice-2033
        https://testpalm.yandex-team.ru/testcase/alice-2035
        https://testpalm.yandex-team.ru/testcase/alice-2726
    """

    owners = ('norchine', 'abc:alice_iot')

    def test_alice_2030(self, alice):
        response = alice('включи лампочку')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Включаю.',
            ]
        )

        response = alice('выключи лампочку')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Выключаю.',
                'Окей, выключаем.',
                'Окей, выключаю.',
            ]
        )

        response = alice('включи свет')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Включаю.',
            ]
        )

        response = alice('прибавь яркость света')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей, сделаем поярче.',
                'Окей, делаем ярче.',
                'Окей, добавим яркости.',
                'Добавляю яркости.',
                'Больше яркости. Окей.',
            ]
        )

        response = alice('уменьши яркость на лампочке')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей, делаем темнее.',
                'Окей, сделаем темнее.',
                'Окей, убавим яркость.',
                'Убавляю яркость.',
                'Меньше яркости. Окей.',
            ]
        )

        response = alice('сделай яркость света 55 процентов')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Меняю яркость.',
            ]
        )

        response = alice('включи максимальную яркость на лампочке')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей, яркость на максимум.',
                'Хорошо. Максимум яркости.',
                'Хорошо, яркость на максимум.',
                'Как скажете. Максимальная яркость.',
            ]
        )

        response = alice('включи минимальную яркость на лампочке')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Хорошо. Минимум яркости.',
                'Как скажете. Минимальная яркость.',
            ]
        )

        response = alice('сделай свет потеплее')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей, больше тёплого.',
                'Окей, больше тёплого света.',
                'Включаю свет потеплее.',
            ]
        )

        response = alice('измени цвет лампочки на синий')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Меняю цвет.',
            ]
        )

    def test_alice_1939_2031(self, alice):

        assert_on = [
            'Включаю.',
        ] + nlg.ok

        response = alice('включи кондиционер')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_on)

        response = alice('включи кондей')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_on)

        assert_off = [
            'Выключаю.',
            'Окей, выключаю.',
            'Окей, выключаем.',
        ] + nlg.ok

        response = alice('выключи кондиционер')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_off)

        response = alice('выключи кондей')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_off)

        assert_warm = [
            'Окей, теплее.',
            'Окей, сделаем теплее.',
        ] + nlg.ok

        response = alice('прибавь температуру на кондиционере')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_warm)

        response = alice('увеличь температуру на кондиционере')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_warm)

        response = alice('сделай теплее')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_warm)

        assert_cold = [
            'Окей, прохладнее.',
            'Окей, сделаем холоднее.',
        ] + nlg.ok

        response = alice('уменьши температуру на кондиционере')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_cold)

        response = alice('убавь температуру на кондиционере')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_cold)

        response = alice('сделай холоднее')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_cold)

        assert_temperature = [
            'Делаю.',
            'Меняю температуру.',
        ] + nlg.ok

        response = alice('включи температуру кондиционера 20 градусов')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_temperature)

        response = alice('установи температуру кондея 19')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_temperature)

        response = alice('сделай температуру восемнадцать градусов')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_temperature)

        response = alice('сделай двадцать два градуса')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_temperature)

        assert_max = [
            'Ставлю температуру на максимум.',
            'Ставлю самую высокую температуру.',
        ] + nlg.ok

        response = alice('включи максимальную температуру на кондее')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_max)

        response = alice('включи температуру кондиционера на максимум')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_max)

        response = alice('поставь максимальную температуру')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_max)

        assert_min = [
            'Ставлю температуру на минимум.',
            'Ставлю самую низкую температуру.',
        ] + nlg.ok

        response = alice('включи минимальную температуру на кондее')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_min)

        response = alice('включи температуру кондиционера на минимум')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_min)

        response = alice('поставь минимальную температуру')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_min)

        assert_cool_mode = [
            'Окей, охлаждение.',
            'Окей. Режим охлаждения.',
            'Включаю режим охлаждения.',
        ] + nlg.ok

        response = alice('включи режим охлаждения на кондиционере')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_cool_mode)

        response = alice('включи режим охлаждения')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_cool_mode)

        assert_dry_mode = [
            'Окей, осушение.',
            'Окей. Режим осушения.',
            'Включаю режим осушения.',
            'Как скажете. Режим осушения.',
            'Режим осушения, поехали.',
        ] + nlg.ok

        response = alice('включи режим осушения на кондиционере')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_dry_mode)

        response = alice('включи режим осушения')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_dry_mode)

        assert_auto_mode = [
            'Окей. Автоматический режим.',
            'Включаю автоматический режим.',
            'Как скажете. Автоматический режим.',
            'Автоматический режим, поехали.',
        ] + nlg.ok

        response = alice('включи режим авто')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, assert_auto_mode)

        response = alice('включи скорость вентиляции на кондиционере на среднюю')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)

        response = alice('включи на кондиционере скорость обдува на минимум')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)

        response = alice('включи на кондиционере высокую скорость обдува')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)

        response = alice('включи на кондиционере скорость обдува на максимум')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)

        response = alice('включи на кондиционере низкую скорость обдува')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)

        response = alice('включи низкую скорость вентиляции')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)

        response = alice('двадцать два градуса')
        assert is_iot(response) is False

        response = alice('включи 18')
        assert is_iot(response) is False

        response = alice('режим охлаждения')
        assert is_iot(response) is False

        response = alice('средняя скорость вентиляции')
        assert is_iot(response) is False

    def test_alice_2033(self, alice):
        response = alice('включи телевизор')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Включаю.',
            ]
        )

        response = alice('выключи телевизор')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Выключаю.',
                'Окей, выключаю.',
                'Окей, выключаем.',
            ]
        )

        response = alice('включи звук на телевизоре')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Нажимаю кнопку «Mute».',
            ]
        )

        response = alice('выключи звук на телевизоре')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Нажимаю кнопку «Mute».',
            ]
        )

        response = alice('сделай звук на телевизоре погромче')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Делаю громче.',
                'Делаю погромче.',
                'Окей, делаю погромче.',
            ]
        )

        response = alice('сделай звук на телеке потише')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Делаю тише.',
                'Делаю потише.',
                'Окей, делаю потише.',
                'Окей, убавляю громкость.',
            ]
        )

        response = alice('включи следующий канал на телевизоре')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Переключаю.',
                'Окей, переключаю.',
                'Окей, следующий канал.',
                'Включаю следующий.',
                'Включаю следующий канал.',
            ]
        )

        response = alice('включи предыдущий канал на телевизоре')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Переключаю назад.',
                'Включаю предыдущий.',
                'Включаю предыдущий канал.',
                'Окей, переключаю назад.',
                'Окей, предыдущий канал.',
                'Возвращаюсь на предыдущий канал.',
            ]
        )

        response = alice('включи канал 10 на телевизоре')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Переключаю.',
            ]
        )

    def test_alice_2035(self, alice):
        response = alice('включи всё на кухне')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Включаю.',
            ]
        )

        response = alice('выключи всё в зале')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Выключаю.',
                'Окей, выключаю.',
                'Окей, выключаем.',
            ]
        )

        response = alice('включи кухне свет')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Включаю.',
            ]
        )

    def test_alice_2726(self, alice):
        response = alice('включи пекарь')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)
        assert_response_text(response.text, nlg.ok)

        response = alice('приготовь кашу на пекаре')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)

        response = alice('сделай пюре в пекаре')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(response.text, nlg.ok)


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmWatch(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-2126
    """

    owners = ('norchine', 'abc:alice_iot')

    def test_alice_2126(self, alice, surface):
        response = alice('включи свет')
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'С умным домом я справлюсь лучше на телефоне или умной колонке.',
            ]
        )

        response = alice('давай сыграем в города')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request

        response = alice('включи свет')
        assert response.scenario == scenario.Dialogovo
        _assert_its_not_a_city(response.text)


@pytest.mark.oauth(auth.Smarthome)
@pytest.mark.parametrize('surface', [
    surface.navi,
    surface.searchapp,
    surface.yabro_win,
])
class TestPalmIotScenarios(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-2028
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'доброе утро',
        'запусти сценарий вечеринка',
        'у нас гости',
        'утро доброе',
        'гости у нас',
    ])
    def test_alice_2028(self, alice, command, surface):
        response = alice(command)
        assert is_iot(response) is True
        assert not response.suggests
        assert response.text.startswith((
            'Окей', 'Начнём', 'Активирую программу',
            'Запускаю программу', 'Запускаю вашу программу',
        ))


@pytest.mark.oauth(auth.Smarthome)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmIotInOtherScenarios(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-2199
    """

    owners = ('norchine', 'abc:alice_iot')

    def test_alice_2199(self, alice, surface):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation

        response = alice('включи лампочку')
        assert response.scenario == scenario.GeneralConversation

        response = alice('хватит болтать')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.PureGeneralConversationDeactivation

        response = alice('давай сыграем в города')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Request

        response = alice('включи свет')
        assert response.scenario == scenario.Dialogovo
        _assert_its_not_a_city(response.text)

        response = alice('хватит')
        assert response.scenario == scenario.Dialogovo
        assert response.intent == ExternalSkillIntents.Deactivate

        response = alice('помоги купить на Беру')
        assert response.intent == intent.Market

        response = alice('включи свет')
        assert response.intent == intent.MarketEllipsis
