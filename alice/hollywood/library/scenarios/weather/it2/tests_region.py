import pytest
from alice.hollywood.library.python.testing.it2 import surface, region
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.station])
class TestsRegion:

    @pytest.mark.parametrize('dummy', [
        pytest.param(1, id='undefined', marks=pytest.mark.region(region.Undefined)),
        pytest.param(2, id='zero_location', marks=pytest.mark.region(region.ZeroLocation)),
    ])
    def test_bad_client_region(self, alice, dummy):
        r = alice(voice('погода'))
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Чтобы ответить на этот вопрос мне нужно знать ваше местоположение. Но мне не удалось его определить.',
            'Мне не удалось определить где вы находитесь.',
            'Чтобы дать ответ мне нужно знать ваше местоположение, но я не смогла его определить.',
            'Не могу определить ваше местоположение.',
            'Не могу определить, где вы находитесь.',
            'Я не знаю, где вы сейчас находитесь, и поэтому не могу дать ответ.',
        ]
