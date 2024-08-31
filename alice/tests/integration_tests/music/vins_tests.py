"""
A port of VINS tests for music, but using the whole Alice stack.
See alice/vins/apps/personal_assistant/personal_assistant/tests/integration_data/test_music.yaml

NOTE(a-square): some NLG have changed, I assume that production behavior
is the correct one and the VINS tests are broken if there is a difference.
"""

# TODO(a-square): test auto & auto_old
# TODO(a-square): test suggests
# TODO(a-square): test for unauthorized_personal_attention
# TODO(a-square): test fairy_tale_elari, fairy_tale_auto, other fairy tales?
# TODO(a-square): test misc quasar behaviors

import re

import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import pytest

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface


class _AnswerPatterns(object):
    inability = '|'.join([
        r'Я еще не научилась этому\. Давно собираюсь, но все времени нет',
        r'Я пока это не умею',
        r'Я еще не умею это',
        r'Я не могу пока, но скоро научусь',
        r'Меня пока не научили этому',
        r'Когда-нибудь я смогу это сделать, но не сейчас',
        r'Надеюсь, я скоро смогу это делать\. Но пока нет',
        r'Я не знаю, как это сделать\. Извините',
        r'Так делать я еще не умею',
        r'Программист Алексей обещал это вскоре запрограммировать\. Но он мне много чего обещал',
        r'К сожалению, этого я пока не умею\. Но я быстро учусь\.',
    ])
    unauthorized_personal_attention = (
        r'Я не могу включить ваш плейлист, потому что не знаю вас\. ' +
        r'Проверьте авторизацию в приложении.'
    )
    explicit_warning = '( (Осторожно|Внимание).*(Д|д)ет.*)?'
    unverified_playlist_warning = '(' + '|'.join([
        r'Вот что я нашла среди плейлистов других пользователей\. ',
        r'Нашла что-то подходящее среди плейлистов других пользователей\. ',
    ]) + ')?'
    children_mode_warning = r'[^\.]+детск[^\.]+. '
    music_string = r'[^,\.]+|"[^"]+"'
    only_play_playlist_answer = 'Включаю плейлист'
    open_playlist_answer = fr'{unverified_playlist_warning}Открываю( подборку)? {music_string}\.?{explicit_warning}'
    play_playlist_answer = fr'{unverified_playlist_warning}Включаю( подборку)? {music_string}\.?{explicit_warning}'
    album_answer = fr'(({music_string}), )+альбом {music_string}\.?{explicit_warning}'
    open_album_answer = f'Открываю:? {album_answer}'
    play_album_answer = f'Включаю:? {album_answer}'
    only_play_album_answer = 'Включаю альбом'
    track_answer = fr'(({music_string}|{album_answer}), )+песня {music_string}\.?{explicit_warning}'
    open_track_answer = f'Открываю:? {track_answer}'
    play_track_answer = f'Включаю:? {track_answer}'
    open_author_answer = fr'Открываю:? {music_string}\.?{explicit_warning}'
    play_author_answer = fr'Включаю:? {music_string}\.?{explicit_warning}'
    only_play_answer = fr'(Включаю\.?{explicit_warning})'
    only_open_answer = fr'(Открываю\.?{explicit_warning})'
    play_answer = f'{play_track_answer}|{play_album_answer}|{play_author_answer}|{play_playlist_answer}|{only_play_answer}'
    open_answer = f'{open_track_answer}|{open_album_answer}|{open_author_answer}|{open_playlist_answer}|{only_open_answer}'
    no_music = '|'.join([
        r'К сожалению, у меня нет такой музыки\.',
        r'Была ведь эта музыка у меня где-то... Не могу найти, простите\.',
        r'Как назло, именно этой музыки у меня нет\.',
        r'У меня нет такой музыки, попробуйте что-нибудь другое\.',
        r'Я не нашла музыки по вашему запросу, попробуйте ещё\.',
    ])
    open_serp = '.*([Ии]щу|[Нн]айд(у|ётся)|[Нн]айдём|Одну секунду|поиск|поищем).*'
    music_for_running = 'подборку "Плейлист для бега"|радио "Бег"'
    beatles_yesterday_answer = 'Включаю:.*Beatles.*Yesterday.*'
    sing_song_answer = r'[а-яА-Я! ,\.-]+'
    sing_song_answer_voice = r'[а-яА-Я!\+ ,\.-]+(!|\.) *\.sil<\[\d+\]> +<speaker audio="sing_song_\d\d\.opus">'
    uncertainty = '(' + '|'.join([
        r'Возможно, вам подойдет вот это\. ',
        r'Не совсем уверена, но ',
        r'Скорее всего это вам подойдет\. ',
        r'Надеюсь, это подойдёт\. ',
        r'Вот что нашлось\. '
    ]) + ')?'
    discovery_preambula = '(Вам может понравиться|Могу предложить|Послушайте) '
    discovery_answer = '|'.join([
        r'Включаю. Обязательно поставьте лайк, если я угадала с этим треком. Это сделает мои рекомендации еще круче.',
        r'Включаю. Чтобы я стала лучшим диджеем вашего дома, не забудьте поставить лайк, если я угадала с песней.',
        r'Включаю то, что мне нравится. Если и вам что-то понравится, не забывайте говорить мне: "Лайк!".',
        r'Включаю музыку на свой вкус. Если наши вкусы совпадут, поставьте лайк, и я буду лучше их понимать.',
        r'Включаю, но предупреждаю, в этот раз я немного пошалила, и может быть необычно. Смело скажите: "Лайк!", если вам что-то понравится.',
        r'Включаю. А чтобы я лучше понимала ваши интересы, ставьте лайки особенно классным трекам и говорите: "Дальше!", если не нравится.',
        r'Включаю. И я хочу проверить, хорошо ли я вас знаю. Если понравится, поставьте лайк.',
        r'Включаю. Если какая-то песня запала в душу, так и скажите: "Лайк"!',
        r'Включаю. Если наши вкусы совпадут, скажите: "Лайк!", чтобы я поняла, что все правильно делаю.',
        r'Включаю. Если что-то понравится, скажите: "Лайк!", это сделает мои рекомендации точнее и разнообразнее.',
        r'Включаю. Если с чем-то промахнусь, не судите строго, просто скажите: "Дальше".',
        r'Включаю. Если что-то не зайдет, метните в меня дизлайк.',
        r'Включаю. Я старалась, но могу и ошибаться. Если что, используйте дизлайк.',
        r'Включаю. Если понравится, скажите: "Лайк!" - мне будет приятно.',
        r'Включаю интересную музыку.',
    ])
    wanna_dance = r'\.( Потанцуем\?)?'
    yoga_answer = 'Включаю.*[Йй]ог.*'
    nature_sound = r'[Зз]вуки природы[\.]?'
    music_toster = '.*([Тт]остер|[Tt]oster).*'
    playlist_or_track = f'({play_playlist_answer}|{play_track_answer})'
    personal_playlist_answer = '|'.join([
        r'Послушаем ваше любимое.',
        r'Включаю ваши любимые песни.',
        r'Люблю песни, которые вы любите.',
        r'Окей. Плейлист с вашей любимой музыкой.',
        r'Окей. Песни, которые вам понравились.',
    ])
    personal_playlist_shuffle_answer = '|'.join([
        r'Послушаем ваше любимое вперемешку.',
        r'Включаю ваши любимые песни вперемешку.',
        r'Окей. Плейлист с вашей любимой музыкой вперемешку.'
    ])
    plus_required_text = '|'.join([
        r'Чтобы слушать .*, вам нужно оформить подписку Яндекс\.Плюс.',
        r'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
        r'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
    ])
    plus_required_voice = '|'.join([
        r'Чтобы слушать .*, вам нужно оформить подписку Яндекс Плюс.',
        r'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
        r'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
    ])
    fairytale_answer = 'Включаю сказки\\.'
    some_fairytale_answer = f'Включаю сказку {music_string}'
    fairytale_unknown_answer = '.* такой сказки .*нет.*'
    fairytale_kolobok_answer = 'Включаю.* сказк.*[Кк]олобок.*'
    fairytale_zahoder_answer = 'Включаю "Стихи и сказки"'
    children_mode_basta = f'{children_mode_warning}{play_author_answer}'
    happy_answer = '|'.join([
        r'Это как раз подойдёт под вес[её]лое настроение\.',
        r'Вот, отлично подойдёт под вес[её]лое настроение\.',
        r'Есть отличная музыка для вес[её]лого настроения\.',
        r'Знаю подходящую музыку для вес[её]лого настроения\.',
        r'Вот, самое то для вес[её]лого настроения\.',
    ])
    sad_answer = '|'.join([
        r'Это как раз подойдёт под грустное настроение\.',
        r'Вот, отлично подойдёт под грустное настроение\.',
        r'Есть отличная музыка для грустного настроения\.',
        r'Знаю подходящую музыку для грустного настроения\.',
        r'Вот, самое то для грустного настроения\.',
    ])
    music_tags_activity = '|'.join([
        r'Вот, отлично подойдет для',
        r'Вот, как раз для',
        r'Включаю музыку для',
        r'Хорошо, музыка для',
        r'Окей. Музыка для',
    ])
    party_music_answer = '|'.join([
        r'Включаю подборку "Для вечеринки"\.',
        f'{music_tags_activity} вечеринки\\.',
    ])
    beloved_music_answer = f'{music_tags_activity} влюблённых\\.'
    play_tags_answer = r'Есть кое-что для вас\.|Есть одна идея\.|Такое у меня есть\.|Есть музыка на этот случай.'
    rap = r'.*([Рр]эп|хип-хоп).*'
    rock = r'.*[Рр]ок.*'
    jazz = r'.*[Дд]жаз.*'
    driving = r'.*(за рул[ёе]м|вожден).*'
    acdc_or_train = rf'.*(({play_answer})|трениров).*'
    running = r'.*(бег|пробеж).*'


class _Answers(object):
    def __init__(self):
        for attr, pattern in _AnswerPatterns.__dict__.items():
            if '__' not in attr:
                setattr(self, attr, re.compile(pattern))

    def match(self, response, text=None, voice=None):
        if text:
            assert response.text
            assert getattr(self, text).match(response.text)

        if voice:
            assert response.output_speech_text
            assert getattr(self, voice).match(response.output_speech_text)


answers = _Answers()


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.navi,
    surface.searchapp,
    surface.yabro_win,
])
class TestCommonNonQuasar(object):
    owners = ('abc:alice_scenarios_music',)

    def test_sing_song(self, alice):
        def check_suggests(response):
            assert response.suggest('Еще песню!') is not None
            assert response.suggest('Слушать на Яндекс.Музыке!') is not None

        response = alice('спой песенку')
        answers.match(response, 'sing_song_answer')
        check_suggests(response)

        response = alice('еще')
        answers.match(response, 'sing_song_answer')
        check_suggests(response)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.launcher,
    surface.yabro_win,
])
class TestSearchAppLike(object):
    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.voice
    @pytest.mark.parametrize('command, answer', [
        ('включи песню Yesterday', 'only_play_answer'),
        ('включи Мадонну', 'only_play_answer'),
        ('поставь альбом The Dark Side of the Moon', 'only_play_answer'),
        ('включи Joe Dassin', 'only_play_answer'),
        ('включи музыку', 'play_answer'),
    ])
    def test_music_play(self, alice, command, answer):
        response = alice(command)
        if surface.is_searchapp(alice):
            assert response.output_speech_text.startswith(('Включаю', 'Открываю'))
            assert len(response.cards) == 1
            assert response.div_card
            assert response.cards[0].type == 'div2_card'
            assert not response.text_card
        else:
            answers.match(response, answer)

    @pytest.mark.voice
    @pytest.mark.parametrize('command, answer', [
        ('включи свои любимые песни', 'no_music'),
        ('включи музыку на твой вкус', 'no_music'),
        ('поставь что-нибудь популярное', 'only_play_playlist_answer'),
        ('включи трендовые песни', 'only_play_playlist_answer'),
        pytest.param('включи подборку чарт яндекс музыки', 'only_play_playlist_answer',
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8664')),
        ('поставь твои новогодние песни', 'only_play_album_answer'),
        ('поставь музыку повеселее', 'play_answer'),  # XXX(a-square): launcher has unusual behavior here
    ])
    def test_special_playlist(self, alice, command, answer):
        response = alice(command)
        if surface.is_searchapp(alice):
            assert response.output_speech_text.startswith(('Это экспериментальный плейлист', 'Включаю'))
            assert len(response.cards) == 1
            assert response.div_card
            assert response.cards[0].type == 'div2_card'
            assert not response.text_card
        else:
            answers.match(response, answer)

    @pytest.mark.parametrize('command', [
        'звуки природы',
        'включи звуки природы пение птиц пожалуйста',
        'поставь звуки природы для расслабления',
        'слушать шум моря',
        'слушать как кот мурчит',
        'включи как котик мурчит',
        'включи звуки города',
        'заснуть под звуки воды',
        'включи шум грозы',
    ])
    def test_ambient_sounds(self, alice, command):
        response = alice(command)
        if surface.is_searchapp(alice):
            assert response.text == 'Включаю'
        else:
            assert response.text == '...'  # TODO(a-square): wtf?


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.navi])
class TestNavi(object):
    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.parametrize('command, answer', [
        ('включи песню Yesterday', 'play_track_answer'),
        ('включи Мадонну', 'play_author_answer'),
        ('поставь альбом Троды плудов', 'play_album_answer'),
        ('включи Joe Dassin', 'play_author_answer'),
        ('скачать песню Yesterday бесплатно', 'beatles_yesterday_answer'),
        ('включи музыку', 'only_play_answer'),
        ('включи свои любимые песни', 'only_play_answer'),
        ('включи музыку на твой вкус', 'only_play_answer'),
    ])
    def test_music_play(self, alice, command, answer):
        response = alice(command)
        answers.match(response, answer)

    @pytest.mark.parametrize('command, response_text', [
        ('поставь что-нибудь популярное', 'Включаю подборку "Чарт".'),
        ('включи трендовые песни', 'Включаю подборку "Чарт".'),
        ('включи подборку чарт яндекс музыки', 'Включаю подборку "Чарт".'),
        ('поставь твои новогодние песни', 'Включаю: Алиса, альбом "YANY".'),
        ('поставь музыку повеселее', 'Включаю.'),
    ])
    def test_special_playlist(self, alice, command, response_text):
        response = alice(command)
        assert response.text == response_text

    @pytest.mark.parametrize('command, response_text', [
        ('звуки природы', 'Включаю звуки природы.'),
        ('включи звуки природы пение птиц пожалуйста', 'Включаю Пение птиц.'),
        ('поставь звуки природы для расслабления', 'Включаю звуки природы.'),
        ('слушать шум моря', 'Включаю Шум моря.'),
        ('слушать как кот мурчит', 'Включаю Звук мурчания.'),
        ('включи как котик мурчит', 'Включаю Звук мурчания.'),
        ('включи звуки города', 'Включаю Звуки города.'),
        ('заснуть под звуки воды', 'Включаю Звуки воды.'),
        ('включи шум грозы', 'Включаю Звуки грозы.'),
    ])
    def test_ambient_sounds(self, alice, command, response_text):
        response = alice(command)
        assert response.text == response_text


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])  # TODO(a-square): tv?
class TestQuasar(object):
    owners = ('abc:alice_scenarios_music', 'ardulat')

    @pytest.mark.voice
    @pytest.mark.parametrize('command, directive, response_texts', [
        ('звуки природы', directives.names.MusicPlayDirective, ['Включаю', 'звуки природы']),
        ('включи звуки природы пение птиц пожалуйста', directives.names.MusicPlayDirective, ['Включаю', 'Пение птиц']),
        pytest.param('поставь звуки природы для расслабления', directives.names.AudioPlayDirective,
                     ['Включаю', 'звуки природы'],
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-12854')),
        ('слушать шум моря', directives.names.MusicPlayDirective, ['Включаю', 'Шум моря']),
        ('слушать как кот мурчит', directives.names.MusicPlayDirective, ['Включаю', 'Звук мурчания']),
        # Срабатывает тонкий плеер, так как vins отрезается преклассификатором
        ('включи как котик мурчит', directives.names.MusicPlayDirective, ['Включаю', 'мурчани']),
        ('включи звуки города', directives.names.MusicPlayDirective, ['Включаю', 'Звуки города']),
        ('заснуть под звуки воды', directives.names.MusicPlayDirective, ['Включаю', 'Звуки воды']),
        ('включи шум грозы', directives.names.MusicPlayDirective, ['Включаю', 'Звуки грозы']),
    ])
    def test_ambient_sounds(self, alice, command, directive, response_texts):
        response = alice(command)
        assert response.intent in (intent.MusicAmbientSound, intent.MusicPlay)
        assert response.directive.name == directive

        for response_text in response_texts:
            assert response_text.lower() in response.text.lower()

    def test_ambient_sounds_unusual(self, alice):
        # TODO(a-square): check out that nature sounds are what's playing
        response = alice('включи звуки природы звук тостера')
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

    @pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.children})
    def test_children_mode(self, alice):
        response = alice('включи группу Баста')
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        # TODO(a-square): enable voice session and check the preamble
        # assert answers.match(response, text='play_author_answer', voice='children_mode_basta')
        answers.match(response, 'play_author_answer')

    @pytest.mark.parametrize('command, answer_pattern, response_texts', [
        ('включи свои любимые песни', 'discovery_answer', None),
        ('включи музыку на твой вкус', 'discovery_answer', None),
        ('поставь что-нибудь популярное', None, ['Включаю подборку "Чарт']),
        ('включи трендовые песни', None, ['Включаю подборку "Чарт']),
        ('включи подборку чарт яндекс музыки', None, ['Включаю подборку "Чарт']),
        ('поставь твои новогодние песни', None, ['Включаю: Алиса, альбом "YANY"']),
        ('поставь музыку повеселее', 'happy_answer', None),
        ('включи плейлист для вечеринки', None, ['для вечеринки', 'для рок-вечеринки', 'для знойной вечеринки']),
        ('включи музыку для вечеринки', 'party_music_answer', None),
        ('поставь что-нибудь посвежее', None, ['Включаю подборку "Громкие новинки месяца"']),
        ('включи новую музыку', None, ['Включаю подборку "Громкие новинки месяца"']),
        ('включи плейлист дня', None, ['Включаю подборку "Плейлист дня"']),
        ('включи подборку премьера', None, ['Включаю подборку "Премьера"']),
        ('включи список песен дежавю', None, ['Включаю подборку "Дежавю"']),
        ('включи свои любимые песни', 'discovery_answer', None),
        ('включи музыку для йоги', 'yoga_answer', None),
        ('включи подборку премьера', None, ['Включаю подборку "Премьера"']),
        ('включи сборник тайник', None, ['Включаю подборку "Тайник"']),
        ('включи список песен дежавю', None, ['Включаю подборку "Дежавю"']),
    ])
    def test_special_playlist(self, alice, command, answer_pattern, response_texts):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        if answer_pattern:
            answers.match(response, answer_pattern)
        if response_texts:
            assert any([text.lower() in response.text.lower() for text in response_texts]), \
                f'Not any of {response_texts} is in response.text is {response.text}'

    @pytest.mark.experiments('music_force_show_first_track')
    @pytest.mark.parametrize('command, answer, first_track_piece', [
        ('слушать элджей песню розовое вино', 'play_track_answer', 'розовое вино'),
        ('lady gaga альбом artpop', 'play_album_answer', 'lady gaga, aura'),
        ('come together альбом abbey road', 'play_track_answer', 'come together'),
        ('хочу послушать верка сердючка', 'play_author_answer', 'верка сердючка'),
        ('опа гангнамстайл слушать', 'playlist_or_track', 'gangnam style'),
        ('альбом the dark side of the moon', 'play_album_answer', 'pink floyd, speak to me'),
        ('включи монеточку каждый раз', 'play_answer', 'каждый раз'),
        ('запусти crazy группы aerosmith', 'play_answer', 'aerosmith, crazy'),
        ('Включи понедельник группы 2й сорт', 'play_answer', '2-й сорт, понедельник'),
        ('Включи 2й сорт', 'play_answer', '2-й сорт'),
        ('Поставь 2й сорт алкоголь', 'play_answer', '2-й сорт'),
    ])
    def test_on_demand(self, alice, command, answer, first_track_piece):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        answers.match(response, answer)

        first_track = response.scenario_analytics_info.objects.get('music.first_track_id')
        assert first_track
        if first_track_piece is not None:
            assert first_track_piece in first_track.human_readable.lower()

    @pytest.mark.parametrize('command, answer', [
        ('включи музыку', 'only_play_answer'),
        ('включи военные песни', 'play_playlist_answer'),
        ('играй узбекские песни', 'play_playlist_answer'),
        pytest.param(
            'включи грустный рэп', 'play_playlist_answer',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-1023')
        ),
        # ('хочу послушать подборку индастриал', 'play_playlist_answer'), https://st.yandex-team.ru/DIALOG-7422#60f5c6d660474549b5e3aa44
        ('вруби рок', 'rock'),
        ('играй грустную музыку', 'sad_answer'),
        ('треки для бега', 'running'),
        ('запусти музыку восьмидесятых', 'play_answer'),
        ('включи отечественные песни', 'play_answer'),
        ('включи песни по французски', 'play_answer'),
        ('включи песни с португальским вокалом', 'play_answer'),
        ('включи песни с мужским вокалом', 'play_answer'),
        ('включи песни где поют женщины', 'play_answer'),
        ('включи песни без слов', 'play_answer'),
        ('свежий музон врубай', 'play_playlist_answer'),
        ('слушать грустный панк для бега', 'play_answer'),
        ('включи музыку шестидесятых на английском', 'play_answer'),  # It is epoch+language...
                                                                      # Why not play_tags_answer?
        ('включи веселый рок для тренировки', 'play_playlist_answer'),
        ('наша музыка времен перестройки скачать', 'play_answer'),
        ('новые песни для вождения автомобиля', 'driving'),
        pytest.param('вруби мое любимое веселое', 'personal_playlist_answer',
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-13897')),
        pytest.param('включи красивую музыку для свиданий', 'play_playlist_answer',
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-12854')),
        ('ставь джаз 50х', 'play_tags_answer'),
        ('иностранную музыку скачать', 'play_answer'),
        pytest.param('давай послушаем шансон который мне нравится', 'personal_playlist_answer',
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-13897')),
        pytest.param('хочу послушать грустные песни queen', 'play_answer',
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-11701')),
        pytest.param('включи треки ac dc для тренировки', 'acdc_or_train',
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-11701')),
        ('последний альбом imagine dragons', 'play_answer'),
        pytest.param('слушать мои любимые песни аквариум', 'play_playlist_answer',
                     marks=pytest.mark.xfail(reason='Scenario is unable to work with personal+search_text slots combination for now')),
    ])
    def test_discovery(self, alice, command, answer):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        answers.match(response, answer)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.dexp])
class TestDexp(object):
    owners = ('abc:alice_scenarios_music', 'ardulat')

    @pytest.mark.parametrize('command, response_texts', [
        ('звуки природы', ['Включаю', 'звуки природы']),
        ('включи звуки природы пение птиц пожалуйста', ['Включаю', 'Пение птиц']),
        pytest.param('поставь звуки природы для расслабления', ['Включаю', 'звуки природы'],
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-12854')),
        ('слушать шум моря', ['Включаю', 'Шум моря']),
        ('слушать как кот мурчит', ['Включаю', 'Звук мурчания']),
        ('включи как котик мурчит', ['Включаю', 'мурчани']),
        ('включи звуки города', ['Включаю', 'Звуки города']),
        ('заснуть под звуки воды', ['Включаю', 'Звуки воды']),
        ('включи шум грозы', ['Включаю', 'Звуки грозы']),
    ])
    def test_ambient_sounds(self, alice, command, response_texts):
        response = alice(command)
        assert response.intent in (intent.MusicAmbientSound, intent.MusicPlay)
        assert response.directive.name == directives.names.MusicPlayDirective

        for response_text in response_texts:
            assert response_text.lower() in response.text.lower()

    def test_ambient_sounds_unusual(self, alice):
        # TODO(a-square): check out that nature sounds are what's playing
        response = alice('включи звуки природы звук тостера')
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective

    @pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.children})
    def test_children_mode(self, alice):
        response = alice('включи группу Баста')
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        # TODO(a-square): enable voice session and check the preamble
        # assert answers.match(response, text='play_author_answer', voice='children_mode_basta')
        answers.match(response, 'play_author_answer')

    @pytest.mark.experiments('music_force_show_first_track')
    @pytest.mark.parametrize('command, answer, track_id', [
        ('расскажи сказку', 'fairytale_answer', None),
        ('включи сказку про колобка', 'fairytale_kolobok_answer', None),
        ('включи сказку про 2й сорт алкоголь', 'fairytale_unknown_answer', None),
        ('включи сказку заходера', 'fairytale_zahoder_answer', None),
        ('включи сказку зимовье зверей от Юлии Барановской', 'some_fairytale_answer', 46770453),
        ('поставь сказку история года гарика бурито', 'some_fairytale_answer', 46770454),
    ])
    def test_fairy_tales(self, alice, command, answer, track_id):
        response = alice(command)
        answers.match(response, answer)
        assert response.directive.name == directives.names.MusicPlayDirective
        if track_id:
            assert response.directive.payload.first_track_id == str(track_id)

    @pytest.mark.parametrize('command, answer_pattern, response_texts', [
        ('включи свои любимые песни', 'discovery_answer', None),
        ('включи музыку на твой вкус', 'discovery_answer', None),
        ('поставь что-нибудь популярное', None, ['Включаю подборку "Чарт']),
        ('включи трендовые песни', None, ['Включаю подборку "Чарт']),
        ('включи подборку чарт яндекс музыки', None, ['Включаю подборку "Чарт']),
        ('поставь твои новогодние песни', None, ['Включаю: Алиса, альбом "YANY"']),
        ('поставь музыку повеселее', 'happy_answer', None),
        ('включи плейлист для вечеринки', None, ['для вечеринки', 'для рок-вечеринки', 'для знойной вечеринки']),
        ('включи музыку для вечеринки', 'party_music_answer', None),
        ('поставь что-нибудь посвежее', None, ['Включаю подборку "Громкие новинки месяца"']),
        ('включи новую музыку', None, ['Включаю подборку "Громкие новинки месяца"']),
        ('включи плейлист дня', None, ['Включаю подборку "Плейлист дня"']),
        ('включи подборку премьера', None, ['Включаю подборку "Премьера"']),
        ('включи список песен дежавю', None, ['Включаю подборку "Дежавю"']),
        ('включи свои любимые песни', 'discovery_answer', None),
        ('включи музыку для йоги', 'yoga_answer', None),
        ('включи подборку премьера', None, ['Включаю подборку "Премьера"']),
        ('включи сборник тайник', None, ['Включаю подборку "Тайник"']),
        ('включи список песен дежавю', None, ['Включаю подборку "Дежавю"']),
    ])
    def test_special_playlist(self, alice, command, answer_pattern, response_texts):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective

        if answer_pattern:
            answers.match(response, answer_pattern)
        if response_texts:
            assert any([text.lower() in response.text.lower() for text in response_texts]), \
                f'Not any of {response_texts} is in response.text is {response.text}'

    def test_show_first_track(self, alice):
        # TODO(a-square): learn to check voice replies
        alice('включи джаз')
        alice('включи музыку для вечеринки')

    @pytest.mark.experiments('music_force_show_first_track')
    @pytest.mark.parametrize('command, answer, first_track_piece', [
        ('слушать элджей песню розовое вино', 'play_track_answer', 'розовое вино'),
        ('lady gaga альбом artpop', 'play_album_answer', 'lady gaga, aura'),
        ('come together альбом abbey road', 'play_track_answer', 'come together'),
        ('хочу послушать верка сердючка', 'play_author_answer', 'верка сердючка'),
        ('опа гангнамстайл слушать', 'playlist_or_track', 'gangnam style'),
        ('альбом the dark side of the moon', 'play_album_answer', 'pink floyd, speak to me'),
        ('включи монеточку каждый раз', 'play_answer', 'каждый раз'),
        ('запусти crazy группы aerosmith', 'play_answer', 'aerosmith, crazy'),
        ('Включи понедельник группы 2й сорт', 'play_answer', '2-й сорт, понедельник'),
        ('Включи 2й сорт', 'play_answer', '2-й сорт'),
        ('Поставь 2й сорт алкоголь', 'play_answer', '2-й сорт'),
    ])
    def test_on_demand(self, alice, command, answer, first_track_piece):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective

        answers.match(response, answer)

        first_track = response.scenario_analytics_info.objects.get('music.first_track_id')
        assert first_track
        if first_track_piece is not None:
            assert first_track_piece in first_track.human_readable.lower()

    @pytest.mark.voice
    # TODO(a-square): learn how to check frame slots
    # TODO(a-square): learn how to check voice answer
    @pytest.mark.parametrize('command, answer', [
        ('включи музыку', 'only_play_answer'),
        ('включи военные песни', 'play_playlist_answer'),
        ('играй узбекские песни', 'play_playlist_answer'),
        # ('включи грустный рэп', 'play_playlist_answer'), #  https://st.yandex-team.ru/DIALOG-8829
        # ('хочу послушать подборку индастриал', 'play_playlist_answer'), https://st.yandex-team.ru/DIALOG-7422#60f5c6d660474549b5e3aa44
        ('вруби рок', 'rock'),
        ('играй грустную музыку', 'sad_answer'),
        ('треки для бега', 'running'),
        ('запусти музыку восьмидесятых', 'play_answer'),
        ('включи отечественные песни', 'play_answer'),
        ('включи песни по французски', 'play_answer'),
        ('включи песни с португальским вокалом', 'play_answer'),
        ('включи песни с мужским вокалом', 'play_answer'),
        ('включи песни где поют женщины', 'play_answer'),
        ('включи песни без слов', 'play_answer'),
        ('свежий музон врубай', 'play_playlist_answer'),
        ('слушать грустный панк для бега', 'play_answer'),
        ('включи музыку шестидесятых на английском', 'play_answer'),  # It is epoch+language...
                                                                      # Why not play_tags_answer?
        ('включи веселый рок для тренировки', 'play_playlist_answer'),
        ('наша музыка времен перестройки скачать', 'play_answer'),
        ('новые песни для вождения автомобиля', 'driving'),
        ('вруби мое любимое веселое', 'personal_playlist_answer'),
        ('включи музыку для свиданий', 'beloved_music_answer'),
        ('ставь джаз 50х', 'jazz'),  # It is genre+epoch... Why not play_tags_answer?
        ('иностранную музыку скачать', 'play_answer'),
        ('давай послушаем шансон который мне нравится', 'personal_playlist_answer'),
        ('хочу послушать грустные песни queen', 'play_answer'),
        ('включи треки ac dc для тренировки', 'acdc_or_train'),
        ('последний альбом imagine dragons', 'play_answer'),
        pytest.param('слушать мои любимые песни аквариум', 'play_playlist_answer',
                     marks=pytest.mark.xfail(reason='Scenario is unable to work with personal+search_text slots combination for now')),
    ])
    def test_discovery(self, alice, command, answer):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        answers.match(response, answer)

    # TODO(a-square): learn how to test
    def test_order(self, alice):
        alice('играй вперемешку мои песни')

    # TODO(a-square): learn how to test
    def test_repeat(self, alice):
        alice('включи на репите песню show must go on')

    # TODO(a-square): learn how to test
    def test_anaphora(self, alice):
        alice('включи этого исполнителя')
        alice('открой этот альбом')
        alice('поищи похожие пластинки')
        alice('сыграй что-нибудь аналогичное')
        alice('запусти на повторе этот трек')
        alice('запусти шафлом этот альбом')

    # TODO(a-square): learn how to test
    def test_search_anaphora(self, alice):
        alice('что такое апокалиптика')
        alice('включи их музыку')

        alice('кто такие битлз')
        alice('найди группы типа них')
