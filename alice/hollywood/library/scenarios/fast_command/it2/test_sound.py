import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import text, voice


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ["fast_command"]


@pytest.fixture(scope="function")
def srcrwr_params():
    return {}


MEMENTO_VOLUME_ONBOARDING = {
    "UserConfigs": {
        "VolumeOnboardingConfig": {
            "UsageCounter": 2,
        },
    },
}

OUT_OF_BOUND_REPLIES = (
    "Шкала громкости - от 1 до 10. Но вы можете управлять ею в процентах, если вам так удобнее.",
)

OUT_OF_BOUND_REPLIES_TV = (
    "Шкала громкости - от 1 до 100. Но вы можете управлять ею в процентах, если вам так удобнее.",
)

ALREADY_MAXIMUM_REPLIES = (
    "Уже максимум.",
    "Громче уже некуда.",
    "Куда уж громче.",
    "Громче уже нельзя.",
    "Соседи говорят что и так всё хорошо слышат.",
)

ALREADY_MINIMUM_REPLIES = (
    "Уже минимум.",
    "Уже и так без звука.",
    "Тише уже некуда.",
    "Куда уж тише.",
    "Тише уже нельзя.",
)

ALREADY_SET_REPLIES = (
    "Хорошо.",
    "Уже сделала.",
    "Звук уже выставлен.",
    "Такой уровень звука уже стоит.",
    "Ничего не изменилось.",
)

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


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.legatus])
class TestLegatusSound:

    def test_set_level_absolute_value(self, alice):
        r = alice(voice("поставь громкость 46"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        assert r.run_response.ResponseBody.Layout.OutputSpeech in NOT_SUPPORTED

    def test_relative_value_louder(self, alice):
        r = alice(voice("громче"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        assert r.run_response.ResponseBody.Layout.OutputSpeech in NOT_SUPPORTED

    def test_relative_value_quiter(self, alice):
        r = alice(voice("тише"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        assert r.run_response.ResponseBody.Layout.OutputSpeech in NOT_SUPPORTED

    def test_mute(self, alice):
        r = alice(voice("выключи звук"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        assert r.run_response.ResponseBody.Layout.OutputSpeech in NOT_SUPPORTED

    def test_unmute(self, alice):
        r = alice(voice("включи звук"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        assert r.run_response.ResponseBody.Layout.OutputSpeech in NOT_SUPPORTED

    def test_simple_get_level(self, alice):
        r = alice(voice("какая сейчас громкость"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        assert r.run_response.ResponseBody.Layout.OutputSpeech in NOT_SUPPORTED


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestTVSound:
    """
    Тесты установки звука для ТВ

    ТВ отличается тем, что у него звуковая шкала 0-100, а не 0-10, как на колонке.
    """

    @pytest.mark.device_state(sound_level=0, sound_max_level=100)
    def test_set_level_absolute_value(self, alice):
        """
        Запрос "громкость на Х" поставит громкость на Х
        """
        r = alice(text("поставь громкость 46"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 46
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=2, sound_max_level=100)
    def test_set_level_absolute_value_zero(self, alice):
        """
        Запрос "громкость 0" поставит громкость на 0
        """
        r = alice(text("поставь громкость 0"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 0
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=46, sound_max_level=100)
    def test_set_level_absolute_value_if_already_set(self, alice):
        """
        Если громкость и так уже 46, то просто setlevel будет
        """
        r = alice(text("поставь громкость 46"))
        assert r.scenario_stages() == {"run"}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 46
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert output_speech in ALREADY_SET_REPLIES

    @pytest.mark.device_state(sound_level=0, sound_max_level=100)
    def test_set_level_absolute_value_more_than_maximum(self, alice):
        """
        Громкость вне допустимого диапазона
        """
        r = alice(text("поставь громкость 120"))
        assert r.scenario_stages() == {"run"}
        assert not r.run_response.ResponseBody.Layout.Directives
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert output_speech in OUT_OF_BOUND_REPLIES_TV

    @pytest.mark.device_state(sound_level=0, sound_max_level=100)
    def test_set_level_with_denominator(self, alice):
        """
        Громкость X из Y
        """
        r = alice(text("поставь громкость 3 из 10"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 30
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('volume', [
        pytest.param('0', marks=pytest.mark.device_state(sound_level=0), id='0'),
        pytest.param('5', marks=pytest.mark.device_state(sound_level=5), id='5'),
        pytest.param('10', marks=pytest.mark.device_state(sound_level=10), id='10'),
    ])
    def test_simple_get_level(self, alice, volume):
        r = alice(text("какая сейчас громкость"))
        assert r.scenario_stages() == {"run"}
        assert not r.run_response.ResponseBody.Layout.Directives
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert volume in output_speech


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestKolonkaSound:
    """
    Тесты установки звука для колонки (шкала громкости 0-10)
    """

    @pytest.mark.device_state(sound_level=3, sound_max_level=10)
    def test_set_level_simple(self, alice):
        r = alice(text("давай поставим громкость 5"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 5
        assert not r.run_response.ResponseBody.Layout.OutputSpeech
        return str(r)

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_set_level_absolute_value_too_high(self, alice):
        r = alice(text("поставь громкость 2000"))
        assert r.scenario_stages() == {"run"}
        assert not r.run_response.ResponseBody.Layout.Directives
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert output_speech in OUT_OF_BOUND_REPLIES

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_set_level_absolute_value_too_high_2(self, alice):
        r = alice(text("поставь громкость 15"))
        assert r.scenario_stages() == {"run"}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 2
        assert not r.run_response.ResponseBody.Layout.OutputSpeech
        return str(r)

    @pytest.mark.device_state(sound_level=9, sound_max_level=10)
    def test_set_level_absolute_value_too_high_3(self, alice):
        r = alice(text("поставь громкость 15"))
        assert r.scenario_stages() == {"run"}
        assert not r.run_response.ResponseBody.Layout.Directives
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert output_speech in OUT_OF_BOUND_REPLIES
        return str(r)

    @pytest.mark.device_state(sound_level=9, sound_max_level=10)
    @pytest.mark.memento(MEMENTO_VOLUME_ONBOARDING)
    def test_set_level_absolute_value_too_high_3_memento(self, alice):
        r = alice(text("поставь громкость 15"))
        assert r.scenario_stages() == {"run"}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 2
        assert not r.run_response.ResponseBody.Layout.OutputSpeech
        return str(r)

    @pytest.mark.device_state(sound_level=8, sound_max_level=10)
    def test_set_level_absolute_value_negative(self, alice):
        """
        Громкость -3 - понимаем как тише на 3
        """
        r = alice(text("поставь громкость -3"))
        assert r.scenario_stages() == {"run"}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 5
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_set_level_absolute_value_more_than_35_less_than_100(self, alice):
        """
        Громкость больше равно 35, но меньше равно 100 - то понимаем это как проценты
        """
        r = alice(text("поставь громкость 80"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 8
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_set_level_with_denominator(self, alice):
        r = alice(text("поставь громкость 2 из 5"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 4
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_set_level_with_maximal_level(self, alice):
        r = alice(text("поставь громкость 10"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 10
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=10, sound_max_level=10)
    def test_set_maximum_level_if_already_maximum(self, alice):
        r = alice(text("поставь громкость 10"))
        assert r.scenario_stages() == {"run"}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 10
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert output_speech in ALREADY_MAXIMUM_REPLIES

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_set_minimum_level_if_already_minimum(self, alice):
        r = alice(text("поставь громкость 0"))
        assert r.scenario_stages() == {"run"}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 0
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert output_speech in ALREADY_MINIMUM_REPLIES

    @pytest.mark.device_state(sound_level=3, sound_max_level=10)
    def test_set_level_ellipsis(self, alice):
        r = alice(text("уровень громкости"))
        r = alice(text("давай 5"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 5
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=0)
    def test_simple_get_level(self, alice):
        r = alice(text("какая сейчас громкость"))
        assert r.scenario_stages() == {"run"}
        assert not r.run_response.ResponseBody.Layout.Directives
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert "0" in output_speech


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestNaturalLanguageSetLevel:
    """
    Тесты обработки значений звука вроде "средняя громкость"
    """

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_set_human_level_kolonka(self, alice):
        """
        На колонке "средняя громкость" поставит 4
        """
        r = alice(text("поставь среднюю громкость"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 4
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=0, sound_max_level=100)
    def test_set_human_level_tv(self, alice):
        """
        На ТВ со шкалой 100 "средняя громкость" поставит 40 (?)
        """
        r = alice(text("поставь среднюю громкость"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 40
        assert not r.run_response.ResponseBody.Layout.OutputSpeech


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestLouderKolonka:
    """
    Тесты просьб сделать погромче на колонке
    """

    @pytest.mark.device_state(sound_level=6, sound_max_level=10)
    def test_louder(self, alice):
        """
        Погромче на колонке шагает в 1 шаг
        """
        r = alice(text("громче"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundLouderDirective.Name == "sound_louder"
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.supported_features('relative_volume_change')
    @pytest.mark.device_state(sound_level=6, sound_max_level=10)
    def test_relative_louder(self, alice):
        """
        Погромче на колонке с относительной фичей возвращает sound_louder
        """
        r = alice(text("громче"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundLouderDirective.Name == "sound_louder"
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=6, sound_max_level=10)
    def test_quiter(self, alice):
        """
        Потише на колонке шагает в 1 шаг
        """
        r = alice(text("тише"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundQuiterDirective.Name == "sound_quiter"
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.supported_features('relative_volume_change')
    @pytest.mark.device_state(sound_level=6, sound_max_level=10)
    def test_relative_quiter(self, alice):
        """
        Потише на колонке шагает в 1 шаг
        """
        r = alice(text("тише"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundQuiterDirective.Name == "sound_quiter"
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=4, sound_max_level=10)
    def test_quiter_degree(self, alice):
        """
        Алиса понимает слово немного
        """
        r = alice(text("немного тише"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 3
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=10, sound_max_level=10)
    def test_louder_if_already_maximum(self, alice):
        """
        Громче, если уже максимум, возвращает спич
        """
        r = alice(text("громче"))
        assert r.scenario_stages() == {"run"}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundLouderDirective.Name == "sound_louder"
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_quiter_if_already_mimimum(self, alice):
        """
        Тише, если уже минимум, возвращает спич
        """
        r = alice(text("тише"))
        assert r.scenario_stages() == {"run"}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundQuiterDirective.Name == "sound_quiter"
        assert not r.run_response.ResponseBody.Layout.OutputSpeech


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.smart_tv])
class TestLouderTV:
    """
    Тесты просьб сделать погромче на ТВ
    """

    @pytest.mark.device_state(sound_level=10, sound_max_level=100)
    def test_louder(self, alice):
        """
        Погромче на ТВ идет с шагом в 5%
        """
        r = alice(text("громче"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 15
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=21, sound_max_level=100)
    def test_quiter(self, alice):
        """
        Потише на ТВ идет с шагом в 5%
        """
        r = alice(text("тише"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 16
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=10, sound_max_level=100)
    def test_louder_degree_small(self, alice):
        """
        Немного громче на ТВ - это плюс один шаг
        """
        r = alice(text("немного громче"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 15
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=10, sound_max_level=100)
    def test_louder_degree_medium(self, alice):
        """
        Побольше громче на ТВ - это плюс два шага
        """
        r = alice(text("побольше громче"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 20
        assert not r.run_response.ResponseBody.Layout.OutputSpeech


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestLouderFromZero:
    """
    Включение звука, если сейчас уровень 0
    """

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_louder_percent(self, alice):
        r = alice(text("громче на 30 процентов"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 3
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_louder_factor(self, alice):
        r = alice(text("громче в три раза"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 3
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=0, sound_max_level=10)
    def test_louder_degree(self, alice):
        r = alice(text("сильно громче"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 4
        assert not r.run_response.ResponseBody.Layout.OutputSpeech


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestLouderAbsolute:
    """
    Просьбы изменить громкость на абсолютное значение
    """

    @pytest.mark.device_state(sound_level=3, sound_max_level=10)
    def test_quiter_absolute(self, alice):
        r = alice(text("тише на 2"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 1
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_quiter_absolute_mutes(self, alice):
        r = alice(text("тише на 4"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 0
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=4, sound_max_level=10)
    def test_quiter_relative(self, alice):
        r = alice(text("тише в два раза"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 2
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_quiter_float(self, alice):
        r = alice(text("тише на 1.5"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 1
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    # вот это поведение непредсказуемое: уменьши на 0.5
    # на самом деле ставит половину громкости от текущей
    @pytest.mark.device_state(sound_level=8, sound_max_level=10)
    def test_quiter_float_above_one(self, alice):
        r = alice(text("тише на 0.5"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 4
        assert not r.run_response.ResponseBody.Layout.OutputSpeech


@pytest.mark.scenario(name="Commands", handle="fast_command")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestLouderRelative:
    """
    Регулировка громкости на процент
    """

    @pytest.mark.device_state(sound_level=10, sound_max_level=10)
    def test_quiter_percent(self, alice):
        r = alice(text("тише на 30 процентов"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 7
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=1, sound_max_level=10)
    def test_louder_small_percent(self, alice):
        """
        Громче на 1% от единицы сделает двойку, а не оставит единицу

        Потому что пользователь просит именно громче, нельзя оставлять звук
        как есть.
        """
        r = alice(text("громче на 1 процент"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 2
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=7, sound_max_level=10)
    def test_quiter_should_not_mute(self, alice):
        """
        Тише на 99% оставляет звук
        """
        r = alice(text("тише на 99 процентов"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 1
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=1, sound_max_level=10)
    def test_quiter_from_one_should_mute(self, alice):
        """
        Тише от единицы выключает звук
        """
        r = alice(text("тише на 30 процентов"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 0
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_quiter_degree_beyond_the_limit(self, alice):
        """
        Девайс не мьютится
        """
        r = alice(text("сильно тише"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 1
        assert not r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.device_state(sound_level=1, sound_max_level=10)
    def test_quiter_degree_from_one(self, alice):
        """
        Девайс мьютится от единицы
        """
        r = alice(text("сильно тише"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].SoundSetLevelDirective.NewLevel == 0
        assert not r.run_response.ResponseBody.Layout.OutputSpeech
