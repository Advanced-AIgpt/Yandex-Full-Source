import logging

import pytest
from alice.acceptance.modules.request_generator.lib.vins import make_asr_result
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.scenarios.reask.proto.state_pb2 import TReaskState


logger = logging.getLogger(__name__)


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['reask']


def make_multiple_asr_results(asr_hypos):
    result = []
    for asr_hypo in asr_hypos:
        result += make_asr_result(asr_hypo)

    return result


def _assert_irrelevant_response(response):
    assert response.Features.IsIrrelevant
    layout = response.ResponseBody.Layout
    assert not layout.OutputSpeech
    assert not layout.Directives


def _assert_reask_response(response):
    assert not response.Features.IsIrrelevant

    reaskState = TReaskState()
    response.ResponseBody.State.Unpack(reaskState)
    assert reaskState.ReaskCount == 1

    layout = response.ResponseBody.Layout
    assert layout.OutputSpeech in [
        'Простите, я что-то глуховата. Повторите еще раз.',
        'Повторите, пожалуйста!',
        'Уточните, что именно включить?',
    ]
    assert not layout.Directives


@pytest.mark.scenario(name='Reask', handle='reask')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('bg_fresh_granet_form=alice.reask_play')
class Tests:

    def test_reask(self, alice):
        r = alice(voice('включи', asr_result=make_multiple_asr_results(['включи', 'включи foo'])))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response)
        return str(r)

    def test_already_reasked(self, alice):
        r = alice(voice('включи', asr_result=make_multiple_asr_results(['включи', 'включи foo'])))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response)

        r = alice(voice('включи', asr_result=make_multiple_asr_results(['включи', 'включи foo'])))
        assert r.scenario_stages() == {'run'}
        _assert_irrelevant_response(r.run_response)

    def test_asr_hypotheses(self, alice):
        r = alice(voice('включи', asr_result=make_multiple_asr_results(['включи', 'выключи'])))
        assert r.scenario_stages() == {'run'}
        _assert_irrelevant_response(r.run_response)

    @pytest.mark.experiments('reask_skip_asr_hypo')
    def test_skip_asr_hypotheses(self, alice):
        r = alice(voice('включи'))
        assert r.scenario_stages() == {'run'}
        _assert_reask_response(r.run_response)


@pytest.mark.scenario(name='Reask', handle='reask')
@pytest.mark.parametrize('surface', [surface.searchapp])
@pytest.mark.experiments('bg_fresh_granet_form=alice.reask_play')
def test_not_smart_speaker(alice):
    r = alice(voice('включи', asr_result=make_multiple_asr_results(['включи', 'включи foo'])))
    assert r.scenario_stages() == {'run'}
    _assert_irrelevant_response(r.run_response)
