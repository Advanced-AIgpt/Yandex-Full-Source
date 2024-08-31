import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments(
    'bg_fresh_granet_form=personal_assistant.scenarios.quasar.link_a_remote',
    'bg_fresh_granet_form=personal_assistant.scenarios.quasar.setup_rcu_check.status',
    'bg_fresh_granet_form=personal_assistant.scenarios.quasar.setup_rcu_manual.start',
    'bg_fresh_granet_form=personal_assistant.scenarios.quasar.setup_rcu.stop',
    'bg_fresh_granet_form=personal_assistant.scenarios.request_technical_support',
)
class TestLinkARemote(object):

    owners = ('flimsywhimsy')

    @pytest.mark.parametrize('surface', [surface.station_pro])
    @pytest.mark.parametrize('command', [
        'подключи пульт',
        'прицепи умный пульт управления яндекс',
        'алиса включи настройку умного пульта управления к станции',
    ])
    def test_link_a_remote(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.LinkARemote
        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.SetupRcuDirective

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_link_a_remote_fail(self, alice):
        response = alice('подключи пульт')
        assert response.scenario == scenario.LinkARemote
        assert response.text == 'Я не умею подключать пульт здесь.'

    @pytest.mark.parametrize('surface', [surface.station_pro])
    @pytest.mark.device_state(rcu={
        'is_rcu_connected': True,
        'setup_state': 0,
    })
    def test_link_a_remote_skip_setup_rcu(self, alice):
        response = alice('настрой пульт')
        assert response.scenario == scenario.LinkARemote
        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.SetupRcuAutoDirective

    @pytest.mark.parametrize('surface', [surface.station_pro])
    @pytest.mark.device_state(rcu={
        'is_rcu_connected': True,
    })
    @pytest.mark.parametrize('command, status, setup_state, expected_directives', [
        ('setup_rcu_status_frame', 'Success', 1, [directives.names.SetupRcuAutoDirective, directives.names.TtsPlayPlaceholderDirective]),
        ('setup_rcu_status_frame', 'Error', 1, [directives.names.SetupRcuDirective, directives.names.TtsPlayPlaceholderDirective]),
        ('setup_rcu_status_frame', 'InactiveTimeout', 1, []),
        ('setup_rcu_auto_status_frame', 'Success', 2, [directives.names.SetupRcuCheckDirective, directives.names.TtsPlayPlaceholderDirective]),
        ('setup_rcu_auto_status_frame', 'Error', 2, [directives.names.SetupRcuManualDirective, directives.names.TtsPlayPlaceholderDirective]),
        ('setup_rcu_check_status_frame', 'Success', 3, [directives.names.GoHomeDirective]),
        ('setup_rcu_check_status_frame', 'Error', 3, [directives.names.SetupRcuAdvancedDirective, directives.names.TtsPlayPlaceholderDirective]),
        ('setup_rcu_check_status_frame', 'InactiveTimeout', 3, []),
        ('setup_rcu_advanced_status_frame', 'Success', 4, [directives.names.SetupRcuCheckDirective, directives.names.TtsPlayPlaceholderDirective]),
        ('setup_rcu_advanced_status_frame', 'Error', 4, [directives.names.SetupRcuAdvancedDirective, directives.names.TtsPlayPlaceholderDirective]),
        ('setup_rcu_auto_start_frame', 'Samsung', 5, [directives.names.SetupRcuAutoDirective, directives.names.TtsPlayPlaceholderDirective]),
    ])
    def test_typed_semantic_frame(self, alice, command, status, setup_state, expected_directives):
        alice.device_state.RcuState.SetupState = setup_state

        response = alice.call(command, status)
        assert response.scenario == scenario.LinkARemote
        directive_names = [_.name for _ in response.directives]
        assert directive_names == expected_directives
