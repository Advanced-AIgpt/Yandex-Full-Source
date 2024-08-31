import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


ROBOT_MULTIROOM_YANDEX_STATION_2_ID = '0ccc849b-4d29-4c13-844d-3968aa7475f3'
ROBOT_MULTIROOM_OTHER_SMART_DEVICE_ID = 'f48b7ee3-06cf-4ce0-bbe7-a58d20616a6c'
ROBOT_MULTIROOM_YANDEX_STATION_2_QUASAR_ID = 'feedface-e8a2-4439-b2e7-000000000001.yandexstation_2'
ROBOT_MULTIROOM_OTHER_SMART_DEVICE_QUASAR_ID = 'feedface-e8a2-4439-b2e7-000000000002.unknown_platform'

ROBOT_MULTIROOM_KITCHEN_ROOM_ID = 'b1cf2336-698b-4b8d-8e23-bed112dba7c0'
ROBOT_MULTIROOM_GROUP_ONE_ID = '0cbc849b-4d29-4c13-844d-3968aa7475f3'
ROBOT_MULTIROOM_KITCHEN_DEVICES_IDS = [ROBOT_MULTIROOM_OTHER_SMART_DEVICE_QUASAR_ID]
ROBOT_MULTIROOM_BEDROOM_DEVICES_IDS = [ROBOT_MULTIROOM_YANDEX_STATION_2_QUASAR_ID]

ROBOT_MULTIROOM_GROUP_ONE_DEVICES_IDS = [
    *ROBOT_MULTIROOM_BEDROOM_DEVICES_IDS,
    *ROBOT_MULTIROOM_KITCHEN_DEVICES_IDS,
]

ROBOT_MULTIROOM_ALL_DEVICES_IDS = ROBOT_MULTIROOM_GROUP_ONE_DEVICES_IDS

ALL_ROOM_ID = '__all__'
ALL_DEVICES_IN_CURRENT_GROUP = 'all'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class _TestMultiroomCommon(object):

    owners = ('zubchick', 'igor-darov',)

    multiroom_not_supported_response_text = 'Я ещё не научилась играть музыку на разных устройствах одновременно'
    rooms_not_supported_response_text = ('Я еще не научилась включать музыку в разных комнатах, '
                                         'но вы можете попросить меня включить музыку везде')

    robot_multiroom_room_id_to_devices_ids = {
        ROBOT_MULTIROOM_KITCHEN_ROOM_ID: ROBOT_MULTIROOM_KITCHEN_DEVICES_IDS,
        ROBOT_MULTIROOM_GROUP_ONE_ID: ROBOT_MULTIROOM_GROUP_ONE_DEVICES_IDS,
        ALL_ROOM_ID: ROBOT_MULTIROOM_ALL_DEVICES_IDS,
        ALL_DEVICES_IN_CURRENT_GROUP: ROBOT_MULTIROOM_ALL_DEVICES_IDS,
        ROBOT_MULTIROOM_YANDEX_STATION_2_ID: [ROBOT_MULTIROOM_YANDEX_STATION_2_QUASAR_ID],
        ROBOT_MULTIROOM_OTHER_SMART_DEVICE_ID: [ROBOT_MULTIROOM_OTHER_SMART_DEVICE_QUASAR_ID],
    }

    def _check_multiroom_directive(self, directive, room_id, directive_name=directives.names.StartMultiroomDirective):
        assert directive.name == directive_name
        assert directive.payload.room_id == room_id
        assert room_id in self.robot_multiroom_room_id_to_devices_ids
        room_device_ids = directive.get('room_device_ids', directive.payload.get('room_device_ids'))
        assert room_device_ids == self.robot_multiroom_room_id_to_devices_ids[room_id]

    def _check_multiroom_response(self, response, room_id):
        assert response.scenario == scenario.HollywoodMusic
        assert len(response.directives) == 2
        self._check_multiroom_directive(directive=response.directives[0], room_id=room_id)
        assert response.directives[1].name == directives.names.MusicPlayDirective

    @staticmethod
    def _check_multiroom_not_supported(response, message):
        assert response.scenario == scenario.HollywoodMusic
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == message


class TestAutoPlayMusicInMultiroomGroup(_TestMultiroomCommon):

    @pytest.mark.app(device_id=ROBOT_MULTIROOM_BEDROOM_DEVICES_IDS[0])
    @pytest.mark.oauth(auth.RobotMultiroom)
    def test_start_music(self, alice):
        response = alice('включи музыку')
        self._check_multiroom_response(response=response, room_id=ALL_DEVICES_IN_CURRENT_GROUP)


class TestPlayMusicEverywhereInMultiroom(_TestMultiroomCommon):

    play_everywhere_commands = [
        'включи музыку везде',
        'включи везде yesterday',
        'включи на всех колонках beatles',
        'включи музыку на всем',
    ]

    def test_no_multiroom(self, alice):
        response = alice('включи мощный хит')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective

    @pytest.mark.oauth(auth.RobotMultiroom)
    @pytest.mark.parametrize('command', play_everywhere_commands)
    def test_play_everywhere_with_multiroom(self, alice, command):
        response = alice(command)
        self._check_multiroom_response(response=response, room_id=ALL_ROOM_ID)

    @pytest.mark.supported_features(multiroom=None)
    @pytest.mark.parametrize('command', play_everywhere_commands)
    def test_play_everywhere_without_multiroom(self, alice, command):
        response = alice(command)
        self._check_multiroom_not_supported(response=response, message=self.multiroom_not_supported_response_text)

    @pytest.mark.experiments('disable_multiroom')
    @pytest.mark.parametrize('command', play_everywhere_commands)
    def test_play_everywhere_with_multiroom_feature_only(self, alice, command):
        response = alice(command)
        self._check_multiroom_not_supported(response=response, message=self.multiroom_not_supported_response_text)

    @pytest.mark.experiments('disable_multiroom')
    @pytest.mark.supported_features(multiroom=None)
    @pytest.mark.parametrize('command', play_everywhere_commands)
    def test_play_everywhere_without_multiroom_feature_and_with_disabling_exp(self, alice, command):
        response = alice(command)
        assert len(response.directives) == 1
        assert response.text != self.multiroom_not_supported_response_text
        assert response.directive.name == directives.names.MusicPlayDirective


class TestPlayMusicAtLocationInMultiroom(_TestMultiroomCommon):

    owners = ('igor-darov',)

    @pytest.mark.oauth(auth.RobotMultiroom)
    def test_play_music_in_room(self, alice):
        response = alice('включи музыку в комнате кухня')
        self._check_multiroom_response(response=response, room_id=ROBOT_MULTIROOM_KITCHEN_ROOM_ID)

    @pytest.mark.oauth(auth.RobotMultiroom)
    def test_play_music_in_group(self, alice):
        response = alice('включи beatles на группе группа 1')
        self._check_multiroom_response(response=response, room_id=ROBOT_MULTIROOM_GROUP_ONE_ID)

    @pytest.mark.oauth(auth.RobotMultiroom)
    def test_play_music_at_device(self, alice):
        response = alice('включи scorpions на яндекс станции 2')
        self._check_multiroom_response(response=response, room_id=ROBOT_MULTIROOM_YANDEX_STATION_2_ID)

    @pytest.mark.oauth(auth.RobotMultiroom)
    @pytest.mark.supported_features(multiroom_cluster=None)
    def test_no_multiroom_cluster_client_feature(self, alice):
        response = alice('включи музыку в комнате кухня')
        self._check_multiroom_not_supported(response=response, message=self.rooms_not_supported_response_text)

    @pytest.mark.oauth(auth.RobotMultiroom)
    @pytest.mark.experiments('disable_multiroom')
    def test_play_music_in_room_with_disabling_exp(self, alice):
        response = alice('включи музыку в комнате кухня')
        self._check_multiroom_not_supported(response=response, message=self.multiroom_not_supported_response_text)


class TestContinueCurrentTrackInMultiroom(_TestMultiroomCommon):

    owners = ('igor-darov',)

    @staticmethod
    def _check_start_music_response(response):
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective

    def _check_start_multiroom_from_current_track_response(self, response, room_id):
        assert response.scenario == scenario.HollywoodMusic
        self._check_multiroom_directive(directive=response.directive, room_id=room_id)

    @pytest.mark.oauth(auth.RobotMultiroom)
    @pytest.mark.parametrize('command', [
        'включи эту музыку везде',
        'включи это музыку на всех колонках',
    ])
    def test_continue_everywhere(self, alice, command):
        response = alice('включи музыку')
        self._check_start_music_response(response=response)

        response = alice(command)
        self._check_start_multiroom_from_current_track_response(response=response, room_id=ALL_ROOM_ID)

    @pytest.mark.oauth(auth.RobotMultiroom)
    @pytest.mark.parametrize('command, location_id', [
        pytest.param('включи это на кухне', ROBOT_MULTIROOM_KITCHEN_ROOM_ID, id='kitchen'),
        pytest.param('продолжи играть эту музыку на группе 1', ROBOT_MULTIROOM_GROUP_ONE_ID, id='group-1'),
        pytest.param('включи это на яндекс станции 2', ROBOT_MULTIROOM_YANDEX_STATION_2_ID, id='device'),
    ])
    def test_continue_at_location(self, alice, command, location_id):
        response = alice('включи музыку')
        self._check_start_music_response(response=response)

        response = alice(command)
        self._check_start_multiroom_from_current_track_response(response=response, room_id=location_id)


class TestCommandsInMultiroom(_TestMultiroomCommon):

    owners = ('igor-darov',)

    some_multiroom_session_id = 'multiroom_session_id_123'
    device_state = {
        'multiroom': {
            'multiroom_session_id': some_multiroom_session_id
        },
        'music': {
            'player': {
                'pause': False,
            },
        },
        'video': {
            'current_screen': 'music_player'
        }
    }

    def _check_multiroom_session_id_gets_filled(self, response, expected_directive):
        assert len(response.directives) == 1
        assert response.directive.name == expected_directive
        assert response.directive.multiroom_session_id == self.some_multiroom_session_id

    @pytest.mark.oauth(auth.RobotMultiroom)
    @pytest.mark.app(device_id=ROBOT_MULTIROOM_BEDROOM_DEVICES_IDS[0])
    def test_player_pause(self, alice):
        response = alice('стоп')
        assert response.scenario == scenario.Commands
        assert response.directives[0].name == directives.names.PlayerPauseDirective
        assert response.directives[0].multiroom_session_id == self.some_multiroom_session_id

    @pytest.mark.oauth(auth.RobotMultiroom)
    @pytest.mark.app(device_id=ROBOT_MULTIROOM_BEDROOM_DEVICES_IDS[0])
    @pytest.mark.parametrize('command, expected_directive, expected_scenario', [
        pytest.param(
            'следующую песню', directives.names.PlayerNextTrackDirective, scenario.HollywoodMusic, id='next_track',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-785'),
        ),
        pytest.param(
            'предыдущую песню', directives.names.PlayerPreviousTrackDirective, scenario.HollywoodMusic, id='prev_track',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-785'),
        ),
        pytest.param('громче', directives.names.SoundLouderDirective, scenario.Commands, id='louder'),
        pytest.param('тише', directives.names.SoundQuiterDirective, scenario.Commands, id='quiet'),
        pytest.param('громкость два', directives.names.SoundSetLevelDirective, scenario.Commands, id='set_volume'),
    ])
    def test_commands(self, alice, command, expected_directive, expected_scenario):
        response = alice(command)
        assert response.scenario == expected_scenario
        self._check_multiroom_session_id_gets_filled(response, expected_directive=expected_directive)


class TestRemoteMultiroomCommands(_TestMultiroomCommon):

    owners = ('igor-darov',)

    @pytest.mark.oauth(auth.RobotMultiroom)
    @pytest.mark.parametrize('command, location_id', [
        pytest.param('стоп на кухне', ROBOT_MULTIROOM_KITCHEN_ROOM_ID, id='kitchen'),
        pytest.param('выключи музыку в группе 1', ROBOT_MULTIROOM_GROUP_ONE_ID, id='group-1'),
        pytest.param('стоп везде', ALL_ROOM_ID, id='everywhere'),
        pytest.param('стоп на всех колонках', ALL_ROOM_ID, id='everywhere-2'),
        pytest.param('стоп на яндекс станции 2', ROBOT_MULTIROOM_YANDEX_STATION_2_ID, id='device'),
    ])
    def test_stop_at_location(self, alice, command, location_id):
        response = alice(command)
        assert response.scenario == scenario.Commands
        assert len(response.directives) == 1
        self._check_multiroom_directive(
            response.directive,
            room_id=location_id,
            directive_name=directives.names.PlayerPauseDirective
        )
