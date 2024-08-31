import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.experiments('bg_fresh_granet_prefix=alice.scenarios.get_weather')
@pytest.mark.scenario(name='Weather', handle='weather')
class TestsPrecMap:

    @pytest.mark.parametrize('surface', [surface.loudspeaker])
    def test_can_not_open_uri(self, alice):
        r = alice(voice('карта осадков в москве'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert not directives
        return r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_can_open_uri(self, alice):
        r = alice(voice('карта осадков в москве'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].OpenUriDirective.Uri == \
            'https://yandex.ru/pogoda/maps/nowcast?appsearch_header=1&from=alice_raincard&lat=55.753215&lon=37.622504&utm_campaign=card&utm_content=fullscreen&utm_medium=nowcast&utm_source=alice'
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Открываю карту осадков в Москве.'
        assert r.run_response.ResponseBody.Layout.Cards[0].HasField('TextWithButtons')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_ellipsis_place(self, alice):
        alice(voice('карта осадков в москве'))
        r = alice(voice('а теперь в красноярске'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].OpenUriDirective.Uri == \
            'https://yandex.ru/pogoda/maps/nowcast?appsearch_header=1&from=alice_raincard&lat=56.010563&lon=92.852572&utm_campaign=card&utm_content=fullscreen&utm_medium=nowcast&utm_source=alice'
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Открываю карту осадков в Красноярске.'
