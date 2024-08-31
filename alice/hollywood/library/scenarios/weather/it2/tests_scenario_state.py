import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.station])
class _TestsScenarioStateBase:
    pass


class TestsStateDrop(_TestsScenarioStateBase):

    STATE_DROP_TIME_SECONDS = 5 * 60  # 5 minutes

    @pytest.mark.parametrize('deadline_delta', [
        pytest.param(-1, id='drop_no'),
        pytest.param(0, id='drop_yes'),
    ])
    def test(self, alice, deadline_delta):
        res = ''

        r = alice(voice('погода'))
        res += r.run_response.ResponseBody.Layout.OutputSpeech + '\n'

        skip_seconds = self.STATE_DROP_TIME_SECONDS + deadline_delta
        alice.skip(seconds=skip_seconds)

        r = alice(voice('а завтра'))
        res += r.run_response.ResponseBody.Layout.OutputSpeech + '\n'

        return res
