import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import pytest


YAMUSIC_AUDIOBRANDING_RESPONSES = [
    'Вот что я нашла для вас на Яндекс.Музыке. Надеюсь, понравится.',
    'На Яндекс.Музыке много всего, но я выбрала особенный трек.',
    'Минутку, залезу на Яндекс.Музыку. Вот, послушайте это.',
    'Секунду, выберу для вас что-нибудь на Яндекс.Музыке. Скажем, это.',
]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('yamusic_audiobranding_score=1')
@pytest.mark.parametrize('surface', [surface.station])
class TestYamusicAudiobranding(object):

    owners = ('abc:alice_scenarios_music',)

    def test_play_music(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text in YAMUSIC_AUDIOBRANDING_RESPONSES

    @pytest.mark.xfail(reason='Когда-нибудь и сказки переедут на Голливуд...')
    def test_play_fairy_tale(self, alice):
        response = alice('включи сказку о рыбаке и рыбке')

        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text in [
            'Выбрала для вас хорошую сказку на Яндекс.Музыке. Итак,...',
            'Мой юный друг, я кое-что нашла для вас на Яндекс.Музыке...',
            'У меня много любимых сказок на Яндекс.Музыке, а для вас я выбрала эту...',
        ]

    @pytest.mark.xfail(reason='Когда-нибудь и подкасты переедут на Голливуд...')
    def test_play_podcast(self, alice):
        response = alice('включи подкаст')

        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text in [
            'Вот один из подкастов Яндекс.Музыки.',
            'На Яндекс.Музыке много подкастов, как насчет этого?',
        ]
