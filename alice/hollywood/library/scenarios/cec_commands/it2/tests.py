import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['cec_commands']


SCREEN_OFF_COMMAND = 'выключи экран'
SCREEN_ON_COMMAND = 'включи экран'

CEC_SURFACES = [surface.station_pro]
NON_CEC_SURFACES = [s for s in surface.actual_surfaces if s not in CEC_SURFACES]


@pytest.mark.scenario(name='CecCommands', handle='cec_commands')
@pytest.mark.experiments('bg_fresh_granet')  # TODO(vl-trifonov) remove when delivered
class _TestsCecBase:
    def _assert_run_w_directive(self, r, directive):
        assert r.scenario_stages() == {'run'}
        assert r.is_run_relevant()

        response_body = r.run_response.ResponseBody
        response_proto_layout = response_body.Layout
        directives = response_proto_layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField(directive)

        assert response_proto_layout.OutputSpeech
        assert response_body.AnalyticsInfo.ProductScenarioName == 'cec_commands'

    def _assert_run_wo_directive(self, r):
        assert r.scenario_stages() == {'run'}
        assert not r.is_run_relevant()

        response_body = r.run_response.ResponseBody
        response_proto_layout = response_body.Layout
        directives = response_proto_layout.Directives
        assert len(directives) == 0

        assert response_proto_layout.OutputSpeech
        assert response_body.AnalyticsInfo.ProductScenarioName == 'cec_commands'


@pytest.mark.parametrize('surface', CEC_SURFACES)
class TestsCecSurfaces(_TestsCecBase):
    @pytest.mark.device_state(is_tv_plugged_in=True)
    def test_screen_off(self, alice):
        r = alice(voice(SCREEN_OFF_COMMAND))
        self._assert_run_w_directive(r, 'ScreenOffDirective')

    @pytest.mark.device_state(is_tv_plugged_in=True)
    def test_screen_on(self, alice):
        r = alice(voice(SCREEN_ON_COMMAND))
        self._assert_run_w_directive(r, 'ScreenOnDirective')

    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('command', [
        pytest.param(SCREEN_ON_COMMAND, id='screen_on_command'),
        pytest.param(SCREEN_OFF_COMMAND, id='screen_off_command')
    ])
    def test_screen_unplugged(self, alice, command):
        r = alice(voice(command))
        self._assert_run_wo_directive(r)
        assert 'не подключен' in r.run_response.ResponseBody.Layout.Cards[0].Text


@pytest.mark.parametrize('surface', NON_CEC_SURFACES)
class TestsNonCecSurfaces(_TestsCecBase):
    def test_screen_off(self, alice):
        r = alice(voice(SCREEN_OFF_COMMAND))
        self._assert_run_wo_directive(r)
        assert 'не умею' in r.run_response.ResponseBody.Layout.Cards[0].Text

    def test_screen_on(self, alice):
        r = alice(voice(SCREEN_ON_COMMAND))
        self._assert_run_wo_directive(r)
        assert 'не умею' in r.run_response.ResponseBody.Layout.Cards[0].Text
