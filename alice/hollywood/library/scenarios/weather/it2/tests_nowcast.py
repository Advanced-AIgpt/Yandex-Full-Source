import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Weather', handle='weather')
class TestsNowcast:

    @pytest.mark.parametrize('surface', [surface.loudspeaker])
    def tests_can_not_open_uri(self, alice):
        r = alice(voice('дождь идет в москве'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        return r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_can_open_uri(self, alice):
        r = alice(voice('дождь идет в москве'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].OpenUriDirective.Uri == \
            'https://yandex.ru/pogoda/maps/nowcast?appsearch_header=1&from=alice_raincard&lat=55.753215&lon=37.622504&utm_campaign=card&utm_content=fullscreen&utm_medium=nowcast&utm_source=alice'
        return r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_default(self, alice):
        r_default = alice(voice('дождь в питере'))
        r_now = alice(voice('дождь сейчас в питере'))
        r_today = alice(voice('дождь сегодня в питере'))
        assert r_default.run_response.ResponseBody.Layout.OutputSpeech == r_now.run_response.ResponseBody.Layout.OutputSpeech
        assert r_default.run_response.ResponseBody.Layout.OutputSpeech == r_today.run_response.ResponseBody.Layout.OutputSpeech
        assert r_default.run_response.ResponseBody.Layout.Directives == r_now.run_response.ResponseBody.Layout.Directives
        assert r_default.run_response.ResponseBody.Layout.Directives == r_today.run_response.ResponseBody.Layout.Directives

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_when_ends(self, alice):
        r = alice(voice('когда закончится дождь в саратове'))
        response = r.run_response.ResponseBody.Layout.OutputSpeech
        assert 'прекратится' in response or 'закончится' in response or 'не идёт' in response
        return response

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_today_day_part(self, alice):
        r = alice(voice('снег вечером в перми'))
        cards = r.run_response.ResponseBody.Layout.Cards
        assert len(cards) == 2
        return r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_tomorrow_day_part(self, alice):
        r = alice(voice('снег завтра вечером в перми'))
        cards = r.run_response.ResponseBody.Layout.Cards
        assert len(cards) == 1
        assert cards[0].HasField('DivCard')
        return r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_day(self, alice):  # which is actually a fallback on weather forecast scenario
        r = alice(voice('будет ли завтра дождь в париже'))
        cards = r.run_response.ResponseBody.Layout.Cards
        assert len(cards) == 1
        assert cards[0].HasField('DivCard')
        return r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_ellipsis_place(self, alice):
        alice(voice('какой сейчас дождь в москве'))
        r = alice(voice('а в красноярске'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_ellipsis_time(self, alice):
        alice(voice('какой сейчас дождь в москве'))
        r = alice(voice('а вечером'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_suggests(self, alice):
        r = alice(voice('какие осадки на улице'))
        assert r.run_response.ResponseBody.Layout.SuggestButtons
        return str(r.run_response.ResponseBody.Layout.SuggestButtons)
