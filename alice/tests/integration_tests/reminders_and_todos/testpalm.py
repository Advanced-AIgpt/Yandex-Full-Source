import re

import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class TestPalmUnsupported(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-27
    https://testpalm.yandex-team.ru/testcase/alice-1262
    https://testpalm.yandex-team.ru/testcase/alice-2166
    https://testpalm.yandex-team.ru/testcase/alice-2581
    """

    owners = ('leletko',)
    expected_response = 'Простите, напоминания доступны только на Станциях и колонках, в которых я живу.'

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.navi,
        surface.smart_tv,
        surface.watch,
        surface.yabro_win,
    ])
    def test_all(self, alice):
        response = alice('поставь напоминание')
        assert response.intent == intent.CreateReminder
        assert response.text == self.expected_response

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.parametrize('surface', [
        surface.launcher,
        surface.searchapp,
    ])
    def test_only_reminders(self, alice):
        response = alice('поставь напоминание')
        assert response.intent == intent.CreateReminder
        assert response.text == self.expected_response

    @pytest.mark.parametrize('surface', [surface.yabro_win])
    def test_alice_2166(self, alice):
        response = alice('Что играет?')
        assert response.intent == intent.MusicWhatIsPlaying
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
        assert not response.directive

        response = alice('Поставь будильник')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        assert response.text == 'Я так пока не умею.'
        assert not response.directive

        response = alice('Поставь таймер')
        assert response.intent == intent.TimerSet
        assert response.text in [
            'Засекать время я пока не умею. Обязательно научусь.',
            'Нет, таймер я пока не умею устанавливать. Но это временно.',
            'Я пока не умею устанавливать таймеры на этом устройстве, но когда-нибудь научусь.',
        ]
        assert not response.directive

        response = alice('Поставь напоминание')
        assert response.intent == intent.CreateReminder
        assert response.text == 'Простите, напоминания доступны только на Станциях и колонках, в которых я живу.'
        assert not response.directive

        response = alice('Добавь дело')
        assert response.intent == intent.CreateTodo
        assert response.text in [
            'Я этого пока не умею, но обязательно научусь.',
            'Этого я пока не умею, но я способная, я научусь.',
            'Ох, это я пока не умею. Эту функцию я отложила для будущих версий.',
            'Программист Алексей обещал запрограммировать это к осени. Но не сказал, к какой.',
            'Этого я пока не умею. Но это временно.',
        ]
        assert not response.directive


@pytest.mark.skip
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
])
class TestPalmReminders(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-577
    https://testpalm.yandex-team.ru/testcase/alice-670
    """

    owners = ('mihajlova',)

    def test(self, alice):
        response = alice('поставь напоминание')
        assert response.intent == intent.CreateReminder
        assert re.fullmatch('(О чём|Что).* напомнить?', response.text)

        response = alice('сварить макароны')
        assert response.intent == intent.CreateReminderEllipsis
        assert response.text == 'На какое время поставить напоминание?'

        response = alice('на завтра в 19 часов')
        assert response.intent == intent.CreateReminderEllipsis
        assert response.text == 'Поставила напоминание \"сварить макароны\" на завтра в 19:00.'

        response = alice('удали напоминание "сварить макароны"')
        assert response.intent == intent.CreateReminderCancel
        assert response.text == 'Отменила это напоминание.'

        response = alice('покажи напоминания')


@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestPalmSmartTvReminders(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2867
    """

    owners = ('ivanbo',)
    todo_expected_response = [
        'Я этого пока не умею, но обязательно научусь.',
        'Этого я пока не умею, но я способная, я научусь.',
        'Ох, это я пока не умею. Эту функцию я отложила для будущих версий.',
        'Программист Алексей обещал запрограммировать это к осени. Но не сказал, к какой.',
        'Этого я пока не умею. Но это временно.',
    ]

    def test_alice_2867(self, alice):
        response = alice('Добавь дело')
        assert response.intent == intent.CreateTodo
        assert response.text in self.todo_expected_response
        assert not response.directive

        response = alice('Список дел')
        assert response.intent == intent.ListTodo
        assert response.text in self.todo_expected_response
        assert not response.directive
