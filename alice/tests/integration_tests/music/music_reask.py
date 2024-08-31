import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


FIRST_REASK_TRACK_RESPONSES = [
    'Какую песню?',
    'Не поняла. Какую песню?',
    'Что включить?',
]

FIRST_REASK_ALBUM_RESPONSES = [
    'Какой альбом?',
    'Не поняла. Какой альбом?',
    'Что включить?',
]

FIRST_REASK_ARTIST_RESPONSES = [
    'Какого исполнителя?',
    'Не поняла. Какого исполнителя?',
    'Кого включить?',
]

SECOND_REASK_RESPONSES = [
    'Помедленее, я не поняла!',
    'Повторите почётче, пожалуйста!',
]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
])
class TestMusicReask(object):

    owners = ('ardulat', )

    @staticmethod
    def _assert_reask_response(response, texts):
        assert response.scenario == scenario.HollywoodMusic
        assert not response.directive
        assert response.text in texts

    @staticmethod
    def _assert_music_response(response, directive=directives.names.AudioPlayDirective):
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directive

    def test_reask_positive(self, alice):
        response = alice('включи песню')
        self._assert_reask_response(response, FIRST_REASK_TRACK_RESPONSES)

        response = alice('кукушка')
        self._assert_music_response(response)

    def test_reask_negative(self, alice):
        response = alice('включи песню')
        self._assert_reask_response(response, FIRST_REASK_TRACK_RESPONSES)

        response = alice('включи песню')
        self._assert_reask_response(response, SECOND_REASK_RESPONSES)

        response = alice('включи песню')
        self._assert_music_response(response, directive=directives.names.MusicPlayDirective)  # TODO(vitvlkv): This should be audio_play directive here too

    def test_reask_nothing(self, alice):
        response = alice('включи песню')
        self._assert_reask_response(response, FIRST_REASK_TRACK_RESPONSES)

        response = alice('никакую')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert not response.directive
        assert response.text

    def test_reask_stop(self, alice):
        response = alice('включи песню')
        self._assert_reask_response(response, FIRST_REASK_TRACK_RESPONSES)

        response = alice('стоп')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.has_voice_response()
        assert not response.text

    @pytest.mark.parametrize('command', [
        'черный кот',
        'smells like teen spirit',
        'californication',
    ])
    def test_reask_track(self, alice, command):
        response = alice('включи песню')
        self._assert_reask_response(response, FIRST_REASK_TRACK_RESPONSES)

        response = alice('включи песню')
        self._assert_reask_response(response, SECOND_REASK_RESPONSES)

        response = alice(command)
        self._assert_music_response(response)

    @pytest.mark.parametrize('command', [
        'горгород',
        'rumours',
        'the dark side of the moon',
    ])
    def test_reask_album(self, alice, command):
        response = alice('включи альбом')
        self._assert_reask_response(response, FIRST_REASK_ALBUM_RESPONSES)

        response = alice('включи альбом')
        self._assert_reask_response(response, SECOND_REASK_RESPONSES)

        response = alice(command)
        self._assert_music_response(response)

    @pytest.mark.parametrize('command', [
        'linkin park',
        'ac dc',
        'адель',
        'майкл джексон',
    ])
    def test_reask_artist(self, alice, command):
        response = alice('включи группу')
        self._assert_reask_response(response, FIRST_REASK_ARTIST_RESPONSES)

        response = alice('включи группу')
        self._assert_reask_response(response, SECOND_REASK_RESPONSES)

        response = alice(command)
        self._assert_music_response(response)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.smart_tv,
    surface.automotive,
    surface.navi,
    surface.searchapp,
    surface.launcher,
    surface.yabro_win,
])
class TestMusicReaskUnsupported1(object):

    owners = ('ardulat', )

    @staticmethod
    def _assert_music_response(response, alice):
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        if not surface.is_searchapp(alice):
            assert response.text not in FIRST_REASK_TRACK_RESPONSES
        else:
            assert response.directive.name == directives.names.OpenUriDirective

    def test_reask_positive(self, alice):
        response = alice('включи песню')
        self._assert_music_response(response, alice)

        response = alice('smells like teen spirit')
        self._assert_music_response(response, alice)

    def test_reask_negative(self, alice):
        response = alice('включи песню')
        self._assert_music_response(response, alice)

        response = alice('включи песню')
        self._assert_music_response(response, alice)

        response = alice('включи песню')
        self._assert_music_response(response, alice)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.watch])
class TestMusicReaskUnsupported2(object):

    owners = ('ardulat', )

    @staticmethod
    def _assert_music_response(response):
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ProhibitionError
        assert not response.directive
        assert response.text in [
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        ]

    def test_reask_positive(self, alice):
        response = alice('включи песню')
        self._assert_music_response(response)

        response = alice('smells like teen spirit')
        assert not response.directive
        assert response.intent != intent.MusicPlay

    def test_reask_negative(self, alice):
        response = alice('включи песню')
        self._assert_music_response(response)

        response = alice('включи песню')
        self._assert_music_response(response)

        response = alice('включи песню')
        self._assert_music_response(response)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
])
class TestMusicReaskWithoutPlus(object):

    owners = ('ardulat', )

    @staticmethod
    def _assert_music_response(response):
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert not response.directive
        assert response.text in [
            'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.',
            'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
            'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
        ]

    def test_reask_positive(self, alice):
        response = alice('включи песню')
        self._assert_music_response(response)

        response = alice('smells like teen spirit')
        self._assert_music_response(response)

    def test_reask_negative(self, alice):
        response = alice('включи песню')
        self._assert_music_response(response)

        response = alice('включи песню')
        self._assert_music_response(response)

        response = alice('включи песню')
        self._assert_music_response(response)


@pytest.mark.oauth(auth.Yandex)
class TestMusicReaskDeeplinkWithoutPlus(TestMusicReaskUnsupported1):
    pass
