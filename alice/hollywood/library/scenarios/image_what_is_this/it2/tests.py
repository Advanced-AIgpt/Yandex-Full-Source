import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['image_what_is_this']


@pytest.mark.scenario(name='ImageWhatIsThis', handle='image_what_is_this')
class TestsShowPromo:

    @pytest.mark.parametrize('surface', [surface.webtouch])
    def test_webtouch(self, alice):
        r = alice(voice('что изображено на картинке'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech
        assert r.run_response.ResponseBody.Layout.Directives[0].ShowPromoDirective
