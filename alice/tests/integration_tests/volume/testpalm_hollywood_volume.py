import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def check_voice_and_text(response, target=None):
    assert not response.has_voice_response()
    if not target:
        assert not response.text
    else:
        assert response.text in target

ALREADY_MIN = ['Уже минимум.', 'Уже и так без звука.', 'Тише уже некуда.', 'Куда уж тише.', 'Тише уже нельзя.']
ALREADY_MAX = ['Уже максимум.', 'Громче уже некуда.', 'Куда уж громче.', 'Громче уже нельзя.', 'Соседи говорят что и так всё хорошо слышат.']
ALREADY_SET = ['Хорошо.', 'Уже сделала.', 'Звук уже выставлен.', 'Такой уровень звука уже стоит.', 'Ничего не изменилось.']
INCORRECT_LEVEL = ['Больше 10 не могу.', 'Попробуйте число от 0 до 10.']
SOUND_DONE = ['Сделала.', 'Готово.', 'Как скажете.']
SOUND_DONE_MUTE = ['Ок, выключаю звук.', 'Хорошо, выключаю звук.', 'Сейчас выключу звук.']
SOUND_DONE_UNMUTE = ['Ок, включаю звук.', 'Хорошо, включаю звук.', 'Сейчас включу звук.']
NOT_SUPPORTED = [
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


class TestSoundCommands(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-605
    https://testpalm.yandex-team.ru/testcase/alice-3149
    https://testpalm.yandex-team.ru/testcase/alice-2695

    old_automotive surface:
    Частично https://testpalm.yandex-team.ru/testcase/alice-1800
    """

    owners = ('nkodosov', 'makatunkin', 'tolyandex', )

    @pytest.mark.parametrize('surface', [surface.station])
    def test_station(self, alice):
        response = alice('поставь громкость 4')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 4
        check_voice_and_text(response)

        response = alice('громче')
        assert response.directive.name == directives.names.SoundLouderDirective
        assert alice.device_state.SoundLevel == 5
        check_voice_and_text(response)

        response = alice('еще')
        assert response.directive.name == directives.names.SoundLouderDirective
        assert alice.device_state.SoundLevel == 6
        check_voice_and_text(response)

        response = alice('тише')
        assert response.directive.name == directives.names.SoundQuiterDirective
        assert alice.device_state.SoundLevel == 5
        check_voice_and_text(response)

        response = alice('еще')
        assert response.directive.name == directives.names.SoundQuiterDirective
        assert alice.device_state.SoundLevel == 4
        check_voice_and_text(response)

        response = alice('поставь громкость 0')
        response = alice('тише')
        assert response.directive.name == directives.names.SoundQuiterDirective
        assert alice.device_state.SoundLevel == 0

        response = alice('поставь громкость 10')
        response = alice('громче')
        assert response.directive.name == directives.names.SoundLouderDirective
        assert alice.device_state.SoundLevel == 10

        response = alice('поставь среднюю громкость')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 4
        check_voice_and_text(response)
        response = alice('поставь среднюю громкость')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 4
        check_voice_and_text(response, ALREADY_SET)

        response = alice('вруби на всю')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 10
        check_voice_and_text(response)

        response = alice('поставь громкость -7')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 3
        check_voice_and_text(response)

        response = alice('поставь громкость 17')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 2
        check_voice_and_text(response)

        response = alice('поставь громкость 80')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 8
        check_voice_and_text(response)

        response = alice('выключи звук')
        assert response.directive.name == directives.names.SoundMuteDirective
        assert alice.device_state.SoundLevel == 8
        assert alice.device_state.SoundMuted
        check_voice_and_text(response)

        response = alice('включи звук')
        assert response.directive.name == directives.names.SoundUnmuteDirective
        assert alice.device_state.SoundLevel == 8
        assert not alice.device_state.SoundMuted
        check_voice_and_text(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_searchapp(self, alice):
        response = alice('поставь громкость 4')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 4
        check_voice_and_text(response, SOUND_DONE)

        response = alice('громче')
        assert response.directive.name == directives.names.SoundLouderDirective
        assert alice.device_state.SoundLevel == 5
        check_voice_and_text(response, SOUND_DONE)

        response = alice('еще')
        assert response.directive.name == directives.names.SoundLouderDirective
        assert alice.device_state.SoundLevel == 6
        check_voice_and_text(response, SOUND_DONE)

        response = alice('тише')
        assert response.directive.name == directives.names.SoundQuiterDirective
        assert alice.device_state.SoundLevel == 5
        check_voice_and_text(response, SOUND_DONE)

        response = alice('еще')
        assert response.directive.name == directives.names.SoundQuiterDirective
        assert alice.device_state.SoundLevel == 4
        check_voice_and_text(response, SOUND_DONE)

        response = alice('поставь громкость 0')
        response = alice('тише')
        assert response.directive.name == directives.names.SoundQuiterDirective
        assert alice.device_state.SoundLevel == 0

        response = alice('поставь громкость 10')
        response = alice('громче')
        assert response.directive.name == directives.names.SoundLouderDirective
        assert alice.device_state.SoundLevel == 10

        response = alice('поставь среднюю громкость')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 4
        check_voice_and_text(response, SOUND_DONE)
        response = alice('поставь среднюю громкость')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 4
        check_voice_and_text(response, ALREADY_SET)

        response = alice('вруби на всю')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 10
        check_voice_and_text(response, SOUND_DONE)

        response = alice('поставь громкость -7')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 3
        check_voice_and_text(response, SOUND_DONE)

        response = alice('поставь громкость 17')
        assert not response.directive
        assert alice.device_state.SoundLevel == 3
        check_voice_and_text(response, INCORRECT_LEVEL)

        response = alice('поставь громкость 80')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 8
        check_voice_and_text(response, SOUND_DONE)

        response = alice('выключи звук')
        assert response.directive.name == directives.names.SoundMuteDirective
        assert alice.device_state.SoundLevel == 8
        assert alice.device_state.SoundMuted
        check_voice_and_text(response, SOUND_DONE_MUTE)

        response = alice('включи звук')
        assert response.directive.name == directives.names.SoundUnmuteDirective
        assert alice.device_state.SoundLevel == 8
        assert not alice.device_state.SoundMuted
        check_voice_and_text(response, SOUND_DONE_UNMUTE)

    @pytest.mark.parametrize('surface', [surface.navi])
    def test_navi(self, alice):
        response = alice('поставь громкость 4')
        check_voice_and_text(response, NOT_SUPPORTED)

        response = alice('громче')
        check_voice_and_text(response, NOT_SUPPORTED)

        response = alice('тише')
        check_voice_and_text(response, NOT_SUPPORTED)

        response = alice('выключи звук')
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://set_setting?name=soundNotifications&value=Alerts'
        assert alice.device_state.SoundMuted
        check_voice_and_text(response, SOUND_DONE_MUTE)

        response = alice('включи звук')
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://set_setting?name=soundNotifications&value=All'
        assert not alice.device_state.SoundMuted
        check_voice_and_text(response, SOUND_DONE_UNMUTE)

    @pytest.mark.parametrize('surface', [surface.automotive])
    def test_auto(self, alice):
        response = alice('поставь громкость 4')
        check_voice_and_text(response, NOT_SUPPORTED)

        response = alice('громче')
        assert alice.device_state.SoundLevel == 1
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexauto://sound?action=volume_up'
        check_voice_and_text(response)

        response = alice('тише')
        assert alice.device_state.SoundLevel == 0
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexauto://sound?action=volume_down'

        response = alice('выключи звук')
        assert alice.device_state.SoundMuted
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexauto://sound?action=mute'
        check_voice_and_text(response)

        response = alice('включи звук')
        assert not alice.device_state.SoundMuted
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexauto://sound?action=unmute'
        check_voice_and_text(response)

    @pytest.mark.parametrize('surface', [surface.old_automotive])
    def test_old_auto(self, alice):
        response = alice('поставь громкость 4')
        check_voice_and_text(response, NOT_SUPPORTED)

        response = alice('громче')
        assert alice.device_state.SoundLevel == 1
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.intent == 'volume_up'
        check_voice_and_text(response)

        response = alice('тише')
        assert alice.device_state.SoundLevel == 0
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.intent == 'volume_down'

        response = alice('выключи звук')
        check_voice_and_text(response, NOT_SUPPORTED)

        response = alice('включи звук')
        check_voice_and_text(response, NOT_SUPPORTED)

    @pytest.mark.parametrize('surface', [surface.watch])
    def test_watch(self, alice):
        response = alice('Сделай погромче')
        check_voice_and_text(response, NOT_SUPPORTED)

    @pytest.mark.version(hollywood=187, megamind=220)
    @pytest.mark.parametrize('surface', [surface.legatus])
    def test_legatus(self, alice):
        response = alice('Сделай погромче')
        check_voice_and_text(response, NOT_SUPPORTED)

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
    def test_alice_605(self, alice):
        response = alice('поставь громкость 5')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 5
        check_voice_and_text(response)

        response = alice('Включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective

        sound_level = alice.device_state.SoundLevel

        for command in ['громче', 'сделай громче', 'погромче', 'увеличить громкость']:
            response = alice(command)
            sound_level += 1
            assert response.directive.name == directives.names.SoundLouderDirective
            assert alice.device_state.SoundLevel == sound_level
            check_voice_and_text(response)

        for command in ['тише', 'сделай потише', 'потише', 'уменьшить громкость']:
            response = alice(command)
            sound_level -= 1
            assert response.directive.name == directives.names.SoundQuiterDirective
            assert alice.device_state.SoundLevel == sound_level
            check_voice_and_text(response)

        response = alice('уровень громкости 9')
        assert response.directive.name == directives.names.SoundSetLevelDirective
        assert alice.device_state.SoundLevel == 9
        check_voice_and_text(response)

        response = alice('Какая сейчас громкость?')
        assert response.scenario == scenario.Commands
        assert response.directive is None
        assert alice.device_state.SoundLevel == 9
        assert response.text in [
            str(alice.device_state.SoundLevel),
            f'Сейчас {alice.device_state.SoundLevel}',
            f'Сейчас громкость {alice.device_state.SoundLevel}',
            f'Текущий уровень громкости {alice.device_state.SoundLevel}',
        ]

        response = alice('выключи звук')
        assert response.directive.name == directives.names.SoundMuteDirective
        assert alice.device_state.SoundLevel == 9
        assert alice.device_state.SoundMuted
        check_voice_and_text(response)

        response = alice('включи звук')
        assert response.directive.name == directives.names.SoundUnmuteDirective
        assert alice.device_state.SoundLevel == 9
        assert not alice.device_state.SoundMuted
        check_voice_and_text(response)
