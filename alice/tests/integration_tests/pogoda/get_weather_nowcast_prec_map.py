import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest

from .common import assert_is_prec_map_case, assert_is_not_prec_map_case, assert_is_nowcast_case, common_where


_smoke_requests = [
    'карта осадков в москве',
]

_basic_requests = [
    'погодная карта яндекс',
    'открой-ка карту с осадками',
    'карта осадков мне конкретно в санкт-петербурге интересно',
    'покажи казань на карте осадков',
    'покажи осадки на карте',
]

_positives = [
    *_basic_requests,
    *[f'карта осадков {where}'.strip() for where in common_where],
]

_negatives = [
    'какой дождь в москве',
    'карта москвы',
    'сыграем в карты',
]

_ellipsis_positives = [
    'а в красноярске',
    'давай еще в чикаго',
]

_ellipsis_negatives = [
    'а на завтра',
    'карта осадков в красноярске',
]


class TestPrecMap(object):

    owners = ('abc:weatherbackendvteam', )

    @pytest.mark.parametrize('command', _smoke_requests)
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.navi, surface.yabro_win])
    def test_prec_map_smoke(self, alice, command):
        response = alice(command)
        assert_is_prec_map_case(response)
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri in [
            'https://yandex.ru/pogoda/maps/nowcast?appsearch_header=1&from=alice_raincard&lat=55.753215&lon=37.622504&utm_campaign=card&utm_content=fullscreen&utm_medium=nowcast&utm_source=alice',
            'https://yandex.ru/pogoda/maps/nowcast?from=alice_raincard&lat=55.753215&lon=37.622504&utm_campaign=card&utm_content=fullscreen&utm_medium=nowcast&utm_source=alice',
        ]

    @pytest.mark.parametrize('command', _positives)
    @pytest.mark.parametrize('surface', [surface.searchapp, surface.navi, surface.yabro_win])
    def test_prec_map_positive(self, alice, command):
        response = alice(command)
        assert_is_prec_map_case(response)

    @pytest.mark.parametrize('command', _negatives)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_prec_map_negative(self, alice, command):
        response = alice(command)
        assert_is_not_prec_map_case(response)

    @pytest.mark.parametrize('command', _ellipsis_positives)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_prec_map_ellipsis_positive(self, alice, command):
        alice('карта осадков в москве')
        response = alice(command)
        assert_is_prec_map_case(response, ellipsis=True)
        assert response.text.startswith('Открываю карту осадков')
        assert response.directive.name == directives.names.OpenUriDirective

    @pytest.mark.parametrize('command', _ellipsis_negatives)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_prec_map_ellipsis_negative(self, alice, command):
        alice('карта осадков в москве')
        response = alice(command)
        assert_is_not_prec_map_case(response, ellipsis=True)

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [surface.loudspeaker, surface.automotive])
    def test_prec_map_can_not_open_uri(self, alice):  # fallback on nowcast scenario
        response = alice('карта осадков')
        assert_is_nowcast_case(response)
        assert response.text.startswith('У меня не получается открыть карту осадков, однако могу сообщить вам, что')
        assert response.output_speech_text.startswith('У меня не получается открыть карту осадков, однако могу сообщить вам, что')
