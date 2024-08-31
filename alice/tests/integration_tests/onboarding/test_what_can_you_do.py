import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.version(hollywood=210, megamind=245)
@pytest.mark.oauth(auth.YandexPlus)
class _TestWhatCanYouDo(object):
    owners = (
        'karina-usm',
        'abc:alisa_skill_recommendation',
    )


@pytest.mark.parametrize('surface', [surface.station, surface.station_pro])
@pytest.mark.parametrize('channels', [
    pytest.param(
        'РБК или программу ТНТ',
        marks=pytest.mark.device_state(is_tv_plugged_in=True),
    ),
    pytest.param(
        'Первому каналу или ТНТ',
        marks=pytest.mark.device_state(is_tv_plugged_in=False),
    ),
])
class TestWhatCanYouDoQuasarMain(_TestWhatCanYouDo):

    first_main_screen_phrase = 'Вы можете поставить таймер или будильник. Например, скажите: ' \
                               '"Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".'

    def test_main_screen(self, alice, channels):
        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith(self.first_main_screen_phrase)

        response = alice('Еще')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.')

        response = alice('Дальше')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith(f'Я знаю телепрограмму, вы всегда можете спросить меня, что идет по {channels}.')

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith(self.first_main_screen_phrase)

        response = alice('Нет')
        assert response.scenario == scenario.DoNothing
        assert not response.text
        assert not response.has_voice_response()
        assert not response.output_speech_text

        response = alice('Домой')

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith(self.first_main_screen_phrase)


@pytest.mark.parametrize('surface', [surface.station, surface.station_pro])
@pytest.mark.device_state(is_tv_plugged_in=True)
class TestWhatCanYouDoQuasarWithScreen(_TestWhatCanYouDo):

    @pytest.mark.experiments('hw_what_can_you_do_switch_phrases')
    def test_radio_player_screen(self, alice):
        response = alice('Включи радио')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Чтобы переключаться между радиостанциями, говорите «следующая станция» или «предыдущая станция».')

        response = alice('Дальше')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Скажите название станции или назовите частоту. Радио можно ставить на паузу, чтобы включить позже. '
                                        'К сожалению, перемотать ничего не получится — это прямой эфир.')

    @pytest.mark.version_lt(hollywood=213)
    @pytest.mark.experiments('hw_what_can_you_do_main_phrases')
    def test_main_phrases(self, alice):
        response = alice('Включи радио')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Вы можете поставить таймер или будильник. Например, скажите: '
                                        '"Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".')

    @pytest.mark.version(hollywood=213)
    def test_main_phrases_no_old_flag(self, alice):
        response = alice('Включи радио')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Вы можете поставить таймер или будильник. Например, скажите: '
                                        '"Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".')

    @pytest.mark.experiments('hw_what_can_you_do_switch_phrases')
    def test_video_search_screen(self, alice):
        response = alice('Видео про котиков')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.MordoviaShowDirective

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Вы можете перейти к нужному видео, сказав, например, «запусти номер один», или просто произнеся его название.')

        response = alice('Дальше')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Вы можете листать список видео, просто сказав «дальше» или «назад».')

        response = alice('Рассказывай еще')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Вы можете перейти к нужному видео, сказав, например, «запусти номер один», или просто произнеся его название.')

    @pytest.mark.experiments('hw_what_can_you_do_switch_phrases')
    def test_channels_screen(self, alice):
        response = alice('Что по тв')
        assert response.directive.name == directives.names.ShowTvGalleryDirective

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Здесь вы можете смотреть каналы и подборки передач из онлайн-телевидения Яндекса — Яндекс Эфира')

        response = alice('Еще')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Найдите нужный канал в галерее и скажите мне его название или номер. '
                                        'Чтобы листать галерею, говорите «дальше», «назад», «в начало» или «в конец».')

        response = alice('Включи номер 2')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.type == 'tv_stream'

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith(
            'Вы можете нажать паузу и вернуться к просмотру позже. '
            'Но это прямой эфир, так что перемотать в нужное место не выйдет.'
        )


@pytest.mark.parametrize('surface', [surface.station, surface.station_pro])
@pytest.mark.device_state(is_tv_plugged_in=True)
class TestWhatCanYouDoQuasarCommon(_TestWhatCanYouDo):

    @pytest.mark.experiments('hw_what_can_you_do_switch_phrases')
    @pytest.mark.experiments('hw_what_can_you_do_dont_stop_on_decline')
    def test_dont_stop_on_decline(self, alice):
        response = alice('Включи радио')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RadioPlay

        response = alice('Что ты умеешь')
        assert response.scenario == scenario.Onboarding
        assert response.intent == intent.OnboardingWhatCanYouDo
        assert response.text.startswith('Чтобы переключаться между радиостанциями, говорите «следующая станция» или «предыдущая станция».')

        response = alice('Хватит')
        assert response.scenario == scenario.DoNothing
        assert not response.text
        assert not response.has_voice_response()
        assert not response.output_speech_text
        assert not response.directives
