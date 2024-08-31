import re

import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import alice.tests.library.scenario as scenario
import pytest
from alice.tests.library.vins_response import action, DivWrapper, DivIterableWrapper
from cached_property import cached_property


class Memento(object):
    def __init__(self, alice, cleanup):
        self._alice = alice
        self._cleanup = cleanup

    def create_new(self, text):
        response = self._alice(text)
        assert response.scenario in {scenario.Vins, scenario.Reminders}

        schedule_directive = None
        memento_directive = None

        for d in response.voice_response.uniproxy_directives:
            if 'context_save_directive' in d:
                schedule_directive = d.context_save_directive
                break

        for d in response.voice_response.directives:
            if d.name == 'update_memento':
                memento_directive = d

        assert schedule_directive is not None
        assert memento_directive is not None

        if self._cleanup:
            self._cleanup.add(schedule_directive)

        return response


@pytest.fixture
def cleanup_reminder():
    class RemindersCleanup(object):
        def __init__(self):
            self._reminders = []

        def add(self, directive):
            self._reminders.append(directive.payload.Spec.Action.SendTechnicalPush.TechnicalPush.SpeechKitDirective.payload.typed_semantic_frame.reminders_on_shoot_semantic_frame.id.string_value)

        def clean(self):
            # TODO (petrk) Write cleanup here.
            pass

    reminders = RemindersCleanup()
    yield reminders
    reminders.clean()


class ReminderResponse(object):
    WhatTime = r'На какое время поставить напоминание\?'
    WhatSubject = r'(О чём|Что).* напомнить\?'


class RemindersCard(DivWrapper):
    class _Reminders(DivIterableWrapper):
        class _Item(DivWrapper):
            @action
            def cancel(self):
                return self.cancel_action

    def __init__(self, data):
        assert data.log_id == 'reminders_card'
        super().__init__(data.reminders_card)

    @property
    def header(self):
        assert self.card_items[0].type == 'reminders_header'
        return self.card_items[0].title

    @cached_property
    def reminders(self):
        return RemindersCard._Reminders(self.card_items[1:])


@pytest.mark.supported_features('supports_device_local_reminders')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestLocalReminders(object):
    owners = ('petrk', )

    def test_create(self, alice):
        response = alice('поставь напоминание поесть завтра в 8:00')
        assert response.scenario == scenario.Reminders
        assert response.text.lower() == 'вы успешно поставили напоминание "поесть" на завтра в 8 часов утра.'

    @pytest.mark.unsupported_features('div2_cards')
    def test_list_without_div2_cards(self, alice):
        response = alice('поставь напоминание поесть на завтра в 3 часа дня')
        assert response.scenario == scenario.Reminders

        response = alice('покажи список напоминаний')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ListReminders
        assert 'установлено только одно' in response.text
        assert 'поесть на завтра' in response.text

    @pytest.mark.supported_features('div2_cards')
    def test_list_with_div2_cards(self, alice):
        response = alice('поставь напоминание поесть на завтра в 3 часа дня.')
        assert response.scenario == scenario.Reminders

        response = alice('поставь напоминание покормить кота на завтра в пять вечера.')
        assert response.scenario == scenario.Reminders

        response = alice('покажи список напоминаний.')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ListReminders

        card = RemindersCard(response.div_card)
        assert len(card.reminders) == 2

        reminder = card.reminders[0]
        assert reminder.title == 'Поесть'
        assert reminder.description == 'завтра в 3 часа дня'
        assert reminder.cancel_action

        reminder = card.reminders[1]
        assert reminder.title == 'Покормить кота'
        assert reminder.description == 'завтра в 5 вечера'
        assert reminder.cancel_action

    def test_cancel_after_set(self, alice):
        response = alice('поставь напоминание поесть на завтра в 3 часа дня.')
        assert response.scenario == scenario.Reminders

        response = alice('поставь напоминание покормить кота на завтра в пять вечера.')
        assert response.scenario == scenario.Reminders

        response = alice('удали')
        assert 'успешно' in response.text
        assert 'удалил' in response.text

    def test_cancel_by_number_voice(self, alice):
        commands = [
            'поставь напоминание поесть на завтра в 3 часа дня.',
            'поставь напоминание покормить котика завтра в час дня.',
            'поставь напоминание покумекать послезавтра в 15:00.',
        ]

        for command in commands:
            response = alice(command)
            assert response.scenario == scenario.Reminders

        response = alice('покажи список напоминаний.')
        assert response.scenario in {scenario.Vins, scenario.Reminders}

        # это "покормить котика" в 13 часов.
        response = alice('удали второе')
        assert response.scenario in {scenario.Vins, scenario.Reminders}
        text = response.text.lower()
        for word in ['удалил', 'покормить котика', 'завтра', '13']:
            assert word in text

    @pytest.mark.supported_features('div2_cards')
    def test_cancel_by_number_button(self, alice):
        commands = [
            'поставь напоминание поесть на завтра в 3 часа дня.',
            'поставь напоминание покормить котика завтра в час дня.',
            'поставь напоминание покумекать послезавтра в 15:00.',
        ]

        for command in commands:
            response = alice(command)
            assert response.scenario == scenario.Reminders

        response = alice('покажи список напоминаний.')
        assert response.scenario in {scenario.Vins, scenario.Reminders}

        card = RemindersCard(response.div_card)
        response = alice.click(card.reminders[0].cancel())
        assert response.scenario in {scenario.Vins, scenario.Reminders}
        text = response.text.lower()
        for word in ['удалил', 'поесть', 'завтра', '15']:
            assert word in text


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.supported_features('notifications')
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestMementoReminders(object):
    owners = ('petrk', )

    def test_set(self, alice, cleanup_reminder):
        Memento(alice, cleanup_reminder).create_new('поставь напоминение поесть на завтра в 15:00')

    def test_cancel_after_set(self, alice, cleanup_reminder):
        Memento(alice, cleanup_reminder).create_new('поставь напоминение поесть на завтра в 15:00')

        response = alice('отмени это напоминание')
        assert response.scenario in {scenario.Vins, scenario.Reminders}
        assert 'отменила' in response.text.lower()

        assert len(response.voice_response.uniproxy_directives) == 1
        assert 'context_save_directive' in response.voice_response.uniproxy_directives[0]

        assert len(response.voice_response.directives) == 1
        assert 'update_memento' == response.voice_response.directives[0].name

    @pytest.mark.xfail(reason='Допилить когда юзера будут')
    def test_cancel_after_list(self, alice, cleanup_reminder):
        commands = [
            'поставь напоминание поесть на завтра в 3 часа дня.',
            'поставь напоминание покормить котика завтра в час дня.',
            'поставь напоминание покумекать послезавтра в 15:00.',
        ]

        memento = Memento(alice, cleanup_reminder)
        for command in commands:
            memento.create_new(command)

    def test_list(self, alice, cleanup_reminder):
        commands = [
            'поставь напоминание поесть на завтра в 3 часа дня.',
            'поставь напоминание покормить котика завтра в час дня.',
            'поставь напоминание покумекать послезавтра в 15:00.',
        ]

        memento = Memento(alice, cleanup_reminder)
        for command in commands:
            memento.create_new(command)

        response = alice('покажи список напоминаний')

        assert response.text.lower().find('сейчас установлено') != -1

        """
        card = RemindersCard(response.div_card)
        assert len(card.reminders) == 3
        reminder = card.reminders[0]
        assert reminder.title == 'Поесть'
        assert reminder.description == 'завтра в 3 часа дня'
        assert reminder.cancel_action

        reminder = card.reminders[1]
        assert reminder.title == 'Покормить кота'
        assert reminder.description == 'завтра в 5 вечера'
        assert reminder.cancel_action
        """


@pytest.mark.xfail(reason='Эти тесты старых ремайндеров, их надо перенести, но сейчас такой возможности нет т.к. сейчас ремайндеры копяться у пользователя, ждем отдельных юзеров')
@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestReminders(object):
    owners = ('petrk', )

    def test_reminder_ellipsis_time(self, alice):
        response = alice('напомни приготовить ужин')
        assert response.intent == intent.CreateReminder
        assert re.fullmatch(ReminderResponse.WhatTime, response.text)

        response = alice('на завтра в 19 часов')
        assert response.intent == intent.CreateReminderEllipsis
        assert response.text == 'Поставила напоминание \"приготовить ужин\" на завтра в 19:00.'

    def test_reminder_ellipsis_what(self, alice):
        response = alice('поставь напоминание на завтра на 20 часов')
        assert response.intent == intent.CreateReminder
        assert re.fullmatch(ReminderResponse.WhatSubject, response.text)

        response = alice('приготовить ужин')
        assert response.intent == intent.CreateReminderEllipsis
        assert response.text == 'Поставила напоминание \"приготовить ужин\" на завтра в 20:00.'

    def test_reminder_create_and_cancel(self, alice):
        response = alice('поставь напоминание на завтра сходить погулять в 18 часов')
        assert response.intent == intent.CreateReminder
        assert response.text == 'Поставила напоминание \"сходить погулять\" на завтра в 18:00.'

        response = alice('отмени')
        assert response.intent == intent.CreateReminderCancel
        assert response.text.startswith('Отменила это напоминание.')

    def test_reminder_cancel_while_create(self, alice):
        response = alice('напомни приготовить ужин')
        assert response.intent == intent.CreateReminder
        assert re.fullmatch(ReminderResponse.WhatTime, response.text)

        response = alice('отмена')
        assert response.intent == intent.CreateReminderCancel
        assert response.text.startswith('Хорошо, отменила.')

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7472')
    def test_reminder_with_cancel_text(self, alice):
        response = alice('поставь напоминание на завтра 20 часов')
        assert response.intent == intent.CreateReminder
        assert re.fullmatch(ReminderResponse.WhatSubject, response.text)

        response = alice('закончить')
        assert response.intent == intent.CreateReminderEllipsis
        assert response.text == 'Поставила напоминание \"закончить\" на завтра в 20:00.'

        response = alice('Поставь напоминание на завтра на 5 часов хватит это терпеть')
        assert response.intent == intent.CreateReminder
        assert response.text == 'Поставила напоминание \"хватит это терпеть\" на завтра в 5:00.'

    # ALICE-7832
    def test_reminder_regex_tagger_issues_fix(self, alice):
        response = alice('Алиса, поставь напоминание на завтра в девять часов тридцать утра вынести мусор')
        assert response.intent == intent.CreateReminder
        assert response.text == 'Поставила напоминание \"вынести мусор\" на завтра в 9:30.'
