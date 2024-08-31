import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.iot_demo as config
from iot.common import is_iot, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.SmarthomeOther)
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestIotDemo(object):
    """
        See more https://st.yandex-team.ru/IOT-555
    """

    owners = ('norchine', 'abc:alice_iot')

    def test_phrase_fan(self, alice):
        response = alice('включи вентилятор')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'С удовольствием. Только сначала подключите его к умному дому. '
                'Если у вас обычный вентилятор и умная розетка, вставьте одно в другое и переименуйте'
                ' розетку в приложении Дом с Алисой. А если ваш вентилятор и сам умный, добавьте его в'
                ' приложение как самостоятельное умное устройство. Когда всё настроите,'
                ' магия заработает.'
            ]
        )

    def test_phrase_heater(self, alice):
        response = alice('включи обогреватель')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'С удовольствием. Только сначала подключите его к умному дому. Если у вас обычный'
                ' обогреватель и умная розетка, вставьте одно в другое и переименуйте розетку'
                ' в приложении Дом с Алисой. А если ваш обогреватель и сам умный, добавьте его в приложение'
                ' как самостоятельное умное устройство. Когда всё настроите, магия заработает.'
            ]
        )

    @pytest.mark.parametrize('command', [
        'включи кондиционер',
        'сделай кондиционер попрохладнее',
        'переведи кондиционер в режим обогрева',
        'включи кондиционер на 23 градуса',
        'выключи кондиционер через полчаса',
    ])
    def test_phrase_ac(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Легко. Только сначала подключите его к умному дому. Если у вас обычный кондиционер'
                ' и умный пульт Яндекса, найдите его в приложении Дом с Алисой, нажмите «Добавить пульт», '
                'выберите «Кондиционер» — а дальше поможет приложение. А если ваш кондиционер'
                ' и сам умный, добавьте его в приложение как самостоятельное умное устройство. '
                'Когда всё настроите, я смогу управлять кондиционером. Только попросите.'
            ]
        )

    def test_phrase_tv_box(self, alice):
        response = alice('переключи ресивер на следующий канал')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Не вижу ваш ресивер. Вы его подключили? Если у вас обычный ресивер и умный '
                'пульт Яндекса, найдите его в приложении Дом с Алисой, нажмите «Добавить пульт», выберите '
                'ресивер и следуйте инструкции. А если ваш ресивер и сам умный, добавьте его '
                'в приложение как самостоятельное умное устройство. Когда всё будет готово, '
                'возвращайтесь — я смогу переключать каналы, включать ресивер и так далее.'
            ]
        )

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-9706')
    def test_phrase_tv(self, alice):
        response = alice('сделай телевизор потише')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Кажется, вы его не подключили. Если у вас обычный телевизор и умный пульт Яндекса, '
                'найдите его в приложении Дом с Алисой, нажмите «Добавить пульт», выберите телевизор и '
                'слушайтесь приложения. А если ваш телевизор и сам умный, добавьте его в приложение '
                'как самостоятельное умное устройство. Когда всё настроите, возвращайтесь — и будем '
                'вместе смотреть кино.'
            ]
        )

    def test_phrase_fireplace(self, alice):
        response = alice('включи электрокамин')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Я бы с радостью, но сначала нужно подключить его к умному дому. Если у вас обычный '
                'электрокамин и умный пульт Яндекса, найдите его в приложении Дом с Алисой, нажмите '
                '«Добавить пульт» и выберите ручную настройку. Дальше просто слушайтесь приложения. '
                'А если ваш электрокамин и сам умный, добавьте его в приложение как самостоятельное '
                'умное устройство. Когда всё будет готово, приходите — магия заработает.'
            ]
        )


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.no_smarthome)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestEmptyConfig(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2226
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет',
    ])
    def test_turn_on(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.no_smarthome)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.households_with_lamp)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestNoFan(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-3219
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи вентилятор',
    ])
    def test_turn_on(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.no_fan)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.no_smarthome)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestAskAboutConnection(object):

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize(
        'command, expected_response, expected_iot',
        [
            (
                'как подключить лампочку?',
                'Ничего сложного в этом нет! Переведите ваше устройство в режим подключения. '
                'Когда будете готовы, скажите: "Алиса, найди устройство".',
                True,
            ),
            (
                'как подключить пульт?',
                'Я не умею подключать пульт здесь.',
                False,
            ),
            (
                'как подключить умный пульт?',
                'Ничего сложного в этом нет! Переведите ваше устройство в режим подключения. '
                'Когда будете готовы, скажите: "Алиса, найди устройство".',
                True
            ),
            (
                'как подключить розетку?',
                'Ничего сложного в этом нет! Переведите ваше устройство в режим подключения. '
                'Когда будете готовы, скажите: "Алиса, найди устройство".',
                True
            ),
        ]
    )
    def test_ask_about_connection(self, alice, command, expected_response, expected_iot):
        response = alice(command)
        assert is_iot(response) == expected_iot
        assert_response_text(response.text, [expected_response])


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.household_with_one_lamp)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestLampInDifferentRoom(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-4180
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет в ванной',
    ])
    def test_turn_on(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.wrong_room)
