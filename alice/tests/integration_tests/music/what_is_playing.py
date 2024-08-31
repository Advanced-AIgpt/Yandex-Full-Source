import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def _assert_what_is_playing_response(response, text):
    assert response.scenario == scenario.Vins
    assert response.intent == intent.MusicWhatIsPlaying
    assert text in response.text
    assert not response.directive


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestWhatIsPlaying(object):
    owners = ('olegator', )

    @pytest.mark.device_state(music={
        'player': {
            'pause': False,
        },
        'currently_playing': {
            'track_info': {
                'artists': [
                    {
                        'name': 'Imagine Dragons',
                        'id': 675068,
                    },
                ],
                'id': '42197229',
                'type': 'music',
                'title': 'Natural',
                'albums': [
                    {
                        'genre': 'rock',
                        'artists': [
                            {
                                'name': 'Imagine Dragons',
                                'id': 675068,
                            },
                        ],
                        'id': 6017186,
                        'title': 'Origins',
                        'metaType': 'music',
                    },
                ],
            },
            'track_id': '42197229',
        },
    })
    def test_music(self, alice):
        response = alice('что играет?')
        _assert_what_is_playing_response(response, 'песня')

    @pytest.mark.device_state(music={
        'player': {
            'pause': False,
        },
        'currently_playing': {
            'track_info': {
                'artists': [
                    {
                        'name': 'Сказки',
                        'id': 219352,
                    },
                ],
                'id': '27736941',
                'type': 'fairy-tale',
                'title': 'Храбрый портняжка',
                'albums': [
                    {
                        'genre': 'fairytales',
                        'artists': [
                            {
                                'name': 'Сказки',
                                'id': 219352,
                            },
                        ],
                        'id': 3317192,
                        'type': 'fairy-tale',
                        'title': 'Книга добрых сказок. Братья Гримм. Золотой гусь',
                        'metaType': 'podcast',
                    },
                ],
            },
            'track_id': '27736941',
        },
    })
    def test_fairytale(self, alice):
        response = alice('что играет?')
        _assert_what_is_playing_response(response, 'сказка')

    @pytest.mark.device_state(music={
        'player': {
            'pause': False,
        },
        'currently_playing': {
            'track_info': {
                'artists': [],
                'id': '79897777',
                'type': 'podcast-episode',
                'title': 'Шоу "Stand Up" на ТНТ. Виктория Складчикова и Иван Абрамов',
                'albums': [
                    {
                        'genre': 'comedypodcasts',
                        'artists': [],
                        'id': 13779940,
                        'type': 'podcast',
                        'title': 'Шоу Stand Up на ТНТ',
                        'metaType': 'podcast',
                    },
                ],
            },
            'track_id': '79897777',
        },
    })
    def test_podcast(self, alice):
        response = alice('что играет?')
        _assert_what_is_playing_response(response, 'выпуск')
