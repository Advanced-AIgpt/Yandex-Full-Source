import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import alice.tests.library.intent as intent
import pytest


def assert_audio_play_directive(audio_play, title=None, subtitle=None, subtitle_re=None, offset_ms=0, offset_ms_err=10000):
    assert audio_play.name == directives.names.AudioPlayDirective

    if title:
        assert audio_play.payload.metadata.title == title
    if subtitle:
        assert audio_play.payload.metadata.subtitle == subtitle
    if subtitle_re:
        assert re.match(subtitle_re, audio_play.payload.metadata.subtitle), \
               f'subtitle={audio_play.payload.metadata.subtitle}'

    for callback_name in ['on_failed', 'on_finished', 'on_started', 'on_stopped']:
        assert callback_name in audio_play.payload.callbacks

    assert offset_ms <= audio_play.payload.stream.offset_ms < offset_ms + offset_ms_err
    assert audio_play.payload.stream.url.startswith('https://')


def assert_audio_rewind_directive(directive, type_, amount_ms=0):
    assert directive.name == directives.names.AudioRewindDirective
    assert directive.payload.type == type_
    assert directive.payload.amount_ms == amount_ms


def assert_response(response, text=None, text_re=None, sub_text=None, text_is_none=False, scenario=None,
                    directive=None):
    if text_is_none:
        assert response.text is None
    if text:
        assert response.text == text
    if text_re:
        assert re.match(text_re, response.text), f'`{response.text}` does not match `{text_re}`'
    if sub_text:
        assert sub_text in response.text

    if scenario:
        assert response.scenario == scenario

    if directive:
        assert response.directive.name == directive


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestMusicThinClient(object):

    owners = ('vitvlkv', 'stupnik', 'amullanurov',)

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи queen', r'Включаю.*', 'Queen', id='artist'),
        pytest.param('включи песню sia cheap thrills', r'Включаю.*', 'Sia', id='track'),
        pytest.param('включи альбом dark side of the moon', r'Включаю.*', 'Pink Floyd', id='album'),
        pytest.param('включи музыку', None, None, id='radio my'),
        pytest.param('включи грустную музыку', None, None, id='radio mood'),
        pytest.param('включи музыку для бега', None, None, id='radio activity'),
        pytest.param('включи джаз', None, None, id='radio genre'),
        pytest.param('включи музыку девяностых', r'.*90\-х.*', None, id='radio epoch'),
        pytest.param('включи музыку на французском языке', None, None, id='radio language'),
        pytest.param('включи инструментальную музыку', None, None, id='radio vocal'),
        pytest.param('включи подкаст лайфхакера', r'Включаю подкаст.*', None,
                      id='album podcast', marks=pytest.mark.xfail(reason='For now, Vins wins here')),
        pytest.param('включи мою музыку', None, None, id='playlist my'),
    ])
    def test_play_different_content_types(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert response.directive.name == directives.names.AudioPlayDirective
        assert_audio_play_directive(response.directive, subtitle=subtitle)
        assert response.directives[-1].name == directives.names.MmStackEngineGetNextCallback

        alice.skip(milliseconds=alice.device_state.AudioPlayer.DurationMs)
        assert response.directive.name == directives.names.AudioPlayDirective

    def test_play_artist(self, alice):
        response = alice('включи queen')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen')

        first_track = alice.device_state.AudioPlayer.CurrentlyPlaying.Title
        response = alice('что сейчас играет')
        assert_response(response, sub_text=f'Queen, песня "{first_track}"', scenario=scenario.HollywoodMusic)
        alice.skip(milliseconds=alice.device_state.AudioPlayer.DurationMs)

        second_track = alice.device_state.AudioPlayer.CurrentlyPlaying.Title
        response = alice('что сейчас играет')
        assert_response(response, sub_text=f'Queen, песня "{second_track}"', scenario=scenario.HollywoodMusic)
        assert first_track != second_track
        alice.skip(milliseconds=alice.device_state.AudioPlayer.DurationMs)

        third_track = alice.device_state.AudioPlayer.CurrentlyPlaying.Title
        response = alice('что сейчас играет')
        assert_response(response, sub_text=f'Queen, песня "{third_track}"', scenario=scenario.HollywoodMusic)
        assert third_track != first_track and third_track != second_track

    def test_play_artist_pause_next_track(self, alice):
        response = alice('включи queen')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen')
        track_title = response.directive.payload.metadata.title

        alice.skip(seconds=60)

        response = alice('поставь на паузу')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.ClearQueueDirective)

        response = alice('следующий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        # NOTE: Five has a 'We Will Rock You' cover song, so it's also valid
        assert_audio_play_directive(response.directive, subtitle_re='Queen|Five', offset_ms=0)
        assert track_title != response.directive.payload.metadata.title

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи metallica', r'.*(Metallica).*', 'Metallica', id='artist'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    @pytest.mark.parametrize('next_command, prev_command', [
        pytest.param('следующий трек', 'предыдущий трек', id='next_prev_track'),
        pytest.param('дальше', 'назад', id='forward_backward',
                     marks=pytest.mark.xfail(reason='Need to fix HOLLYWOOD-296')),
    ])
    def test_play_music_next_track_previous_track(self, alice, command, next_command, prev_command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)
        track_title = response.directive.payload.metadata.title

        response = alice(next_command)
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)
        assert track_title != response.directive.payload.metadata.title

        response = alice(prev_command)
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)
        assert track_title == response.directive.payload.metadata.title

    def test_play_content1_play_content2_previous_track(self, alice):
        response = alice('включи песню yesterday')
        assert_response(response, sub_text='Yesterday', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')
        first_track_title = response.directive.payload.metadata.title

        response = alice('включи queen')
        assert_response(response, sub_text='Queen', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen')
        assert response.directive.payload.metadata.title != first_track_title

        response = alice('предыдущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')
        assert response.directive.payload.metadata.title == first_track_title

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи queen', r'.*(Queen).*', 'Queen', id='artist'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_pause_continue(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)
        track_title = response.directive.payload.metadata.title

        alice.skip(seconds=60)

        response = alice('поставь на паузу')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.ClearQueueDirective)

        response = alice('продолжи')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle, offset_ms=60000)
        assert track_title == response.directive.payload.metadata.title

    @pytest.mark.parametrize('command, text_re, subtitle, text_re2', [
        pytest.param(
            'включи песню yesterday',
             r'.*(Beatles).*',
             'The Beatles',
             r'.*The Beatles, песня \"Yesterday\".*',
             id='track',
        ),
        pytest.param(
            'включи веселую музыку',
             r'.+', None,
             r'(Это|Сейчас играет)*.',
             id='mood',
        ),
    ])
    def test_play_music_what_is_playing(self, alice, command, text_re, subtitle, text_re2):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)

        response = alice('что сейчас играет')
        assert_response(response, text_re=text_re2, scenario=scenario.HollywoodMusic)
        assert not response.directive

    def test_play_track_pause_what_is_playing(self, alice):
        response = alice('включи песню yesterday')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')

        response = alice('поставь на паузу')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.ClearQueueDirective)

        alice.skip(seconds=60)

        response = alice('что сейчас играет')
        assert response.scenario == scenario.Vins, 'This is Vins\'s Shazamilka'
        assert response.text in [
            'Дайте-ка прислушаться...',
            'Минуточку...',
            'Сейчас узнаем',
            'Внимательно слушаю...',
            'Тихо, сейчас узнаем',
        ]

    @pytest.mark.xfail(reason='Надо в директивах music_play, radio_play и т.п. прописывать в device_state CurrentlyPlaying')
    @pytest.mark.parametrize('second_command, text_re, second_directive_name', [
        pytest.param('включи шум дождя', r'.*[Дд]ожд.*', directives.names.MusicPlayDirective, id='vins_music'),
        pytest.param('включи детское радио', r'.*детское радио.*', directives.names.RadioPlayDirective, id='fm_radio'),
    ])
    def test_what_is_playing_supports_player_features1(self, alice, second_command, text_re, second_directive_name):
        '''
        Команды "что сейчас играет" винса и голливуда должны возвращать player_features.
        Выигрывает на постклассификаторе тот, кто позже всех включал плеер.
        '''
        response = alice('включи beatles')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')

        response = alice(second_command)
        assert response.scenario != scenario.HollywoodMusic
        assert response.directive.name == second_directive_name

        response = alice('что сейчас играет')
        assert_response(response, text_re=text_re)
        assert response.scenario != scenario.HollywoodMusic
        assert not response.directive

    @pytest.mark.xfail(reason='Надо в директивах music_play, radio_play и т.п. прописывать в device_state CurrentlyPlaying')
    @pytest.mark.parametrize('first_command, first_directive_name', [
        pytest.param('включи шум дождя', directives.names.MusicPlayDirective, id='vins_music'),
        pytest.param('включи радио хит фм', directives.names.RadioPlayDirective, id='fm_radio'),
    ])
    def test_what_is_playing_supports_player_features2(self, alice, first_command, first_directive_name):
        '''
        Команды "что сейчас играет" винса и голливуда должны возвращать player_features.
        Выигрывает на постклассификаторе тот, кто позже всех включал плеер.
        '''
        response = alice(first_command)
        assert response.scenario != scenario.HollywoodMusic
        assert response.directive.name == first_directive_name

        response = alice('включи beatles')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')

        response = alice('что сейчас играет')
        assert_response(response, text_re=r'.*[Bb]eatles.*', scenario=scenario.HollywoodMusic)
        assert not response.directive

    # TODO(vitvlkv): Consider adding test cases that involve biometry, e.g.
    # - make Alice to remember owner's voice
    # - test_play_artist_like/dislike with the owner's voice
    # - test_play_artist_like/dislike with some guest's voice
    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи queen', r'.*(Queen).*', 'Queen', id='artist'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_like(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)

        response = alice('поставь лайк')
        assert any([sub_text in response.text for sub_text in [
            'лайк', 'вы оценили', 'вам такое по душе', 'Буду включать такое чаще.',
        ]]), f'response.text={response.text}'
        assert response.scenario == scenario.HollywoodMusic
        assert not response.directive

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи queen', r'.*(Queen).*', 'Queen', id='artist'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_dislike(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)
        disliked_track_title = response.directive.payload.metadata.title

        response = alice('поставь дизлайк')
        assert any([sub_text in response.text for sub_text in [
            'дизлайк', 'Дизлайк', 'не буду', 'не включу',
        ]]), f'response.text={response.text}'
        assert response.scenario == scenario.HollywoodMusic
        assert_audio_play_directive(response.directive)  # Dislike command switches playback to the next track
        assert response.directive.payload.metadata.title != disliked_track_title

    def test_play_track_dislike(self, alice):
        response = alice('включи let it be')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        response = alice('поставь дизлайк')
        assert any([sub_text in response.text for sub_text in [
            'дизлайк', 'Дизлайк', 'не буду', 'не включу',
        ]]), f'response.text={response.text}'
        assert_response(response, scenario=scenario.HollywoodMusic,
                        directive=directives.names.MmStackEngineGetNextCallback)
        # NOTE: Включение радио происходит через клиент hop при помощи паровозных механизмов

        response.next()
        assert response.directive.name == directives.names.AudioPlayDirective, 'Должен включиться какой-то похожий трек'

    def test_play_album_shuffled_next_track(self, alice):
        response = alice('включи альбом dark side of the moon вперемешку')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Pink Floyd')
        assert response.directive.payload.metadata.title != 'Speak To Me', \
               'Because \'Speak To Me\' is the first track of this album. Yes, this can flap'

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Pink Floyd')
        assert response.directive.payload.metadata.title != 'Breathe (In The Air)', \
               'Because \'Breathe\' is the second track of this album. Yes, this can flap'

    def test_play_artist_shuffled_next_track(self, alice):
        response = alice('включи audioslave вперемешку')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Audioslave')
        assert response.directive.payload.metadata.title != 'Be Yourself', \
               'Трек "Be Yourself" первый по списку у артиста Audioslave. Может флапать...'

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Audioslave')
        assert response.directive.payload.metadata.title != 'Like a Stone', \
               'Трек "Like a Stone" второй по списку у артиста Audioslave. Может флапать...'

    def test_play_album_shuffle_next_track(self, alice):
        response = alice('включи альбом dark side of the moon')
        assert_response(response, text='Включаю: Pink Floyd, альбом "The Dark Side Of The Moon".',
                        scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, title='Speak To Me', subtitle='Pink Floyd')

        alice.skip(seconds=60)

        response = alice('перемешай')
        assert any([sub_text in response.text for sub_text in [
            'Перемешала', 'беспорядок', 'Ок', 'вперемешку', 'полный винегрет'
        ]]), f'response.text={response.text}'
        assert response.scenario == scenario.HollywoodMusic
        assert len(response.directives) == 1
        set_glagol_metadata = response.directives[0]
        assert set_glagol_metadata.name == directives.names.SetGlagolMetadataDirective
        assert set_glagol_metadata.payload.glagol_metadata.music_metadata.shuffled

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Pink Floyd')
        assert response.directive.payload.metadata.title != 'Breathe (In The Air)', \
               'Because \'Breathe\' is the second track of this album. Yes, this can flap'

    def test_play_artist_shuffle_next_track(self, alice):
        response = alice('включи audioslave')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Audioslave')
        assert response.directive.payload.metadata.title == 'Be Yourself', \
               'Трек "Be Yourself" первый по списку у артиста Audioslave'

        response = alice('перемешай')
        assert any([sub_text in response.text for sub_text in [
            'Перемешала', 'беспорядок', 'Ок', 'вперемешку', 'полный винегрет'
        ]]), f'response.text={response.text}'
        assert response.scenario == scenario.HollywoodMusic
        assert len(response.directives) == 1
        set_glagol_metadata = response.directives[0]
        assert set_glagol_metadata.name == directives.names.SetGlagolMetadataDirective
        assert set_glagol_metadata.payload.glagol_metadata.music_metadata.shuffled

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Audioslave')
        assert response.directive.payload.metadata.title != 'Like a Stone', \
               'Трек "Like a Stone" второй по списку у артиста Audioslave. Может флапать...'

    def test_play_mood_shuffle_next_track(self, alice):
        response = alice('включи веселую музыку')
        assert_response(response, text_re=r'.*(весёлое настроение|веселого настроения).*',
                        scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        response = alice('перемешай')
        assert any([sub_text in response.text for sub_text in [
            'А я уже.', 'Да тут и так все вперемешку.', 'Ок, еще раз пропустила через блендер.'
        ]]), f'response.text={response.text}'
        assert response.scenario == scenario.HollywoodMusic
        assert not response.directive, 'Текущий трек после перемешивания не меняется'

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_play_shuffle_on_short_pause(self, alice):
        response = alice('включи Клаву Коку')
        assert_audio_play_directive(response.directive)
        track_title = response.directive.payload.metadata.title

        alice.skip(seconds=30)
        response = alice('поставь на паузу')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.ClearQueueDirective)
        alice.skip(seconds=50)

        response = alice('перемешай')
        assert any([sub_text in response.text for sub_text in [
            'Перемешала', 'беспорядок', 'Ок', 'вперемешку', 'полный винегрет'
        ]]), f'response.text={response.text}'
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.SetGlagolMetadataDirective

        response = alice('продолжи')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, offset_ms=30000)
        assert track_title == response.directive.payload.metadata.title

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_play_shuffle_on_long_pause(self, alice):
        response = alice('включи Клаву Коку')
        assert_audio_play_directive(response.directive)

        alice.skip(seconds=30)
        response = alice('поставь на паузу')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.ClearQueueDirective)
        alice.skip(seconds=70)

        response = alice('перемешай')
        assert response.text in [
            'Перемешаю для вас тишину, потому что ничего не играет.',
            'Сейчас ведь ничего не играет.',
            'Честно говоря, сейчас ничего не играет.',
            'Не знаю, как быть - ничего ведь не играет.',
        ]
        assert response.scenario == scenario.HollywoodMusic

    @pytest.mark.device_state(is_tv_plugged_in=True)
    def test_play_shuffle_not_music_screen(self, alice):
        response = alice('включи Клаву Коку')
        assert_audio_play_directive(response.directive)

        response = alice('домой')
        assert response.intent == intent.ProtocolGoHome

        response = alice('перемешай')
        assert response.text in [
            'Перемешаю для вас тишину, потому что ничего не играет.',
            'Сейчас ведь ничего не играет.',
            'Честно говоря, сейчас ничего не играет.',
            'Не знаю, как быть - ничего ведь не играет.',
        ]
        assert response.scenario == scenario.HollywoodMusic

    def test_play_track_on_repeat_next_track(self, alice):
        '''
        Команда "следущий трек" в режиме повтора одного трека выполняет выход из режима повтора и включает радио с
        похожими треками.
        '''
        response = alice('включи песню yesterday на повторе')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic,
                        directive=directives.names.AudioPlayDirective)

    def test_play_album_on_repeat_next_track(self, alice):
        response = alice('включи альбом beyond magnetic металлики на повтор')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Metallica')
        first_track_title = response.directive.payload.metadata.title

        for i in range(3):
            response = alice('следущий трек')
            assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
            assert_audio_play_directive(response.directive, subtitle='Metallica')
            assert response.directive.payload.metadata.title != first_track_title, f'iteration={i}'

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Metallica')
        assert response.directive.payload.metadata.title == first_track_title, \
               'В альбоме всего 4 трека, так что после этой команды мы должны вернуться к первому треку'

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи песню yesterday', r'.*(Beatles).*', 'The Beatles', id='track'),
        pytest.param('включи альбом beyond magnetic металлики', r'.*(Metallica).*', 'Metallica', id='album'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_repeat_next_track(self, alice, command, text_re, subtitle):
        '''
        В режиме повторения одного трека - трек повторяется если переключение треков происходит автоматически.
        '''
        music_response = alice(command)
        assert_response(music_response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(music_response.directive, subtitle=subtitle)
        first_track_title = music_response.directive.payload.metadata.title

        repeat_response = alice('повторяй этот трек')
        assert_response(repeat_response, scenario=scenario.HollywoodMusic)
        assert any([sub_text in repeat_response.text for sub_text in [
            'Хорошо, буду повторять.', 'Ок, ставлю на повтор.',
        ]]), f'response.text={repeat_response.text}'
        set_glagol_metadata = repeat_response.directives[0]
        assert set_glagol_metadata.name == directives.names.SetGlagolMetadataDirective
        assert set_glagol_metadata.payload.glagol_metadata.music_metadata.repeat_mode == 'One'

        for i in range(2):
            alice.skip(milliseconds=alice.device_state.AudioPlayer.DurationMs)
            assert_response(music_response, text_is_none=True, scenario=scenario.HollywoodMusic)
            assert_audio_play_directive(music_response.directive, subtitle=subtitle)
            assert music_response.directive.payload.metadata.title == first_track_title, f'iteration={i}'

    def test_play_track_next_track(self, alice):
        response = alice('включи песню yesterday')
        assert_response(response, sub_text='Yesterday', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic,
                        directive=directives.names.AudioPlayDirective)

    @pytest.mark.parametrize('command, text_re, subtitle_re', [
        pytest.param('включи queen', r'.*(Queen).*', r'Queen|Five', id='artist'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_replay_next_track(self, alice, command, text_re, subtitle_re):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle_re=subtitle_re)
        first_track_title = response.directive.payload.metadata.title

        alice.skip(seconds=60)

        response = alice('включи трек с начала')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle_re=subtitle_re)
        assert response.directive.payload.metadata.title == first_track_title

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle_re=subtitle_re)
        assert response.directive.payload.metadata.title != first_track_title

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи песню yesterday', r'.*(Yesterday).*', 'The Beatles', id='track'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_rewind_fwd_30s(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)

        alice.skip(seconds=60)

        response = alice('перемотай вперед на тридцать секунд')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_rewind_directive(response.directive, type_='Forward', amount_ms=30000)

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи песню yesterday', r'.*(Yesterday).*', 'The Beatles', id='track'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_rewind_bwd_half_minute(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)

        alice.skip(seconds=60)

        response = alice('перемотай назад на полминуты')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_rewind_directive(response.directive, type_='Backward', amount_ms=30000)

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи песню yesterday', r'.*(Yesterday).*', 'The Beatles', id='track'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_rewind_to_10s(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)

        alice.skip(seconds=60)

        response = alice('перемотай на десятую секунду')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_rewind_directive(response.directive, type_='Absolute', amount_ms=10000)

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи песню yesterday', r'.*(Yesterday).*', 'The Beatles', id='track'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_rewind_to_1h_5m_30s(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)

        alice.skip(seconds=60)

        rewind_response = alice('перемотай на один час пять минут и тридцать секунд')
        assert_response(rewind_response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_rewind_directive(rewind_response.directive, type_='Absolute', amount_ms=((1 * 60 + 5) * 60 + 30) * 1000)

        alice.skip(seconds=60)

        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert response.directive.name == directives.names.AudioPlayDirective

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи песню yesterday', r'.*(Yesterday).*', 'The Beatles', id='track'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_rewind_fwd_little(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)

        alice.skip(seconds=60)

        response = alice('перемотай немного вперед')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_rewind_directive(response.directive, type_='Forward', amount_ms=10000)

    @pytest.mark.parametrize('command, text_re, subtitle', [
        pytest.param('включи песню yesterday', r'.*(Yesterday).*', 'The Beatles', id='track'),
        pytest.param('включи веселую музыку', r'.*(весёлое настроение|веселого настроения).*', None, id='mood'),
    ])
    def test_play_music_rewind_bwd_little(self, alice, command, text_re, subtitle):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle=subtitle)

        alice.skip(seconds=60)

        response = alice('перемотай немного назад')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_rewind_directive(response.directive, type_='Backward', amount_ms=10000)

    def test_play_playlist_of_the_day(self, alice):
        response = alice('включи плейлист дня')
        assert_response(response, sub_text='Плейлист дня', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

    def test_play_personal_playlist(self, alice):
        response = alice('включи плейлист хрючень брудень и элекок')
        assert_response(response, sub_text='хрючень брудень и элекок', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')

    def test_play_public_playlist(self, alice):
        response = alice('включи плейлист песни из фильмов тарантино')
        assert_response(response, sub_text='Тарантино', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

    def test_fm_radio_continue_supports_player_features(self, alice):
        response = alice('включи queen')
        assert_response(response, text='Включаю: Queen.', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        response = alice('включи детское радио')
        assert_response(response, sub_text='Детское радио', scenario=scenario.Vins)

        response = alice('пауза')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.PlayerPauseDirective)

        response = alice('играй')
        assert_response(response, scenario=scenario.Vins)  # This must be 'Детское радио'

    @pytest.mark.parametrize('command', [
        'следующая',
        'предыдущая',
    ])
    def test_fm_radio_prev_next_support_player_features(self, alice, command):
        response = alice('включи queen')
        assert_response(response, text='Включаю: Queen.', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        response = alice('включи детское радио')
        assert_response(response, sub_text='Детское радио', scenario=scenario.Vins)

        response = alice(command)
        assert_response(response, scenario=scenario.Vins)  # Vins must switch the radio station to another one

    @pytest.mark.parametrize('command', [
        'поставь лайк',
        'поставь дизлайк',
        'перемешай',
        'повторяй этот трек',
        'включи трек с начала',
    ])
    def test_fm_radio_other_commands_support_player_features(self, alice, command):
        response = alice('включи queen')
        assert_response(response, text='Включаю: Queen.', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        response = alice('включи детское радио')
        assert_response(response, sub_text='Детское радио', scenario=scenario.Vins)

        response = alice(command)
        assert_response(response, scenario=scenario.Vins)  # Alice should say that fm-radio is unable do it

    def test_player_continue_music(self, alice):
        '''
        Должна включиться музыка, даже несмотря на то, что последний плеер, который играл - видео.
        '''
        response = alice('включи queen')
        assert response.intent == intent.MusicPlay
        assert_response(response, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen')
        track_title = response.directive.payload.metadata.title

        response = alice('включи рик и морти первая серия второй сезон')
        assert response.intent == intent.VideoPlay

        response = alice('домой')
        assert response.intent == intent.ProtocolGoHome

        response = alice('продолжи музыку')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen')
        assert track_title == response.directive.payload.metadata.title

    @pytest.mark.parametrize('last_command', [
        ('продолжи фильм'),
        ('продолжить смотреть'),
    ])
    def test_player_continue_video(self, alice, last_command):
        '''
        Должно включаться именно видео, даже несмотря на то, что последний плеер, который играл - музыка.
        '''
        response = alice('включи рик и морти первая серия второй сезон')
        assert response.intent == intent.VideoPlay

        response = alice('включи queen')
        assert response.intent == intent.MusicPlay

        response = alice('домой')
        assert response.intent == intent.ProtocolGoHome

        response = alice(last_command)
        assert response.intent == intent.PlayerContinue
        assert response.scenario == scenario.Vins

    def test_player_next_track_video(self, alice):
        '''
        Должно включаться именно видео, даже несмотря на то, что последний плеер, который играл - музыка.
        '''
        response = alice('включи рик и морти первая серия второй сезон')
        assert response.intent == intent.VideoPlay

        response = alice('включи queen')
        assert response.intent == intent.MusicPlay

        response = alice('домой')
        assert response.intent == intent.ProtocolGoHome

        response = alice('следующий фильм')
        assert response.intent == intent.PlayNextTrack
        assert response.scenario == scenario.Vins

    def test_player_prev_track_video(self, alice):
        '''
        Должно включаться именно видео, даже несмотря на то, что последний плеер, который играл - музыка.
        '''
        response = alice('включи рик и морти первая серия второй сезон')
        assert response.intent == intent.VideoPlay

        response = alice('включи queen')
        assert response.intent == intent.MusicPlay

        response = alice('следующий трек')  # Чтобы хотя бы потенциально мы могли включить предыдущий чуть позже

        response = alice('домой')
        assert response.intent == intent.ProtocolGoHome

        response = alice('предыдущий фильм')
        assert response.intent == intent.PlayPreviousTrack
        assert response.scenario == scenario.Vins


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestMusicThinClientOnDemandOnly(object):

    owners = ('vitvlkv',)

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-619')
    @pytest.mark.experiments('uniproxy_use_server_time_from_client')
    def test_play_track_next_track_prev_track_positive(self, alice):
        '''
        Если после проваливания в радио на старой музыке в течение 30 секунд сказать "предыдущий трек", то мы
        вернемся в музыку на тонком клиенте.
        '''
        response = alice('включи песню yesterday')
        assert_response(response, sub_text='Yesterday', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')
        first_track_title = response.directive.payload.metadata.title

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic,
                        directive=directives.names.MmStackEngineGetNextCallback)

        response.next()
        assert response.directive.name == directives.names.MusicPlayDirective, 'Включается радио на старой музыке'
        alice.skip(seconds=20)

        response = alice('предыдущий трек')
        assert_audio_play_directive(response.directive)
        assert response.directive.payload.metadata.title == first_track_title

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-619')
    @pytest.mark.experiments('uniproxy_use_server_time_from_client')
    def test_play_track_next_track_prev_track_negative(self, alice):
        '''
        Если после проваливания в радио на старой музыке по прошествии 30 секунд сказать "предыдущий трек", то мы
        НЕ вернемся в музыку на тонком клиенте.
        '''
        response = alice('включи песню yesterday')
        assert_response(response, sub_text='Yesterday', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='The Beatles')

        response = alice('следущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic,
                        directive=directives.names.MmStackEngineGetNextCallback)

        response.next()
        assert response.directive.name == directives.names.MusicPlayDirective, 'Включается радио на старой музыке'
        alice.skip(seconds=40)

        response = alice('предыдущий трек')
        assert response.directive.name == directives.names.PlayerPreviousTrackDirective, 'Старая музыка обрабатывает'\
                                                                                         'команду "предыдущий трек"'
