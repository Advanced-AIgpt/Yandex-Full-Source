import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture


logger = logging.getLogger(__name__)

bass_stubber = create_localhost_bass_stubber_fixture()


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['music']


@pytest.fixture(scope='function')
def srcrwr_params(bass_stubber):
    return {
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}',
    }


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments('enable_continue_in_hw_music')
class TestsPlayerCommands:

    @pytest.mark.device_state(music={
        'currently_playing': {
            'track_id': 'track_id_1'
        }
    })
    def test_continue_with_track_id(self, alice):
        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run'}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('PlayerContinueDirective')
        player_continue = r.run_response.ResponseBody.Layout.Directives[0].PlayerContinueDirective
        assert player_continue.Player == 'music'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'
        return str(r)

    @pytest.mark.device_state(music={})
    def test_continue_empty_track_id(self, alice):
        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run'}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('MusicPlayDirective')
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'


@pytest.mark.parametrize('surface', [surface.station_pro])
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(
    'hw_music_thin_client',
    'hw_music_thin_playlists'
)
class TestsPlayerCommandsWithThinPlayerEnabled:

    @pytest.mark.device_state(music={
        'last_play_timestamp': 1579488271000,
        'player': {
            'pause': False,
        },
    }, video={
        'current_screen': 'music_player',
    })
    def test_next_track(self, alice):
        r = alice(voice('следующий трек'))
        assert r.run_response.Features.MusicFeatures.IsPlayerCommand
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('PlayerNextTrackDirective')
        assert r.run_response.ResponseBody.Layout.Directives[0].PlayerNextTrackDirective.Player == 'music'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.player_next_track'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'

    @pytest.mark.device_state(music={
        'last_play_timestamp': 1579488271000,
        'player': {
            'pause': False,
        },
    }, video={
        'current_screen': 'music_player',
    })
    def test_prev_track(self, alice):
        r = alice(voice('предыдущий трек'))
        assert r.run_response.Features.MusicFeatures.IsPlayerCommand
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('PlayerPreviousTrackDirective')
        assert r.run_response.ResponseBody.Layout.Directives[0].PlayerPreviousTrackDirective.Player == 'music'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.player_previous_track'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'

    @pytest.mark.experiments('enable_continue_in_hw_music')
    @pytest.mark.device_state(music={
        'last_play_timestamp': 1579488273000,
        'player': {
            'pause': True,
        },
        'currently_playing': {
            'track_id': 'track_id_1'
        },
    }, audio_player={
        'player_state': 'Stopped',
        'last_play_timestamp': 1579488272000,
    }, bluetooth={
        'last_play_timestamp': 1579488271000,
        'player': {
            'pause': True,
        },
        'currently_playing': {
            'track_id': 'track_id_1'
        },
    }, video={
        'current_screen': 'music_player',
    })
    def test_continue(self, alice):
        r = alice(voice('продолжи'))
        assert r.run_response.Features.MusicFeatures.IsPlayerCommand
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('PlayerContinueDirective')
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.player_continue'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'

    @pytest.mark.experiments('enable_shuffle_in_hw_music')
    @pytest.mark.device_state(music={
        'last_play_timestamp': 1579488271000,
        'player': {
            'pause': False,
        },
    }, video={
        'current_screen': 'music_player',
    })
    def test_shuffle(self, alice):
        r = alice(voice('перемешай треки'))
        assert r.run_response.Features.MusicFeatures.IsPlayerCommand
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('PlayerShuffleDirective')
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.player_shuffle'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'

    @pytest.mark.device_state(bluetooth={
        'last_play_timestamp': 1579488261000,
        'player': {
            'pause': False,
        },
        'currently_playing': {
            'track_id': 'track_id_1'
        },
    }, video={
        'current_screen': 'bluetooth',
    })
    def test_bluetooth_next_track(self, alice):
        r = alice(voice('следующий трек'))
        assert r.run_response.Features.MusicFeatures.IsPlayerCommand
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('PlayerNextTrackDirective')
        assert r.run_response.ResponseBody.Layout.Directives[0].PlayerNextTrackDirective.Player == 'bluetooth'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.player_next_track'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'

    @pytest.mark.device_state(bluetooth={
        'last_play_timestamp': 1579488261000,
        'player': {
            'pause': False,
        },
        'currently_playing': {
            'track_id': 'track_id_1'
        },
    }, video={
        'current_screen': 'bluetooth',
    })
    def test_bluetooth_prev_track(self, alice):
        r = alice(voice('предыдущий трек'))
        assert r.run_response.Features.MusicFeatures.IsPlayerCommand
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('PlayerPreviousTrackDirective')
        assert r.run_response.ResponseBody.Layout.Directives[0].PlayerPreviousTrackDirective.Player == 'bluetooth'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.player_previous_track'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'

    @pytest.mark.experiments('enable_continue_in_hw_music')
    @pytest.mark.device_state(bluetooth={
        'last_play_timestamp': 1579488263000,
        'player': {
            'pause': True,
        },
        'currently_playing': {
            'track_id': 'track_id_1'
        },
    }, music={
        'last_play_timestamp': 1579488262000,
        'player': {
            'pause': True,
        },
        'currently_playing': {
            'track_id': 'track_id_2'
        },
    }, audio_player={
        'player_state': 'Stopped',
        'last_play_timestamp': 1579488261000,
    }, video={
        'current_screen': 'bluetooth',
    })
    def test_bluetooth_continue(self, alice):
        r = alice(voice('продолжи'))
        assert r.run_response.Features.MusicFeatures.IsPlayerCommand
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.ResponseBody.Layout.Directives[0].HasField('PlayerContinueDirective')
        assert r.run_response.ResponseBody.Layout.Directives[0].PlayerContinueDirective.Name == \
            'bluetooth_player_continue'
        assert r.run_response.ResponseBody.Layout.Directives[0].PlayerContinueDirective.Player == 'bluetooth'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.player_continue'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'

    @pytest.mark.experiments('enable_shuffle_in_hw_music')
    @pytest.mark.device_state(bluetooth={
        'last_play_timestamp': 1579488261000,
        'player': {
            'pause': False,
        },
        'currently_playing': {
            'track_id': 'track_id_1'
        },
    }, video={
        'current_screen': 'bluetooth',
    })
    def test_bluetooth_shuffle(self, alice):
        r = alice(voice('перемешай треки'))
        assert r.run_response.Features.MusicFeatures.IsPlayerCommand
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.ResponseBody.Layout.OutputSpeech == \
            'К сожалению, этого я пока не умею. Но я быстро учусь.'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.player_shuffle'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'player_commands'
