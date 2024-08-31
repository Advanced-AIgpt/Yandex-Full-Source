import re
from datetime import timedelta

import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class TestPalmAlarms(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-6
    https://testpalm.yandex-team.ru/testcase/alice-1081
    https://testpalm.yandex-team.ru/testcase/alice-1341
    https://testpalm.yandex-team.ru/testcase/alice-1912
    https://testpalm.yandex-team.ru/testcase/alice-2051
    https://testpalm.yandex-team.ru/testcase/alice-2128
    https://testpalm.yandex-team.ru/testcase/alice-2133
    https://testpalm.yandex-team.ru/testcase/alice-2135
    https://testpalm.yandex-team.ru/testcase/alice-2136
    https://testpalm.yandex-team.ru/testcase/alice-2246
    https://testpalm.yandex-team.ru/testcase/alice-2253
    https://testpalm.yandex-team.ru/testcase/alice-2255
    https://testpalm.yandex-team.ru/testcase/alice-2256
    https://testpalm.yandex-team.ru/testcase/alice-2257
    https://testpalm.yandex-team.ru/testcase/alice-2326
    https://testpalm.yandex-team.ru/testcase/alice-2365
    https://testpalm.yandex-team.ru/testcase/alice-2366
    https://testpalm.yandex-team.ru/testcase/alice-2369
    https://testpalm.yandex-team.ru/testcase/alice-2370
    https://testpalm.yandex-team.ru/testcase/alice-2371
    https://testpalm.yandex-team.ru/testcase/alice-2416
    https://testpalm.yandex-team.ru/testcase/alice-2418
    https://testpalm.yandex-team.ru/testcase/alice-2722
    https://testpalm.yandex-team.ru/testcase/alice-2851
    Частично https://testpalm.yandex-team.ru/testcase/alice-1390
    """

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    @pytest.mark.parametrize('surface', [surface.station])
    def test_alice_2051(self, alice):
        response = alice('поставь будильник')
        assert response.scenario == scenario.Alarm
        assert response.text == 'На какое время поставить будильник?'
        alice('на 7 утра')

        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 7

        response = alice('поставить будильник на время через три минуты')
        assert response.scenario == scenario.Alarm

        second_alarm = alice.datetime_now + timedelta(minutes=3)
        alarms = alice.device_state.alarms
        assert len(alarms) == 2
        assert alarms[0].hour == 7
        assert alarms[1].minute == second_alarm.minute

        response = alice('удали будильник')
        assert response.text.startswith('Сейчас установлено несколько будильников') and \
               response.text.endswith('Какой из них вы хотите выключить?')
        alice('удалить будильник, поставленный на 7 утра')

        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].minute == second_alarm.minute

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_alice_1081(self, alice):
        response = alice('поставь будильник')
        assert response.scenario == scenario.Alarm
        assert response.text == 'На какое время поставить будильник?'
        response = alice('на 20:30')
        assert 'на 20:30' in response.text

        response = alice('заведи будильник по вторникам на пять часов')
        assert response.scenario == scenario.Alarm
        assert 'по вторникам в 5 часов утра' in response.text

        response = alice('Поставь будильник по будням на 7 утра')
        assert response.scenario == scenario.Alarm
        assert 'по будням в 7 часов утра' in response.text

        response = alice('Покажи список будильников')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmShow
        assert response.directive.name == directives.names.ShowAlarmsDirective
        assert not response.text

    @pytest.mark.parametrize('surface', [surface.loudspeaker])
    def test_alice_1341(self, alice):
        response = alice('поставь будильник')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        assert response.text == 'На какое время поставить будильник?'

        response = alice('на 20:30')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmAskTime
        assert 'на 20:30' in response.text

        response = alice('заведи будильник по вторникам на пять часов')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        assert 'по вторникам в 5 часов утра' in response.text

        response = alice('Поставь будильник по будням на 7 утра 15 минут')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        assert 'по будням в 07:15' in response.text

        response = alice('У меня есть будильники?')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmShow
        assert 'в 20:30' in response.text
        assert 'по вторникам в 5 часов утра' in response.text
        assert 'по будням в 07:15' in response.text

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2128_2133_2257(self, alice):
        response = alice('поставь на будильник музыку для спорта')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert any(f'{phrase} тренировки' in response.text for phrase in [
            'Хорошо, вас разбудит музыка для',
            'Окей. Поставила на будильник музыку для',
            'Поняла. Разбужу вас музыкой для',
        ])
        assert any(phrase in response.text for phrase in [
            'Не забудьте завести будильник.',
            'Только будильник завести не забудьте.',
            'Только сам будильник вы забыли установить. Переживаю.',
            'А самих будильников у вас нет. Можете попросить меня завести.',
        ])

        response = alice('Что на будильнике?')
        assert 'музыка для тренировки' in response.text

        response = alice('Поставь стандартный звук будильника')
        assert response.text in [
            'Возврат к исходному коду... То есть к стандартной мелодии.',
            'Окей. Да будет стандартная мелодия.',
            'Новое — хорошо забытое старое. Вернула обычную мелодию.',
            'Вернула всё как было.',
            'Окей, вернула стандартную мелодию.',
        ]

        response = alice('Поставь стандартный звук будильника')
        assert response.text == 'У вас уже стоит стандартный звук будильника.'

        response = alice('Что на будильнике?')
        assert response.scenario == scenario.Alarm
        assert any(phrase in response.text for phrase in [
            'стандартная мелодия',
            'Будильник самый обычный',
        ])

    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    @pytest.mark.parametrize('command', [
        'Как поставить музыку на будильник?',
        'Как поставить радио на будильник?',
        'Какие мелодии ты можешь поставить на будильник?',
    ])
    def test_alice_2135(self, alice, command):
        response = alice(command)
        assert response.intent == intent.AlarmHowToSetSound
        assert any(phrase in response.text for phrase in [
            'попросите меня: Алиса, поставь',
            'Скажите "Алиса, разбуди меня песней',
            'Скажите "Алиса, поставь',
            'Скажите, например, "Алиса, разбуди меня песней',
            'Скажите, например, "Алиса, поставь',
            'Например, скажите "Алиса, разбуди меня песней',
            'Например, скажите "Алиса, поставь',
        ])

    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2246(self, alice):
        response = alice('Поставь бодрую музыку на будильник')
        assert any(phrase in response.text for phrase in [
            'необходимо купить подписку Яндекс.Плюс',
            'необходимо купить подписку на Яндекс.Плюс',
            'Вы не авторизовались',
        ])

    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2253(self, alice):
        response = alice('Сколько до будильника')
        assert 'Включенных будильников не обнаружено' in response.text

        alice('Поставь будильник через 9 часов')
        alarm = alice.datetime_now + timedelta(hours=9)
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == alarm.hour
        assert alarms[0].minute == alarm.minute

        response = alice('Сколько осталось времени до будильника')
        assert '9 часов' in response.text

        response = alice('удали будильник')
        assert len(alice.device_state.alarms) == 0

        weekday = 'четвергам' if alice.datetime_now.weekday() in [0, 6] else 'понедельникам'
        response = alice(f'Поставь будильник по {weekday} в 5 утра')
        assert f'по {weekday} в 5 часов утра' in response.text

        response = alice('Сколько осталось времени до будильника')
        assert 'нет будильников' in response.text

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2255(self, alice):
        response = alice('поставь на будильник бодрую музыку')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search('Хорошо, вас разбудит энергичная музыка.|Поставила на будильник энергичную музыку.|\
Разбужу вас энергичной музыкой.', response.text)

        response = alice('Поставь мелодию на будильник в жанре электроник')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search('Теперь на вашем будильнике электроника.|Вас разбудит электроника.', response.text)

        response = alice('Поставь на будильник Arctic Monkeys')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search('(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|\
Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) Arctic Monkeys', response.text)

        response = alice('Поставь на будильник песня Bohemian Rhapsody')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search('(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|\
Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) \
Queen(, альбом "The Platinum Collection"|), песня "Bohemian Rhapsody"', response.text)

        response = alice('Поставь на будильник последний альбом Muse')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search('(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|\
Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) \
Muse, альбом "Simulation Theory"', response.text)

        response = alice('Разбуди меня своей песней')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search('(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|\
Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) ', response.text)

        response = alice('Поставь на будильник музыку для приятного сна')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert 'разбудит музыка для приятного сна' in response.text

        response = alice('Поставь мою музыку на будильник')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert any(phrase in response.text for phrase in [
            'Поставила вашу любимую музыку на будильник.',
            'Поставила песни, которые вам обычно нравятся.',
            'Хорошо, поставила кое-что подходящее для вас.',
            'Хорошо, вас разбудят ваши любимые песни.',
        ])

        response = alice('Поставь какую-нибудь музыку на будильник')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert response.text and any(phrase in response.text for phrase in [
            'Хорошо, поставила кое-что подходящее для вас.',
            'Поставила песни, которые вам обычно нравятся.',
            'Хорошо, поставила кое-что в вашем стиле.',
        ])

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2256(self, alice):
        response = alice('Поставь радио бизнес фм на будильник.')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert any(phrase in response.text for phrase in [
            'Радио "Business FM" — хороший выбор.',
            'Вас разбудит радио "Business FM".',
        ])

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2326(self, alice):
        response = alice('Поставь музыку на будильник.')
        assert 'Какую музыку вы хотите установить на будильник?' in response.text

        response = alice('Rolling Stones')
        assert response.intent == intent.AlarmAskSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search('(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|\
Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) The Rolling Stones', response.text)

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2365(self, alice):
        response = alice('Поставь электронную музыку на будильник в 17:00')
        assert 'Вас разбудит электроника' in response.text
        assert any(phrase in response.text for phrase in [
            'в 17:00',
            'в 17 часов',
        ])

        response = alice('поставь рок на будильник через 2 часа')
        assert 'Вас разбудит рок' in response.text

        alarms = alice.device_state.alarms
        second_alarm = alice.datetime_now + timedelta(hours=2)
        assert len(alarms) == 2
        assert alarms[0].hour == 17
        assert alarms[1].hour == second_alarm.hour
        assert alarms[1].minute == second_alarm.minute

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2366(self, alice):
        response = alice('Поставь радио монте карло на завтра на 8 часов на будильник')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.AlarmSetWithSound
        if 3 < alice.datetime_now.hour < 8:
            assert response.text == 'Я могу поставить будильник на ближайшие 24 часа.' \
                ' А еще могу поставить повторяющийся будильник на выбранный день недели.'
        else:
            assert 'Вас разбудит радио "Монте Карло"' in response.text
            assert any(phrase in response.text for phrase in [
                'в 8:00',
                'в 8 часов',
            ])

        response = alice('поставь европа плюс на будильник на сегодня через 3 часа')
        assert 'Вас разбудит радио "Европа плюс"' in response.text

        alarms = alice.device_state.alarms
        second_alarm = alice.datetime_now + timedelta(hours=3)
        if 3 < alice.datetime_now.hour < 8:
            assert len(alarms) == 1
            assert alarms[0].hour == second_alarm.hour
            assert alarms[0].minute == second_alarm.minute
        else:
            assert len(alarms) == 2
            assert alarms[0].hour == 8
            assert alarms[1].hour == second_alarm.hour
            assert alarms[1].minute == second_alarm.minute

    @pytest.mark.no_oauth
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2369(self, alice):
        response = alice('Поставь поставь литтл биг на будильник в 17:00')
        assert response.text and any(phrase in response.text for phrase in [
            'необходимо купить подписку Яндекс.Плюс',
            'вы не авторизовались',
        ])
        assert any(phrase in response.text for phrase in [
            'в 17:00',
            'в 17 часов',
        ])

        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 17

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.children})
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2370(self, alice):
        response = alice('Поставь кровосток на будильник в 17:00')
        assert response.text
        assert any(phrase in response.text for phrase in [
            'включён детский режим',
            'поставить в детском режиме не могу',
        ])
        assert any(phrase in response.text for phrase in [
            'в 17:00',
            'в 17 часов',
        ])

        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 17

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    def test_alice_2371(self, alice):
        response = alice('Поставь радио питер фм на будильник в 17:00')
        assert 'эту частоту я ещё не поймала' in response.text
        assert any(phrase in response.text for phrase in [
            'в 17:00',
            'в 17 часов',
        ])
        response = alice('поставь плейлист Пилите Шура на будильник через 3 минуты')
        assert any(phrase in response.text for phrase in [
            'Вас разбудит плейлист Пилите Шура',
            'Вас разбудит подборка "Плейлист Шуры из «Би-2»".',
            'Вас разбудит подборка "Лучшее: Шура"',
        ])

        alarms = alice.device_state.alarms
        second_alarm = alice.datetime_now + timedelta(minutes=3)
        assert len(alarms) == 2
        assert alarms[0].hour == 17
        assert alarms[1].minute == second_alarm.minute

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('surface', [
        surface.station,
    ])
    def test_alice_2416(self, alice):
        response = alice('Поставь на будильник радио Energy')
        assert any(phrase in response.text for phrase in [
            'Вас разбудит "Радио Energy".',
            '"Радио Energy" — хороший выбор. Разбужу вас этой станцией.',
        ])
        assert any(phrase in response.text for phrase in [
            'Не забудьте завести будильник',
            'Только будильник завести не забудьте',
            'Только сам будильник вы забыли установить',
        ])

        response = alice('Поставь плейлист чтобы проснуться на будильник')
        assert any(phrase in response.text for phrase in [
            'Теперь на вашем будильнике подборка "Проснуться"',
            'На будильнике подборка "Проснуться"',
            'Вас разбудит подборка "Проснуться"',
            'Хорошо, вас разбудит музыка для пробуждения',
            'Разбужу вас музыкой для пробуждения',
        ])
        assert any(phrase in response.text for phrase in [
            'Не забудьте завести будильник',
            'Только будильник завести не забудьте',
            'Только сам будильник вы забыли установить',
        ])

        response = alice('Поставь плейлист "музыка из-под одеялка" на будильник')
        assert any(phrase in response.text for phrase in [
            'Теперь на вашем будильнике подборка "Слушать из-под одеялка"',
            'На будильнике подборка "Слушать из-под одеялка"',
            'Вас разбудит подборка "Слушать из-под одеялка"',
        ])
        assert any(phrase in response.text for phrase in [
            'Не забудьте завести будильник',
            'Только будильник завести не забудьте',
            'Только сам будильник вы забыли установить',
        ])

    @pytest.mark.experiments('change_alarm_sound_level')
    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
    ])
    def test_alice_2722(self, alice):
        response = alice('Какая громкость будильника?')
        assert response.text == 'Громкость будильника - 7.'

        response = alice('Измени громкость будильника на 4')
        assert 'громкость будильника - 4' in response.text

        response = alice('Какая громкость будильника?')
        assert response.text == 'Громкость будильника - 4.'

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.smart_tv,
        surface.watch,
        surface.yabro_win,
    ])
    def test_alarm_unsupported(self, alice):
        response = alice('Поставь будильник')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        assert not response.directive
        assert response.text == 'Я так пока не умею.'

    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.loudspeaker,
        surface.searchapp,
        surface.station,
    ])
    def test_alarm_max_sound_level(self, alice):
        response = alice('какая громкость будильника')
        assert response.text == 'Громкость будильника - 7.'

        alice.device_state.AlarmState.MaxSoundLevel = 6

        response = alice('какая громкость будильника')
        assert response.text == 'Громкость будильника - 6.'

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.navi,
        surface.smart_tv,
        surface.yabro_win,
    ])
    def test_alarm_max_sound_level_unsupported(self, alice):
        response = alice('какая громкость будильника')
        assert response.intent == intent.AlarmWhatSoundLevelIsSet
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
            'Я так пока не умею.',
            'Надеюсь, я скоро смогу это делать. Но пока нет.',
            'К сожалению, у меня нет доступа к будильникам на данном устройстве.',
        ]

    @pytest.mark.parametrize('surface', surface.actual_surfaces)
    @pytest.mark.parametrize('command, level', [
        ('установи громкость будильника 5', 5),
        ('установи минимальную громкость будильника', 1),
        ('средний уровень громкости будильника', 4),
        ('сделай максимальную громкость будильника', 10),
    ])
    def test_alarm_set_max_sound_level(self, alice, command, level):
        response = alice(command)
        if 'change_alarm_sound_level' in alice.supported_features:
            assert response.intent == intent.AlarmSoundSetLevel
            assert response.text.startswith((
                'Хорошо,', 'Готово,', 'Как скажете,',
            ))
            assert f'громкость будильника - {level}' in response.text
            assert response.directive.name == directives.names.AlarmSetMaxLevelDirective
        else:
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
                'В часах такое провернуть сложновато.',
                'Я бы и рада, но здесь не могу. Эх.',
                'Здесь точно не получится.',
            ]
