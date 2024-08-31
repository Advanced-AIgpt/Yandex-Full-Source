import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice


logger = logging.getLogger(__name__)

EXPERIMENTS = [
    'hw_music_thin_client',
]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.watch])
class _TestsWatchBase:
    pass


class TestsWatch(_TestsWatchBase):

    def test_simple(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert not layout.Directives
        assert layout.OutputSpeech in [
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        ]
