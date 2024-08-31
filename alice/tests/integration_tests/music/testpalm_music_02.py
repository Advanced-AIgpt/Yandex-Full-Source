import json
import re

import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import pytest

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
    surface.station(is_tv_plugged_in=False),
])
class TestPalmHelpWhileMusicPlays(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1319
    https://testpalm.yandex-team.ru/testcase/alice-388
    """

    owners = ('abc:alice_scenarios_music',)

    _answer_prefixes = (
        'Вы можете',
        'Я могу',
        'Я знаю',
        'Я умею',
    )
    _response_text_re = re.compile(
        r'.*\. (Рассказать ещё|Продолжить|Хотите узнать больше, скажите «да»|Хотите узнать ещё|Рассказывать ещё|Продолжим).'
    )

    @pytest.mark.xfail(reason="https://st.yandex-team.ru/HOLLYWOOD-1026")
    def test_alice_1319_388(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice('помощь')
        assert response.intent == intent.Onboarding
        self._assert_response_text(response.text)

        response = alice('да')
        assert response.intent == intent.OnboardingNext
        self._assert_response_text(response.text)

        response = alice('да')
        assert response.intent == intent.OnboardingNext
        self._assert_response_text(response.text)

        response = alice('да')
        assert response.intent == intent.OnboardingNext
        self._assert_response_text(response.text)

        response = alice('да')
        assert response.intent == intent.OnboardingNext
        self._assert_response_text(response.text)

    def _assert_response_text(self, response_text):
        assert response_text.startswith(self._answer_prefixes), f'Response text prefix is unexpected: {response_text}'
        assert self._response_text_re.match(response_text)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestPalmFairyTalesSmoke(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-576
    """

    owners = ('vitvlkv',)

    def test_alice_576(self, alice):
        response = alice('включи сказку Колобок')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent in [intent.MusicPlay, intent.MusicFairyTale]
        assert response.directive.name == directives.names.AudioPlayDirective
        assert 'колобок' in response.text.lower()
        # TODO(vitvlkv): assert AnalyticsInfo.Objects.FirstTrack.Genre in ['poemsforchildren', 'fairytales'] when nature sounds will be moved to Hollywood
        # There is no scenario_analytics_info, because Vins wins

        response = alice('расскажи сказку про Красную Шапочку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent in [intent.MusicPlay, intent.MusicFairyTale]
        assert response.directive.name == directives.names.AudioPlayDirective
        assert 'красная шапочка' in response.text.lower()
        # TODO(vitvlkv): assert 'красная шапочка' in AnalyticsInfo.Objects.HumanReadable when this intent will be moved to Hollywood
        # TODO(vitvlkv): assert AnalyticsInfo.Objects.FirstTrack.Genre in ['poemsforchildren', 'fairytales'] when nature sounds will be moved to Hollywood

        response = alice('поставь какую-нибудь сказку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent in [intent.MusicPlay, intent.MusicFairyTale]
        assert response.directive.name == directives.names.AudioPlayDirective
        assert 'включаю' in response.text.lower()
        # TODO: Как проверить что включилась именно сказка?..
        # TODO(vitvlkv): assert AnalyticsInfo.Objects.FirstTrack.Genre in ['poemsforchildren', 'fairytales'] when nature sounds will be moved to Hollywood

        for _ in range(2):
            response = alice('следующая сказка')
            assert response.scenario == scenario.HollywoodMusic
            assert response.intent == intent.PlayNextTrack
            assert response.directive.name == directives.names.AudioPlayDirective
            assert not response.text
            # TODO(vitvlkv): How to assert on this: Среди сказок не должны встречаться отдельные главы (например, главы из сказки “Маленький принц”)?


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestPalmNatureSoundsSmoke(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1279
    """

    owners = ('vitvlkv',)

    @pytest.mark.parametrize('command, response_texts, expected_directive', [
        ('Включи звук водопада', ['Включаю', 'Звук водопада'], directives.names.MusicPlayDirective),
        ('Поставь шум моря', ['Включаю', 'Шум моря'], directives.names.MusicPlayDirective),
        ('Слушать звук костра', ['Включаю', 'Звук костра'], directives.names.MusicPlayDirective),
        ('Поставь пожалуйста шум леса', ['Включаю', 'Шум леса'], directives.names.MusicPlayDirective),
        ('Уснуть под шум ветра', ['Включаю', 'Шум ветра'], directives.names.MusicPlayDirective),
        ('Запусти пение птиц', ['Включаю', 'Пение птиц'], directives.names.MusicPlayDirective),
        ('Включи звук лая собаки', ['Включаю', 'Лай собаки'], directives.names.MusicPlayDirective),
        ('Включи звук поезда', ['Включаю', 'Звук поезда'], directives.names.MusicPlayDirective),
        ('Слушать звук колокольчика', ['Включаю', 'Звук колокольчика'], directives.names.MusicPlayDirective),
        ('Вруби звук самолёта', ['Включаю', 'Звук самолета'], directives.names.MusicPlayDirective),
        ('Поставь мне звуки города', ['Включаю', 'Звуки города'], directives.names.MusicPlayDirective),
        ('Играй звук воды', ['Включаю', 'Звуки воды'], directives.names.MusicPlayDirective),
        ('Найди шум дождя', ['Включаю', 'Шум дождя'], directives.names.MusicPlayDirective),
        ('Запусти белый шум', ['Включаю', 'Белый шум'], directives.names.MusicPlayDirective),
        ('Слушать звук мурчания кота', ['Включаю', 'Звук мурчания'], directives.names.MusicPlayDirective),
        ('Поставь звуки природы', ['Включаю', 'звуки природы'], directives.names.MusicPlayDirective),
        ('Звук камина', ['Включаю', 'Звук камина'], directives.names.MusicPlayDirective),
        ('Играй звук камина и грозы', ['Включаю', 'Звук камина и грозы'], directives.names.MusicPlayDirective),
    ])
    def test_alice_1279(self, alice, command, response_texts, expected_directive):
        response = alice(command)
        assert response.scenario in [scenario.Vins, scenario.HollywoodMusic]
        assert response.directive.name == expected_directive
        for response_text in response_texts:
            assert response_text.lower() in response.text.lower()
        # TODO(vitvlkv): assert AnalyticsInfo.Objects.FirstTrack.Genre in ['naturesounds', 'relax', ''] when nature sounds will move to Hollywood


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'mm_formula=ambient_sounds_music',
    'vins_add_irrelevant_intents=personal_assistant.scenarios.music_ambient_sound',
    'bg_enable_ambient_sounds_in_music',
    'hw_music_enable_ambient_sound',
)
@pytest.mark.parametrize('surface', [surface.station])
class TestAmbientSoundsMusic(object):

    owners = ('ardulat', )

    @pytest.mark.parametrize('command, response_text', [
        ('Поставь шум моря', 'Шум моря'),
        ('Слушать звук костра', 'Звук костра'),
        ('Поставь пожалуйста шум леса', 'Шум леса'),
        ('Включи звук поезда', 'Звук поезда'),
        ('Играй звук воды', 'Звуки воды'),
        ('Найди шум дождя', 'Шум дождя'),
        ('Поставь звуки природы', 'Звуки природы'),
    ])
    def test_ambient_sounds(self, alice, command, response_text):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response_text.lower() in response.text.lower()


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.station(device_config={'content_settings': content_settings_pb2.children}),
])
class TestPalmFamilyModeContentSearch(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-591
    """

    owners = ('vitvlkv', 'akormushkin',)

    @pytest.mark.parametrize('command, response_text_re', [
        ('Найди ролики про котиков', r'.*про котиков.*'),
        ('Поставь мультик', r'.*мультфильмы.*'),
    ])
    def test_alice_591_video(self, alice, command, response_text_re):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert re.match(response_text_re, response.text.lower())
        assert 'videoSearch' in response.directive.payload.url or 'filmsSearch' in response.directive.payload.url
        # Unfortunately we cannot check that there are no explicit content

    @pytest.mark.parametrize('command, response_text_re', [
        ('Хочу сказку', r'.*[сказок|сказки].*'),
        ('Поставь песенку', r'.*включаю.*'),
    ])
    def test_alice_591_music(self, alice, command, response_text_re):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective
        assert re.match(response_text_re, response.text.lower())


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('podcasts')
@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.loudspeaker,
    surface.navi,
    surface.searchapp,
    surface.station,
    surface.yabro_win,
])
class TestPalmPodcasts(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1486
    """

    owners = ('vitvlkv', 'ardulat')

    @pytest.mark.parametrize('command', [
        'Включи подкаст',
        'Запусти подкаст',
        'Слушать подкаст',
        'Поставь подкаст',
        pytest.param('Заснуть под подкаст', marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8703')),
        'Прослушать подкаст',
        'Услышать подкаст',
        'Поставь пожалуйста подкаст',
        'привет включи подкаст',
        'включи мне пожалуйста подкаст',
        pytest.param('Играй подкаст', marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8703')),
        pytest.param('Играть подкаст', marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8703')),
        'Вруби подкаст',
        'Найти подкаст',
    ])
    def test_default_podcast_alice_1486(self, alice, command):
        response = alice(command)
        assert response.scenario in [scenario.Vins, scenario.HollywoodMusic]
        assert response.directive.name in [
            directives.names.AudioPlayDirective,
            directives.names.OpenUriDirective,
        ]
        if surface.is_smart_speaker(alice):
            assert 'топ-100' in response.text.lower()
            assert 'подкаст' in response.text.lower()

        playlist_id = '414787002:1104'
        if response.scenario == scenario.Vins:
            assert 'answer' in response.slots
            assert json.loads(response.slots['answer'].string)['id'] == playlist_id
        else:
            music_event = response.scenario_analytics_info.event('music_event')
            assert music_event is not None
            assert music_event.id == playlist_id

    @pytest.mark.parametrize('command, response_text', [
        ('Включи подкаст НОРМ', 'НОРМ'),
        ('Включи подкаст Жизнь человека', 'Жизнь человека'),
        pytest.param('Слушать подкаст Текст недели', 'Текст недели', marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8703')),
        ('Поставь подкаст Что случилось', 'Что случилось'),
        pytest.param('Заснуть под подкаст Продуктивный Роман', 'Продуктивный Роман', marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8703')),
        ('Прослушать подкаст Это Непросто', 'Это Непросто'),
        ('Услышать подкаст Крути педали', 'Крути педали'),
        ('Поставь пожалуйста подкаст Читатель', 'Читатель'),
        ('привет включи подкаст Мам, я и так знаю', 'Мам, я и так знаю'),
        pytest.param('включи подкаст Книжный обзор', None, marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8703')),
        ('Играй подкаст Дело случая', 'Дело случая'),
        ('Играть подкаст Как жить', 'Как жить'),
        pytest.param('Вруби подкаст Два по цене одного', 'Два по цене одного', marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8703')),
        ('Найди подкаст Дикие утки', 'Дикие утки'),
    ])
    def test_alice_1487(self, alice, command, response_text):
        response = alice(command)
        assert response.scenario in [scenario.Vins, scenario.HollywoodMusic]
        assert response.directive.name in [
            directives.names.AudioPlayDirective,
            directives.names.OpenUriDirective,
        ]
        if surface.is_smart_speaker(alice):
            assert 'подкаст' in response.text.lower() or 'выпуск' in response.text.lower()
            if response_text is not None:
                assert response_text.lower() in response.text.lower()


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.automotive])
class TestPodcastsAutomotive(object):

    owners = ('ardulat', )

    @pytest.mark.parametrize('command', [
        'Включи подкаст',
        'Включи подкаст Текст недели',
        'Включи подкаст Мам, я и так знаю',
    ])
    def test_podcasts(self, alice, command):
        response = alice(command)
        if response.scenario == scenario.Vins:
            assert response.directive.name == directives.names.OpenUriDirective
        else:
            assert response.scenario == scenario.HollywoodMusic
            assert not response.directive
            assert response.text == 'Извините, я пока не умею искать музыку.'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.watch])
class TestPodcastsWatch(object):

    owners = ('ardulat', )

    @pytest.mark.parametrize('command', [
        'Включи подкаст',
        'Включи подкаст Текст недели',
        'Включи подкаст Мам, я и так знаю',
        'Включи подкаст Медузы',
    ])
    def test_podcasts(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert not response.directive
        assert response.text in [
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestSmartTvSoundsAndPodcastUnsupported(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2864
    https://testpalm.yandex-team.ru/testcase/alice-2866
    """

    owners = ('ardulat', )

    @pytest.mark.parametrize('command, response_text', [
        ('Включи звуки природы', 'Включаю звуки природы.'),
        ('Включи звук моря', 'Включаю Шум моря.'),
    ])
    def test_alice_2864(self, alice, command, response_text):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.MusicAmbientSound
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == response_text

    def test_alice_2866(self, alice):
        response = alice('Включи подкасты')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent in (intent.MusicPodcast, intent.MusicPlay)
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю подборку "Подкасты: топ-100".'
