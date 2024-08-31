import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

from iot.common import is_iot, get_iot_reaction, get_selected_hypothesis, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.configs.voice_queries as config


@pytest.mark.oauth(auth.SmartHomeVoiceQueries)
@pytest.mark.parametrize('surface', [surface.station_pro])
class TestVoiceQueriesDeviceCapabilityPropertyRequests(object):
    owners = ('galecore', 'abc:alice_iot')

    # garland
    @pytest.mark.parametrize('command', [
        'включена ли гирлянда',
        'выключена ли гирлянда',
        'алиса а работает сейчас гирлянда',
    ])
    def test_device_garland_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Гирлянда включена'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что с напряжением на гирлянде',
        'какое напряжение на гирлянде',
    ])
    def test_device_garland_amperage_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Текущее напряжение 10 вольт'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что с током на гирлянде',
        'какой ток в гирлянде',
    ])
    def test_device_garland_voltage_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Потребление тока 5 ампер'
            ]
        )

    # ac
    @pytest.mark.parametrize('command', [
        'включен ли кондиционер',
        'работает ли сейчас кондиционер',
    ])
    def test_device_ac_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Кондиционер включен'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какая температура на кондиционере',
        'что с температурой на кондее',
    ])
    def test_device_ac_temperature_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Целевая температура 24 градуса цельсия'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какой режим термостата на кондиционере',
    ])
    def test_device_ac_mode_thermostat_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Режим работы термостата - охлаждение'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какая скорость вентиляции на кондиционере',
        'что со скоростью вентиляции в кондее'
    ])
    def test_device_ac_mode_fan_speed_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Скорость вентиляции - авто'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какой режим на кондиционере',
        'что с режимом на кондиционере'
    ])
    def test_device_ac_mode_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Режим работы термостата - охлаждение, скорость вентиляции - авто'
            ]
        )

    # purifier
    @pytest.mark.parametrize('command', [
        'включен ли очиститель',
        'работает ли сейчас мой очиститель'
    ])
    def test_device_purifier_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Очиститель включен'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какой уровень углекислого газа в очистителе',
        'что с кислородом в очистителе'
    ])
    def test_device_purifier_co2_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Уровень углекислого газа 700 миллионных долей'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какая температура на очистителе',
        'что с температурой на очистителе',
    ])
    def test_device_purifier_temperature_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Температура 20 градусов цельсия'
            ]
        )

    # vacuum
    @pytest.mark.parametrize('command', [
        'включен ли пылесос',
        'работает ли пылесос'
    ])
    def test_device_vacuum_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Пылесос включен'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что с уровнем заряда на пылесосе',
        'какой уровень заряда в пылесосе'
    ])
    def test_device_vacuum_battery_level_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Уровень заряда 95 процентов'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что со скоростью работы пылесоса',
        'какая скорость работы пылесоса'
    ])
    def test_device_vacuum_mode_work_level_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Скорость работы - турбо'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что с паузой на пылесосе',
        'включена ли пауза на пылесосе'
    ])
    def test_device_vacuum_pause_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Пауза выключена'
            ]
        )

    # coffeemaker
    @pytest.mark.parametrize('command', [
        'включена ли кофеварка',
        'работает ли кофеварка'
    ])
    def test_device_coffeemaker_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Кофеварка включена'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какой режим на кофеварке',
        'что с режимом на кофеварке',
        'какой режим кофе на кофеварке',
        'что с режимом кофе в кофеварке',
    ])
    def test_device_coffeemaker_mode_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Режим приготовления кофе - эспрессо'
            ]
        )

    # lamp
    @pytest.mark.parametrize('command', [
        'включена ли лампа',
        'работает ли лампа',
    ])
    def test_device_lamp_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Лампа выключена'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какой цвет сейчас на лампе',
        'каким цветом светит лампа',
        'каким цветом горит лампа',
        'что за цвет на лампе',
    ])
    def test_device_lamp_color_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Горит красный цвет'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какая яркость на лампе',
        'что с яркостью на лампе',
    ])
    def test_device_lamp_brightness_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Яркость - 75 процентов'
            ]
        )

    # multicooker
    @pytest.mark.parametrize('command', [
        'включена ли мультиварка',
        'работает ли мультиварка',
    ])
    def test_device_multicooker_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Мультиварка включена'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какая программа на мультиварке',
        'какой режим на мультиварке',
        'что за программа стоит на мультиварке',
        'в каком режиме работает мультиварка',
        'что с режимом на мультиварке',
    ])
    def test_device_multicooker_mode_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Программа - плов'
            ]
        )

    # dishwasher
    @pytest.mark.parametrize('command', [
        'включена ли посудомойка',
        'работает ли посудомойка',
    ])
    def test_device_dishwasher_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Посудомойка включена'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какая программа на посудомойке',
        'какой режим на посудомойке',
        'что за программа стоит на посудомойке',
        'в каком режиме работает посудомойка',
        'что с режимом на посудомойке',
    ])
    def test_device_dishwasher_mode_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Программа - эко'
            ]
        )

    # washing_machine
    @pytest.mark.parametrize('command', [
        'включена ли стиралка',
        'работает ли стиралка',
    ])
    def test_device_washing_machine_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Стиралка включена'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какая программа на стиралке',
        'какой режим на стиралке',
        'что за программа стоит на стиралке',
        'в каком режиме работает стиралка',
        'что с режимом на стиралке',
    ])
    def test_device_washing_machine_mode_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Программа - шерсть'
            ]
        )

    # kettle
    @pytest.mark.parametrize('command', [
        'включен ли чайник',
        'работает ли чайник',
    ])
    def test_device_kettle_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Чайник включен'
            ]
        )

    @pytest.mark.parametrize('command', [
        'включена ли подсветка в чайнике',
        'работает ли подсветка в чайнике',
        'что с подсветкой в чайнике',
    ])
    def test_device_kettle_backlight_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Подсветка включена'
            ]
        )

    @pytest.mark.parametrize('command', [
        'включено ли поддержание тепла в чайнике',
        'включен ли режим поддержания тепла в чайнике',
        'работает ли поддержание тепла в чайнике',
        'работает ли режим поддержания тепла в чайнике',
        'что с поддержанием тепла в чайнике',
    ])
    def test_device_kettle_keep_warm_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Поддержание тепла выключено'
            ]
        )

    @pytest.mark.parametrize('command', [
        'разблокировано ли управление в чайнике',
        'управление в чайнике заблокировано',
        'что с блокировкой управления в чайнике',
    ])
    def test_device_kettle_controls_locked_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Управление заблокировано'
            ]
        )

    # fan
    @pytest.mark.parametrize('command', [
        'включен ли вентилятор',
        'работает ли вентилятор',
    ])
    def test_device_fan_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Вентилятор выключен'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что со скоростью вентиляции в вентиляторе',
        'какая скорость вентиляции в вентиляторе',
        'что с режимом на вентиляторе',
    ])
    def test_device_fan_mode_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Скорость вентиляции - авто'
            ]
        )

    @pytest.mark.parametrize('command', [
        'включена ли ионизация на вентиляторе',
        'работает ли функция ионизация в вентиляторе',
        'что с ионизацией на вентиляторе',
    ])
    def test_device_fan_ionization_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Ионизация выключена'
            ]
        )

    # tv
    @pytest.mark.parametrize('command', [
        'включен ли телевизор',
        'работает ли телевизор',
    ])
    def test_device_tv_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Телевизор включен'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какой источник сигнала на телевизоре',
        'что с источником сигнала на телевизор',
    ])
    def test_device_tv_input_source_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Источник сигнала - один'
            ]
        )

    @pytest.mark.parametrize('command', [
        'включен ли звук на телевизоре',
        'что со звуком на телевизоре',
    ])
    def test_device_tv_mute_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Звук включен'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какой канал стоит на телевизоре',
        'какой канал работает на телевизоре',
        'какой канал на телике',
        'что за канал на телевизоре',
    ])
    def test_device_tv_channel_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Работает 3 канал'
            ]
        )

    @pytest.mark.parametrize('command', [
        'какая громкость на телевизоре',
        'что с громкостью на телике',
    ])
    def test_device_tv_volume_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Громкость 50'
            ]
        )

    # humidifier
    @pytest.mark.parametrize('command', [
        'включен ли увлажнитель',
        'работает ли увлажнитель воздуха',
    ])
    def test_device_humidifier_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Увлажнитель воздуха включен'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что с влажностью в увлажнителе',
        'какая влажность в увлажнителе',
    ])
    def test_device_humidifier_humidity_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Влажность 35 процентов'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что с уровнем воды в увлажнителе',
        'какой уровень воды в увлажнителе',
    ])
    def test_device_humidifier_water_level_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Уровень воды 90 процентов'
            ]
        )

    # curtain
    @pytest.mark.parametrize('command', [
        'открыты ли шторы',
    ])
    def test_device_curtain_on_off_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Шторы открыты'
            ]
        )

    @pytest.mark.parametrize('command', [
        'насколько открыты шторы',
    ])
    def test_device_curtain_open_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Открыто на 50 процентов'
            ]
        )


@pytest.mark.oauth(auth.SmartHomeVoiceQueries)
@pytest.mark.parametrize('surface', [surface.station_pro])
class TestVoiceQueriesDeviceStateRequests(object):
    owners = ('galecore', 'abc:alice_iot')

    # garland
    @pytest.mark.parametrize('command', [
        'что с гирляндой',
        'статус гирлянды',
    ])
    def test_device_garland_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Гирлянда включена, текущее напряжение 10 вольт, '
                'потребление тока 5 ампер, потребляемая мощность 50 ватт'
            ]
        )

    # ac
    @pytest.mark.parametrize('command', [
        'что с кондиционером',
    ])
    def test_device_ac_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Кондиционер включен, целевая температура 24 градуса цельсия, '
                'режим работы термостата - охлаждение, скорость вентиляции - авто, ионизация выключена'

            ]
        )

    # purifier
    @pytest.mark.parametrize('command', [
        'что с очистителем',
    ])
    def test_device_purifier_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Очиститель включен, уровень углекислого газа 700 миллионных долей, '
                'температура 20 градусов цельсия'
            ]
        )

    # vacuum
    @pytest.mark.parametrize('command', [
        'что с пылесосом',
    ])
    def test_device_vacuum_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Пылесос включен, скорость работы - турбо, уровень заряда 95 процентов'
            ]
        )

    # warm_floor
    @pytest.mark.parametrize('command', [
        'что с теплым полом',
    ])
    def test_device_warm_floor_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Теплый пол включен, целевая температура 24 градуса цельсия'
            ]
        )

    # coffee_maker
    @pytest.mark.parametrize('command', [
        'что с кофеваркой',
    ])
    def test_device_coffee_maker_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Кофеварка включена, режим приготовления кофе - эспрессо'
            ]
        )

    # lamp
    @pytest.mark.parametrize('command', [
        'что с лампой',
    ])
    def test_device_lamp_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Лампа выключена'
            ]
        )

    # multicooker
    @pytest.mark.parametrize('command', [
        'что с мультиваркой',
    ])
    def test_device_multicooker_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Мультиварка включена, программа - плов'
            ]
        )

    # dishwasher
    @pytest.mark.parametrize('command', [
        'что с посудомойкой',
    ])
    def test_device_dishwasher_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Посудомойка включена, программа - эко'
            ]
        )

    # washing_machine
    @pytest.mark.parametrize('command', [
        'что со стиралкой',
    ])
    def test_device_washing_machine_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Стиралка включена, программа - шерсть'
            ]
        )

    # kettle
    @pytest.mark.parametrize('command', [
        'что с чайником',
    ])
    def test_device_kettle_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Чайник включен, подсветка включена, поддержание тепла выключено, '
                'управление заблокировано'
            ]
        )

    # fan
    @pytest.mark.parametrize('command', [
        'что с вентилятором',
    ])
    def test_device_fan_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Вентилятор выключен'
            ]
        )

    # bulb1
    @pytest.mark.parametrize('command', [
        'что с лампочкой 1',
    ])
    def test_device_bulb1_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Лампочка 1 выключена'
            ]
        )

    # bulb2
    @pytest.mark.parametrize('command', [
        'что с лампочкой 2',
    ])
    def test_device_bulb2_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Лампочка 2 включена, яркость - 12 процентов'
            ]
        )

    # strip
    @pytest.mark.parametrize('command', [
        'что с белой лентой',
    ])
    def test_device_strip_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Белая лента выключена'
            ]
        )

    # tv
    @pytest.mark.parametrize('command', [
        'что с телевизором',
    ])
    def test_device_tv_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Телевизор включен, работает 3 канал, громкость 50, источник сигнала - один'
            ]
        )

    # humidifier
    @pytest.mark.parametrize('command', [
        'что с увлажнителем',
    ])
    def test_device_humidifier_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Увлажнитель воздуха включен, ионизация включена, '
                'целевая влажность 40 процентов, уровень воды 90 процентов, '
                'влажность 35 процентов, температура 28 градусов цельсия'
            ]
        )

    # curtain
    @pytest.mark.parametrize('command', [
        'что со шторой',
    ])
    def test_device_curtain_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Шторы открыты, открыто на 50 процентов'
            ]
        )


@pytest.mark.oauth(auth.SmartHomeVoiceQueries)
@pytest.mark.parametrize('surface', [surface.station_pro])
class TestVoiceQueriesRoomPropertyRequests(object):
    owners = ('galecore', 'abc:alice_iot')

    # living_room
    @pytest.mark.parametrize('command', [
        'что температурой в гостиной',
    ])
    def test_room_living_room_temperature_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Температура 20 градусов цельсия'
            ]
        )

    @pytest.mark.parametrize('command', [
        'что уровнем углекислого газа в гостиной',
        'что углекислым газом в гостиной',
        'что кислородом в гостиной',
    ])
    def test_room_living_room_co2_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Уровень углекислого газа 700 миллионных долей'
            ]
        )

    # bedroom
    @pytest.mark.parametrize('command', [
        'что с влажностью в спальне',
    ])
    def test_room_bedroom_humidity_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Влажность 35 процентов'
            ]
        )


@pytest.mark.oauth(auth.SmartHomeVoiceQueries)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestVoiceQueriesGroupRequests(object):
    owners = ('galecore', 'abc:alice_iot')

    # overhead lights
    @pytest.mark.parametrize('command', [
        'что с верхним светом',
    ])
    def test_group_overhead_light_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Лампочка 1 выключена; лампочка 2 включена, яркость - 12 процентов'
            ]
        )

    # ac-s
    @pytest.mark.parametrize('command', [
        'что с обдувателями',
    ])
    def test_group_acs_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Кондиционер включен, целевая температура 24 градуса цельсия, '
                'режим работы термостата - охлаждение, скорость вентиляции - авто, '
                'ионизация выключена; кондиционер обдув выключен'
            ]
        )

    # sockets
    @pytest.mark.parametrize('command', [
        'что с электроприборами',
    ])
    def test_group_sockets_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Гирлянда включена, текущее напряжение 10 вольт, '
                'потребление тока 5 ампер, потребляемая мощность 50 ватт; '
                'розетка включена, текущее напряжение 10 вольт, '
                'потребление тока 5 ампер, потребляемая мощность 50 ватт'
            ]
        )


@pytest.mark.oauth(auth.SmartHomeVoiceQueries)
@pytest.mark.parametrize('surface', [surface.station_pro])
class TestVoiceQueriesDeviceTypeStatusRequests(object):
    owners = ('galecore', 'abc:alice_iot')

    # light
    @pytest.mark.parametrize('command', [
        'что со светом',
    ])
    def test_device_type_light_status(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Белая лента в спальне выключена; '
                'лампа на кухне выключена; '
                'лампочка 1 в мастерской выключена; '
                'лампочка 2 в мастерской включена, яркость - 12 процентов'
            ]
        )


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.sensors_humidity)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestSensors(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-3422
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'что с датчиками в квартире?',
    ])
    def test_sensors_all(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Датчик на кухне сейчас в сети, уровень заряда 15 процентов, влажность 77 процентов; '
                'датчик в спальне сейчас в сети, уровень заряда 20 процентов, влажность 66 процентов; '
                'датчик в кладовой сейчас в сети, уровень заряда 10 процентов, влажность 55 процентов',
            ]
        )

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'query'
            query_parameters = iot_reaction['query_parameters']
            assert query_parameters is not None
            assert set(query_parameters['devices']) == {'evo-test-property-id-1', 'evo-test-property-id-2', 'evo-test-property-id-3'}
            assert query_parameters['target'] == 'state'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-property-id-1', 'evo-test-property-id-2', 'evo-test-property-id-3'}
        assert response_info['type'] == 'query'
        assert response_info['action']['target'] == 'state'

    @pytest.mark.parametrize('command', [
        'что с уровнем заряда датчиков в квартире?',
    ])
    def test_sensors_battery(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Уровень заряда на датчике на кухне 15 процентов, уровень заряда на датчике в '
                'спальне 20 процентов, уровень заряда на датчике в кладовой 10 процентов',
            ]
        )

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'query'
            query_parameters = iot_reaction['query_parameters']
            assert query_parameters is not None
            assert set(query_parameters['devices']) == {'evo-test-property-id-1', 'evo-test-property-id-2', 'evo-test-property-id-3'}
            assert query_parameters['target'] == 'property'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-property-id-1', 'evo-test-property-id-2', 'evo-test-property-id-3'}
        assert response_info['type'] == 'query'
        assert response_info['action']['type'] == 'devices.properties.float'
        assert response_info['action']['target'] == 'property'

    @pytest.mark.parametrize('command', [
        'что с уровнем влажности датчиков в квартире?',
    ])
    def test_sensors_humidity(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Влажность 66 процентов',
            ]
        )

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'query'
            query_parameters = iot_reaction['query_parameters']
            assert query_parameters is not None
            assert set(query_parameters['devices']) == {'evo-test-property-id-1', 'evo-test-property-id-2', 'evo-test-property-id-3'}
            assert query_parameters['target'] == 'property'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-property-id-1', 'evo-test-property-id-2', 'evo-test-property-id-3'}
        assert response_info['type'] == 'query'
        assert response_info['action']['type'] == 'devices.properties.float'
        assert response_info['action']['target'] == 'property'
