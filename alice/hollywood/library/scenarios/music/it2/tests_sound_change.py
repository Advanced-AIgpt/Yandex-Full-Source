import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import text
from alice.megamind.protos.scenarios.directives_pb2 import TAudioPlayDirective


EXPERIMENTS = [
    'hw_music_thin_client',
    'mm_enable_stack_engine',
]

EXPERIMENTS_GENERATIVE = [
    'bg_fresh_granet',
]


def _assert_audio_play_directive(directive, title=None, sub_title=None, has_callbacks=True, offset_ms=None, is_generative=False):
    audio_play = directive.AudioPlayDirective
    assert audio_play.Name == 'music'
    assert audio_play.Stream.Id
    if is_generative:
        assert audio_play.Stream.StreamFormat == TAudioPlayDirective.TStream.TStreamFormat.HLS
    else:
        assert audio_play.Stream.StreamFormat == TAudioPlayDirective.TStream.TStreamFormat.MP3
    assert audio_play.Stream.Url.startswith('https://')
    if has_callbacks:
        assert audio_play.Callbacks.HasField('OnPlayStartedCallback')
        assert audio_play.Callbacks.HasField('OnPlayStoppedCallback')
        assert audio_play.Callbacks.HasField('OnPlayFinishedCallback')
        assert audio_play.Callbacks.HasField('OnFailedCallback')
    assert audio_play.ScenarioMeta['owner'] == 'music'
    assert audio_play.ScreenType == TAudioPlayDirective.EScreenType.Music

    assert audio_play.HasField('AudioPlayMetadata')
    if title:
        assert audio_play.AudioPlayMetadata.Title == title
    if sub_title:
        assert audio_play.AudioPlayMetadata.SubTitle == sub_title

    if offset_ms is not None:
        assert audio_play.Stream.OffsetMs >= offset_ms

    return audio_play


def _assert_music_play_directive(directive, output_speech=None):
    music_play = directive.MusicPlayDirective
    assert music_play.SessionId.strip()
    assert not music_play.RoomId


@pytest.mark.experiments(*EXPERIMENTS_GENERATIVE)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
class TestsBase:
    pass


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS)
class TestsSoundChangeOnThinClient(TestsBase):
    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_set_level_absolute_value(self, alice):
        """
        Запрос "включи Metallica на громкости X" включит Metallica и поставит громкость на Х
        """
        r = alice(text("включи Metallica на громкости 7"))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].SoundSetLevelDirective.NewLevel == 7
        assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        _assert_audio_play_directive(directives[2])

    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_set_level_degree_value(self, alice):
        """
        Запрос "включи Kanye West на всю" включит Kanye West и поставит громкость 10
        """
        r = alice(text("включи Kanye West на всю"))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].SoundSetLevelDirective.NewLevel == 10
        assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        _assert_audio_play_directive(directives[2])

    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_louder_default(self, alice):
        """
        Запрос "включи Kanye West погромче" включит Kanye West и увеличит громкость на дефолтное значение (на 2)
        """
        r = alice(text("включи Kanye West погромче"))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].SoundSetLevelDirective.NewLevel == 4
        assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        _assert_audio_play_directive(directives[2])

    @pytest.mark.device_state(sound_level=5, sound_max_level=10)
    def test_quiter_default(self, alice):
        """
        Запрос "включи Kanye West потише" включит Kanye West и уменьшит громкость на дефолтное значение (на 2)
        """
        r = alice(text("включи Kanye West потише"))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].SoundSetLevelDirective.NewLevel == 3
        assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        _assert_audio_play_directive(directives[2])


@pytest.mark.parametrize('surface', [surface.station])
class TestsSoundChangeOnMusicClient(TestsBase):
    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_set_level_absolute_value(self, alice):
        """
        Запрос "включи Metallica на громкости X" включит Metallica и поставит громкость на Х
        """
        r = alice(text("включи Metallica на громкости 7"))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].SoundSetLevelDirective.NewLevel == 7
        assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        _assert_music_play_directive(directives[2])

    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_set_level_degree_value(self, alice):
        """
        Запрос "включи Kanye West на всю" включит Kanye West и поставит громкость 10
        """
        r = alice(text("включи Kanye West на всю"))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].SoundSetLevelDirective.NewLevel == 10
        assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        _assert_music_play_directive(directives[2])

    @pytest.mark.device_state(sound_level=2, sound_max_level=10)
    def test_louder_default(self, alice):
        """
        Запрос "включи Kanye West погромче" включит Kanye West и увеличит громкость на дефолтное значение (на 2)
        """
        r = alice(text("включи Kanye West погромче"))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].SoundSetLevelDirective.NewLevel == 4
        assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        _assert_music_play_directive(directives[2])

    @pytest.mark.device_state(sound_level=5, sound_max_level=10)
    def test_quiter_default(self, alice):
        """
        Запрос "включи Kanye West потише" включит Kanye West и уменьшит громкость на дефолтное значение (на 2)
        """
        r = alice(text("включи Kanye West потише"))
        assert r.scenario_stages() == {'run', 'continue'}

        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 3
        assert directives[0].SoundSetLevelDirective.NewLevel == 3
        assert directives[1].TtsPlayPlaceholderDirective.Name == 'tts_play_placeholder'
        _assert_music_play_directive(directives[2])
