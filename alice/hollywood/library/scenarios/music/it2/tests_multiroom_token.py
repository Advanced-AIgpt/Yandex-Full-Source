import logging
import pytest
import re

from alice.hollywood.library.framework.proto.framework_state_pb2 import TProtoHwFramework
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import server_action, voice, callback, Scenario
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import get_callback_from_reset_add_effect
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import proto_to_dict
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TScenarioState
from alice.megamind.protos.common.frame_pb2 import TMusicPlaySemanticFrame, TMusicPlayObjectTypeSlot
from conftest import get_scenario_state

from alice.megamind.protos.common.device_state_pb2 import TDeviceState


logger = logging.getLogger(__name__)

bass_stubber = create_localhost_bass_stubber_fixture()


@pytest.fixture(scope='module')
def enabled_scenarios():
    # We need 'fast_command' because 'stop/pause' command lives there in (Commands scenario)
    return ['music', 'fast_command']


MULTIROOM_EXPS = [
    'hw_music_multiroom_client_redirect',
    'hw_music_thin_client',
    'hw_music_thin_client_generative',
    'hw_music_thin_client_multiroom',
    'hw_music_thin_client_playlist',
    'commands_multiroom_client_redirect',
]


DEFAULT_MULTIROOM_TOKEN = 'TestMultiroomToken'


def _make_default_scenario_state():
    state = TProtoHwFramework()
    state.ScenarioState.Pack(TScenarioState(MultiroomToken=DEFAULT_MULTIROOM_TOKEN))
    return state


DEFAULT_SCENARIO_STATE = _make_default_scenario_state()


SIMPLE_IOT_USER_INFO = '''
  {
    "devices": [
      {
        "quasar_info": {
          "device_id": "device_1"
        }
      },
      {
        "quasar_info": {
          "device_id": "device_2"
        }
      }
    ]
  }
'''


SINGLE_PLAYING_DEVICE_STATE = {
    'device_id': 'device_1',
    'multiroom': {
        'visible_peers': [
            'device_2',
        ],
        'multiroom_token': DEFAULT_MULTIROOM_TOKEN,
    },
    'audio_player': {
        'last_play_timestamp': 1579488271000,
        'player_state': 2,  # 'Playing'
    },
}


LOCATIONS_IOT_USER_INFO = '''
    {
        "devices": [
            {
                "id": "station_in_the_kitchen_1",
                "group_ids": ["floor"],
                "room_id": "kitchen",
                "quasar_info": {
                    "device_id": "station_in_the_kitchen_1"
                }
            },
            {
                "id": "station_in_the_kitchen_2",
                "group_ids": [],
                "room_id": "kitchen",
                "quasar_info": {
                    "device_id": "station_in_the_kitchen_2"
                }
            },
            {
                "id": "mini_in_the_kitchen_1",
                "group_ids": ["minis"],
                "room_id": "kitchen",
                "quasar_info": {
                    "device_id": "mini_in_the_kitchen_1"
                }
            },
            {
                "id": "station_in_the_bedroom_1",
                "group_ids": ["floor"],
                "room_id": "bedroom",
                "quasar_info": {
                    "device_id": "station_in_the_bedroom_1"
                }
            },
            {
                "id": "mini_in_the_bedroom_1",
                "group_ids": ["minis"],
                "room_id": "bedroom",
                "quasar_info": {
                    "device_id": "mini_in_the_bedroom_1"
                }
            },
            {
                "id": "mini_in_the_bedroom_2",
                "group_ids": ["minis"],
                "room_id": "bedroom",
                "quasar_info": {
                    "device_id": "mini_in_the_bedroom_2"
                }
            }
        ],
        "rooms": [
            {
                "id": "kitchen",
                "name": "кухня"
            },
            {
                "id": "bedroom",
                "name": "спальня"
            },
            {
                "id": "orangerie",
                "name": "оранжерея"
            }
        ],
        "groups": [
            {
                "id": "minis",
                "name": "миники"
            },
            {
                "id": "floor",
                "name": "пол"
            }
        ]
    }
    '''


def build_location_playing_device_state(is_slave, is_playing, is_in_group):
    if is_slave:
        mode = TDeviceState.TMultiroom.EMultiroomMode.Slave
    else:
        mode = TDeviceState.TMultiroom.EMultiroomMode.Master

    if is_playing:
        player_state = TDeviceState.TAudioPlayer.TPlayerState.Playing
    else:
        player_state = TDeviceState.TAudioPlayer.TPlayerState.Stopped

    if is_in_group:
        device_id = 'station_in_the_bedroom_1'
        room_device_ids = [
            'mini_in_the_bedroom_2',
            'station_in_the_bedroom_1',
        ]
    else:
        device_id = 'station_in_the_kitchen_2'
        room_device_ids = [
            'mini_in_the_bedroom_2',
            'station_in_the_kitchen_2',
        ]

    visible_peers = {
        'station_in_the_kitchen_1',
        'station_in_the_kitchen_2',
        'mini_in_the_kitchen_1',
        'station_in_the_bedroom_1',
        'mini_in_the_bedroom_1',
        'mini_in_the_bedroom_2',
    }
    visible_peers.remove(device_id)

    return {
        'device_id': device_id,  # is excluded from 'visible_peers'
        'multiroom': {
            'visible_peers': list(visible_peers),
            'mode': mode,
            'master_device_id': 'mini_in_the_bedroom_2' if is_slave else device_id,  # MASTER DEVICE ID
            'multiroom_session_id': 'blahblahblah',
            'multiroom_token': DEFAULT_MULTIROOM_TOKEN,
            'room_device_ids': room_device_ids,
        },
        'audio_player': {
            'last_play_timestamp': 1579488271000,
            'player_state': player_state,
        },
    }


LOCATION_PLAYING_DEVICE_STATE_SLAVE = build_location_playing_device_state(is_slave=True, is_playing=True, is_in_group=True)
LOCATION_PLAYING_DEVICE_STATE_MASTER = build_location_playing_device_state(is_slave=False, is_playing=True, is_in_group=True)

LOCATION_NON_PLAYING_DEVICE_STATE_SLAVE = build_location_playing_device_state(is_slave=True, is_playing=False, is_in_group=True)
LOCATION_NON_PLAYING_DEVICE_STATE_MASTER = build_location_playing_device_state(is_slave=False, is_playing=False, is_in_group=True)

LOCATION_PLAYING_DEVICE_STATE_SLAVE_OUT_OF_GROUP = build_location_playing_device_state(is_slave=True, is_playing=True, is_in_group=False)
LOCATION_PLAYING_DEVICE_STATE_MASTER_OUT_OF_GROUP = build_location_playing_device_state(is_slave=False, is_playing=True, is_in_group=False)


@pytest.mark.experiments(*MULTIROOM_EXPS)
@pytest.mark.supported_features('multiroom', 'multiroom_cluster', 'multiroom_audio_client')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.scenario_state(DEFAULT_SCENARIO_STATE)  # THIS IS VERY IMPORTANT!!!
class _TestsMultiroomTokenBase:

    def _get_response(self, response):
        stages = response.scenario_stages()
        if 'apply' in stages:
            return response.apply_response
        elif 'continue' in stages:
            return response.continue_response
        elif 'commit' in stages:
            return response.run_response.CommitCandidate
        else:
            return response.run_response

    def _get_response_body(self, response):
        return self._get_response(response).ResponseBody

    def _get_scenario_state(self, response):
        return get_scenario_state(self._get_response(response))

    def _directives(self, response):
        response_body = self._get_response_body(response)
        return response_body.Layout.Directives

    def _directives_count(self, response):
        return len(self._directives(response))

    def _get_directive(self, response, directive_name):
        for d in self._directives(response):
            if d.HasField(directive_name):
                return getattr(d, directive_name)
        return None

    def _get_pause_directive(self, response):
        return self._get_directive(response, 'PlayerPauseDirective')

    def _get_push_directive(self, response):
        return self._get_directive(response, 'MultiroomSemanticFrameDirective')

    def _get_audio_play_directive(self, response):
        return self._get_directive(response, 'AudioPlayDirective')

    def _get_start_multiroom_directive(self, response):
        return self._get_directive(response, 'StartMultiroomDirective')

    def _is_guid_string(self, s):
        # xxxx-xxxx-xxxx-xxxx
        return re.match(r'^\w+-\w+-\w+-\w+$', s) is not None

    # asserts
    def _assert_general_query_in_session_out_of_group(self, r):
        assert not self._get_push_directive(r)
        assert not self._get_start_multiroom_directive(r)

        # check 'audio_play' WITHOUT the token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert not ap_directive.MultiroomToken

        # check scenario state WITHOUT the token
        state = self._get_scenario_state(r)
        assert not state.MultiroomToken


class TestsMultiroomToken(_TestsMultiroomTokenBase):

    @pytest.mark.iot_user_info(SIMPLE_IOT_USER_INFO)
    @pytest.mark.device_state(SINGLE_PLAYING_DEVICE_STATE)
    def test_play_everywhere_from_single(self, alice):
        r = alice(voice('включи музыку везде'))
        assert not self._get_push_directive(r)

        # check 'start_multiroom' with NEW token
        mr_directive = self._get_start_multiroom_directive(r)
        assert mr_directive
        assert mr_directive.LocationInfo.Everywhere
        assert mr_directive.MultiroomToken != DEFAULT_MULTIROOM_TOKEN
        assert self._is_guid_string(mr_directive.MultiroomToken)

        # check 'audio_play' with the same NEW token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert ap_directive.MultiroomToken == mr_directive.MultiroomToken

        # check scenario state with the same NEW token
        state = self._get_scenario_state(r)
        assert state.MultiroomToken == mr_directive.MultiroomToken

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_common_query_in_session_master(self, alice):
        '''
        Колонки находятся в MR-сессии. Обычные муз. запросы обработаются в текущей MR-сессии.
        Новая сессия не создается.
        '''
        r = alice(voice('включи rammstein'))
        assert not self._get_push_directive(r)
        assert not self._get_start_multiroom_directive(r)

        # check 'audio_play' with the OLD token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert ap_directive.MultiroomToken == DEFAULT_MULTIROOM_TOKEN

        # check scenario state with the OLD token
        state = self._get_scenario_state(r)
        assert state.MultiroomToken == DEFAULT_MULTIROOM_TOKEN

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_common_query_in_session_slave(self, alice):
        '''
        Колонки находятся в MR-сессии. Обычные муз. запросы на слейв переносятся в мастера.
        '''
        r = alice(voice('включи rammstein'))
        assert self._directives_count(r) == 1

        pd = self._get_push_directive(r)
        assert pd.DeviceId == 'mini_in_the_bedroom_2'  # MASTER DEVICE ID
        assert pd.Body.TypedSemanticFrame.HasField('MusicPlaySemanticFrame')

        return str(pd)  # для надежности

    def _assert_general_query_in_session_in_group(self, r):
        assert not self._get_push_directive(r)

        # check 'start_multiroom' with NEW token
        mr_directive = self._get_start_multiroom_directive(r)
        assert mr_directive
        assert mr_directive.LocationInfo.GroupsIds[0] == 'floor'
        assert mr_directive.MultiroomToken != DEFAULT_MULTIROOM_TOKEN

        # check 'audio_play' with the same NEW token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert ap_directive.MultiroomToken == mr_directive.MultiroomToken

        # check scenario state with the same NEW token
        state = self._get_scenario_state(r)
        assert state.MultiroomToken == mr_directive.MultiroomToken

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_general_query_in_session_master_in_group(self, alice):
        '''
        Задаем "абстрактный" запрос "включи музыку", это переключает
        мультирум на одиночное произведение
        !!!НО!!! Так как колонка находится в группе, то музыка включается на всей группе!
        '''
        r = alice(voice('включи музыку'))
        self._assert_general_query_in_session_in_group(r)

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_general_query_in_session_slave_in_group(self, alice):
        '''
        Задаем "абстрактный" запрос "включи музыку", это переключает
        мультирум на одиночное произведение
        !!!НО!!! Так как колонка находится в группе, то музыка включается на всей группе!

        Со слейва не переносим запрос на мастер
        '''
        r = alice(voice('включи музыку'))
        self._assert_general_query_in_session_in_group(r)

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER_OUT_OF_GROUP)
    def test_general_query_in_session_master_out_of_group(self, alice):
        '''
        Задаем "абстрактный" запрос "включи музыку", это переключает
        мультирум на одиночное произведение
        Так как колонка НЕ находится в группе, то музыка включается только на данной колонке, БЕЗ MultiroomToken
        '''
        r = alice(voice('включи музыку'))
        self._assert_general_query_in_session_out_of_group(r)

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE_OUT_OF_GROUP)
    def test_general_query_in_session_slave_out_of_group(self, alice):
        '''
        Задаем "абстрактный" запрос "включи музыку", это переключает
        мультирум на одиночное произведение
        Так как колонка НЕ находится в группе, то музыка включается только на данной колонке, БЕЗ MultiroomToken
        '''
        r = alice(voice('включи музыку'))
        self._assert_general_query_in_session_out_of_group(r)

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_new_location_intersecting_master_in_location(self, alice):
        '''
        мастер = station_in_the_bedroom_1
        слейв = mini_in_the_bedroom_2
        активатор = мастер
        просим включить в спальне (мастер туда принадлежит)
        '''
        r = alice(voice('включи музыку в спальне'))
        assert not self._get_push_directive(r)

        # check 'start_multiroom' with NEW token
        mr_directive = self._get_start_multiroom_directive(r)
        assert mr_directive
        assert mr_directive.LocationInfo.RoomsIds[0] == 'bedroom'
        assert mr_directive.MultiroomToken != DEFAULT_MULTIROOM_TOKEN

        # check 'audio_play' with the same NEW token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert ap_directive.MultiroomToken == mr_directive.MultiroomToken

        # check scenario state with the same NEW token
        state = self._get_scenario_state(r)
        assert state.MultiroomToken == mr_directive.MultiroomToken

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_new_location_intersecting_master_not_in_location(self, alice):
        '''
        мастер = station_in_the_bedroom_1
        слейв = mini_in_the_bedroom_2
        активатор = мастер
        просим включить на миниках (мастер туда не принадлежит, но есть пересечение)
        '''
        r = alice(voice('включи музыку на миниках'))
        assert r.scenario_stages() == {'run'}
        assert self._directives_count(r) == 2

        # directive for stopping playing music
        assert self._get_directive(r, 'ClearQueueDirective')

        # directive for playing music on another device
        pd = self._get_push_directive(r)
        assert pd.DeviceId in [
            'mini_in_the_kitchen_1',
            'mini_in_the_bedroom_1',
            'mini_in_the_bedroom_2',
        ]
        assert pd.Body.TypedSemanticFrame.HasField('MusicPlaySemanticFrame')

        return str(pd)  # для надежности

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_new_location_intersecting_slave_in_location(self, alice):
        '''
        мастер = mini_in_the_bedroom_2
        слейв = station_in_the_bedroom_1
        активатор = слейв
        просим включить в спальне (слейв туда принадлежит)
        '''
        r = alice(voice('включи музыку в спальне'))
        assert not self._get_push_directive(r)

        # check 'start_multiroom' with NEW token
        mr_directive = self._get_start_multiroom_directive(r)
        assert mr_directive
        assert mr_directive.LocationInfo.RoomsIds[0] == 'bedroom'
        assert mr_directive.MultiroomToken != DEFAULT_MULTIROOM_TOKEN

        # check 'audio_play' with the same NEW token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert ap_directive.MultiroomToken == mr_directive.MultiroomToken

        # check scenario state with the same NEW token
        state = self._get_scenario_state(r)
        assert state.MultiroomToken == mr_directive.MultiroomToken

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_new_location_intersecting_slave_not_in_location(self, alice):
        '''
        мастер = mini_in_the_bedroom_2
        слейв = station_in_the_bedroom_1
        активатор = слейв
        просим включить на миниках (слейв туда не принадлежит, но есть пересечение)
        '''
        r = alice(voice('включи музыку на миниках'))
        assert r.scenario_stages() == {'run'}
        assert self._directives_count(r) == 2

        # directive for stopping playing music
        assert self._get_directive(r, 'ClearQueueDirective')

        # directive for playing music on another device
        pd = self._get_push_directive(r)
        assert pd.DeviceId in [
            'mini_in_the_kitchen_1',
            'mini_in_the_bedroom_1',
            'mini_in_the_bedroom_2',
        ]
        assert pd.Body.TypedSemanticFrame.HasField('MusicPlaySemanticFrame')

        return str(pd)  # для надежности

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_new_location_non_intersecting_master(self, alice):
        '''
        мастер = station_in_the_bedroom_1
        слейв = mini_in_the_bedroom_2
        активатор = мастер
        просим включить на кухне (мастер туда не принадлежит, и пересечения нет)
        '''
        r = alice(voice('включи музыку на кухне'))
        assert r.scenario_stages() == {'run'}
        assert self._directives_count(r) == 1

        # directive for playing music on another device
        pd = self._get_push_directive(r)
        assert pd.DeviceId in [
            'station_in_the_kitchen_1',
            'station_in_the_kitchen_2',
            'mini_in_the_kitchen_1',
        ]
        assert pd.Body.TypedSemanticFrame.HasField('MusicPlaySemanticFrame')

        return str(pd)  # для надежности

    @pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_new_location_non_intersecting_slave(self, alice):
        '''
        мастер = mini_in_the_bedroom_2
        слейв = station_in_the_bedroom_1
        активатор = слейв
        просим включить на кухне (слейв туда не принадлежит, и пересечения нет)
        '''
        r = alice(voice('включи музыку на кухне'))
        assert r.scenario_stages() == {'run'}
        assert self._directives_count(r) == 1

        # directive for playing music on another device
        pd = self._get_push_directive(r)
        assert pd.DeviceId in [
            'station_in_the_kitchen_1',
            'station_in_the_kitchen_2',
            'mini_in_the_kitchen_1',
        ]
        assert pd.Body.TypedSemanticFrame.HasField('MusicPlaySemanticFrame')

        return str(pd)  # для надежности


@pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
class TestMultiroomTokenCommands(_TestsMultiroomTokenBase):

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_stop_playing_master(self, alice):
        r = alice(voice('хватит', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}
        assert self._directives_count(r) == 1
        assert self._get_directive(r, 'ClearQueueDirective')

        # Мы здесь не проверяем MultiroomToken, потому что он принадлежит
        # scenario state сценарию МУЗЫКИ, и он не мог поменяться

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_stop_playing_slave(self, alice):
        r = alice(voice('хватит', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}
        assert self._directives_count(r) == 1
        assert self._get_directive(r, 'ClearQueueDirective')

    @pytest.mark.device_state(LOCATION_NON_PLAYING_DEVICE_STATE_MASTER)
    def test_resume_playing_master(self, alice):
        r = alice(voice('продолжи'))
        assert self._directives_count(r) == 1

        # check 'audio_play' with the OLD token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert ap_directive.MultiroomToken == DEFAULT_MULTIROOM_TOKEN

    @pytest.mark.device_state(LOCATION_NON_PLAYING_DEVICE_STATE_SLAVE)
    def test_resume_playing_slave(self, alice):
        '''
        Команда "продолжи" должна эвакуироваться на мастер-колонку
        '''
        r = alice(voice('продолжи'))
        assert self._directives_count(r) == 1

        pd = self._get_push_directive(r)
        assert pd.DeviceId == 'mini_in_the_bedroom_2'  # MASTER DEVICE ID
        assert pd.Body.TypedSemanticFrame.HasField('PlayerContinueSemanticFrame')

        return str(pd)  # для надежности

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_next_track_master(self, alice):
        r = alice(voice('включи rammstein'))  # чтобы заполнить очередь в scenario state
        r = alice(voice('дальше'))

        assert not self._get_push_directive(r)
        assert not self._get_start_multiroom_directive(r)

        # check 'audio_play' with the OLD token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert ap_directive.MultiroomToken == DEFAULT_MULTIROOM_TOKEN

        # check scenario state with the OLD token
        state = self._get_scenario_state(r)
        assert state.MultiroomToken == DEFAULT_MULTIROOM_TOKEN

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_next_track_slave(self, alice):
        r = alice(voice('дальше'))
        assert self._directives_count(r) == 1

        pd = self._get_push_directive(r)
        assert pd.DeviceId == 'mini_in_the_bedroom_2'  # MASTER DEVICE ID
        assert pd.Body.TypedSemanticFrame.HasField('PlayerNextTrackSemanticFrame')

        return str(pd)  # для надежности

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER_OUT_OF_GROUP)
    def test_clear_multiroom_token(self, alice):
        '''
        Кейс на сложное взаимодействие:
        0. Активна мультирумная сессия - DEFAULT_MULTIROOM_TOKEN
        1. Просим включить исполнителя - DEFAULT_MULTIROOM_TOKEN
        2. Просим "Стоп" - <...>  (сценарий Commands)
        3. Просим "включи музыку" - NO TOKEN (играет одна колонка)
        4. Отправляем колбек get_next - NO TOKEN (играет одна колонка)
        '''
        # --------- 1. Просим включить исполнителя
        r = alice(voice('включи rammstein'))
        assert not self._get_push_directive(r)
        assert not self._get_start_multiroom_directive(r)

        # check 'audio_play' with the OLD token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert ap_directive.MultiroomToken == DEFAULT_MULTIROOM_TOKEN

        # check scenario state with the OLD token
        state = self._get_scenario_state(r)
        assert state.MultiroomToken == DEFAULT_MULTIROOM_TOKEN

        # --------- 2. Просим "Стоп"
        r = alice(voice('хватит', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}
        assert self._directives_count(r) == 1
        assert self._get_directive(r, 'ClearQueueDirective')

        # --------- 3. Просим "включи музыку"
        r = alice(voice('включи музыку'))
        self._assert_general_query_in_session_out_of_group(r)

        # --------- 4. Отправляем колбек get_next
        reset_add = self._get_response_body(r).StackEngine.Actions[1].ResetAdd
        callback_next = get_callback_from_reset_add_effect(reset_add, callback_name='music_thin_client_next')
        r = alice(callback(name=callback_next['name'], payload=callback_next['payload']))

        assert not self._get_push_directive(r)
        assert not self._get_start_multiroom_directive(r)

        # check 'audio_play' WITHOUT the token
        ap_directive = self._get_audio_play_directive(r)
        assert ap_directive
        assert not ap_directive.MultiroomToken

        # check scenario state WITHOUT the token
        state = self._get_scenario_state(r)
        assert not state.MultiroomToken

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_player_command_smoke(self, alice):
        '''
        Смоук-тест на то, что команды со слейва переносятся на мастер
        '''
        r = alice(voice('перемотай на 10 секунд'))
        assert self._directives_count(r) == 1

        pd = self._get_push_directive(r)
        assert pd.DeviceId == 'mini_in_the_bedroom_2'  # MASTER DEVICE ID
        assert pd.Body.TypedSemanticFrame.HasField('PlayerRewindSemanticFrame')

        return str(pd)  # для надежности


@pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
class TestStartMultiroom(_TestsMultiroomTokenBase):
    '''
    Кейсы про "продолжи в <название локации>"
    '''

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_non_visible_peer(self, alice):
        '''
        Запрос "продолжи в оранжерее", но нет ни одного visible peer в комнате "оранжерея"
        Поэтому ответит TTS о том, что не знает, что это за место
        '''
        r = alice(voice('включи rammstein'))
        r = alice(voice('продолжи в оранжерее'))
        assert r.scenario_stages() == {'run'}
        assert not self._directives(r)
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Извините, не знаю где это место.'
        assert r.run_response.Features.PlayerFeatures.RestorePlayer

    @pytest.mark.device_state(LOCATION_NON_PLAYING_DEVICE_STATE_MASTER)
    def test_non_playing_device(self, alice):
        '''
        Запрос "продолжи на кухне", но текущая колонка не играет.
        Поэтому ответит TTS о том, что сейчас ничего не играет
        '''
        r = alice(voice('продолжи на кухне'))
        assert r.scenario_stages() == {'run'}
        assert not self._directives(r)
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'Не могу, ведь сейчас ничего не играет.',
            'Не получится. Сейчас ничего не играет.',
        ]
        assert r.run_response.Features.PlayerFeatures.RestorePlayer

    class TrackCase:
        COMMAND = 'включи песню ich will'
        TITLE = 'Ich Will'
        ASK_NEXT_TRACK = False

        def make_frame(self):
            frame = TMusicPlaySemanticFrame()
            frame.Location.UserIotRoomValue = 'bedroom'
            frame.ObjectType.EnumValue = TMusicPlayObjectTypeSlot.EValue.Track
            frame.ObjectId.StringValue = '43127'  # track id of "Ich Will"
            frame.OffsetSec.DoubleValue = 13.0  # start the track from the 13th second
            return frame

        def check(self, music_play):
            # check that music play has OffsetSec
            assert music_play.HasField('OffsetSec')

            frame = self.make_frame()
            frame.OffsetSec.DoubleValue = music_play.OffsetSec.DoubleValue
            assert music_play == frame

    class AlbumCase:
        COMMAND = 'включи альбом mutter'
        TITLE = 'Links 2 3 4'
        ASK_NEXT_TRACK = True

        def make_frame(self):
            frame = TMusicPlaySemanticFrame()
            frame.Location.UserIotRoomValue = 'bedroom'
            frame.ObjectType.EnumValue = TMusicPlayObjectTypeSlot.EValue.Album
            frame.ObjectId.StringValue = '3542'  # album id of "Mutter"
            frame.StartFromTrackId.StringValue = '43130'  # track id of "Links 2 3 4", the second track in album
            frame.OffsetSec.DoubleValue = 13.0  # start the track from the 13th second
            return frame

        def check(self, music_play):
            # check that music play has OffsetSec
            assert music_play.HasField('OffsetSec')

            frame = self.make_frame()
            frame.OffsetSec.DoubleValue = music_play.OffsetSec.DoubleValue
            assert music_play == frame

    class ArtistCase:
        COMMAND = 'включи rammstein'
        TITLE = 'Du Hast'
        ASK_NEXT_TRACK = True

        def make_frame(self):
            frame = TMusicPlaySemanticFrame()
            frame.Location.UserIotRoomValue = 'bedroom'
            frame.ObjectType.EnumValue = TMusicPlayObjectTypeSlot.EValue.Artist
            frame.ObjectId.StringValue = '13002'  # artist id of "Rammstein"
            frame.StartFromTrackId.StringValue = '22771'  # track id of "Du Hast", the second track of artist
            frame.OffsetSec.DoubleValue = 13.0  # start the track from the 13th second
            return frame

        def check(self, music_play):
            # check that music play has any StartFromTrackId
            assert music_play.HasField('StartFromTrackId')

            # check that music play has OffsetSec
            assert music_play.HasField('OffsetSec')

            frame = self.make_frame()
            frame.StartFromTrackId.StringValue = music_play.StartFromTrackId.StringValue
            frame.OffsetSec.DoubleValue = music_play.OffsetSec.DoubleValue
            assert music_play == frame

    class PlaylistCase:
        # https://music.yandex.ru/users/robot-alice-hw-tests-plus/playlists/1000
        COMMAND = 'включи плейлист хрючень брудень и элекок'
        TITLE = 'Tomorrow Comes Today'
        ASK_NEXT_TRACK = True

        def make_frame(self):
            frame = TMusicPlaySemanticFrame()
            frame.Location.UserIotRoomValue = 'bedroom'
            frame.ObjectType.EnumValue = TMusicPlayObjectTypeSlot.EValue.Playlist
            frame.ObjectId.StringValue = '1035351314:1000'
            frame.StartFromTrackId.StringValue = '311756'  # track id of "Tomorrow Comes Today", the second track of playlist
            frame.OffsetSec.DoubleValue = 13.0  # start the track from the 13th second
            return frame

        def check(self, music_play):
            # check that music play has OffsetSec
            assert music_play.HasField('OffsetSec')

            frame = self.make_frame()
            frame.OffsetSec.DoubleValue = music_play.OffsetSec.DoubleValue
            assert music_play == frame

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    @pytest.mark.parametrize('case_class', [
        pytest.param(TrackCase, id='track'),
        pytest.param(AlbumCase, id='album'),
        pytest.param(ArtistCase, id='artist'),
        pytest.param(PlaylistCase, id='playlist'),
    ])
    def test_in_location_begin(self, alice, case_class):
        '''
        Запрос "продолжи в спальне", работает нормально.
        На данной колонке музыка стопнется, в спальню пойдет фрейм.
        '''
        c = case_class()

        r = alice(voice(c.COMMAND))
        if c.ASK_NEXT_TRACK:
            r = alice(voice('следующий трек'))

        alice.skip(seconds=13)  # play the track for 13 seconds

        r = alice(voice('продолжи в спальне'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert self._directives_count(r) == 2

        # directive for stopping playing music
        assert self._get_directive(r, 'ClearQueueDirective')

        # directive for playing music on another device
        pd = self._get_push_directive(r)
        assert pd.DeviceId in [
            'station_in_the_bedroom_1',
            'mini_in_the_bedroom_1',
            'mini_in_the_bedroom_2',
        ]
        assert pd.Body.TypedSemanticFrame.HasField('MusicPlaySemanticFrame')
        c.check(pd.Body.TypedSemanticFrame.MusicPlaySemanticFrame)

        return str(pd)  # для надежности

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    @pytest.mark.parametrize('case_class', [
        pytest.param(TrackCase, id='track'),
        pytest.param(AlbumCase, id='album'),
        pytest.param(ArtistCase, id='artist'),
        pytest.param(PlaylistCase, id='playlist'),
    ])
    def test_in_location_end(self, alice, case_class):
        '''
        Проверяем, что отосланные фреймы правильно работают
        '''
        c = case_class()
        frame = c.make_frame()

        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': proto_to_dict(frame),
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        metadata = r.continue_response.ResponseBody.Layout.Directives[1].AudioPlayDirective.AudioPlayMetadata
        assert metadata.Title == c.TITLE

        ap = self._get_audio_play_directive(r)
        assert ap.Stream.OffsetMs == 13_000

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_slave_in_location_begin(self, alice):
        '''
        Запрос "продолжи в спальне" на слейве, надо отправить запрос в мастер
        '''
        r = alice(voice('продолжи в спальне'))
        assert r.run_response.Features.PlayerFeatures.RestorePlayer

        # directive for playing music on master device
        assert self._directives_count(r) == 1
        pd = self._get_push_directive(r)
        pd.DeviceId == 'mini_in_the_bedroom_2'
        assert pd.Body.TypedSemanticFrame.HasField('StartMultiroomSemanticFrame')

        return str(pd)  # для надежности


@pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
class TestStopMultiroom(_TestsMultiroomTokenBase):
    '''
    Кейсы про "выключи музыку в <название локации>"
    '''

    @pytest.mark.skip(reason='wait for Megamind and Apphost releases')
    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_non_visible_peer(self, alice):
        '''
        Запрос "выключи музыку в оранжерее", но нет ни одного visible peer в комнате "оранжерея"
        Поэтому ответит TTS о том, что не знает, что это за место
        '''
        r = alice(voice('выключи музыку в оранжерее', scenario=Scenario('Commands', 'fast_command')))
        assert not self._directives(r)
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Извините, не знаю где это место.'

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_same_location_on_master(self, alice):
        '''
        Запрос "выключи музыку в спальне" на мастере, колонка в спальне
        '''
        r = alice(voice('выключи музыку в спальне', scenario=Scenario('Commands', 'fast_command')))
        assert self._directives_count(r) == 1
        pd = self._get_pause_directive(r)
        assert pd.RoomId == 'bedroom'
        assert pd.LocationInfo.RoomsIds == ['bedroom']

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_same_location_on_slave(self, alice):
        '''
        Запрос "выключи музыку в спальне" на слейве, колонка в спальне
        '''
        r = alice(voice('выключи музыку в спальне', scenario=Scenario('Commands', 'fast_command')))
        assert self._directives_count(r) == 1
        pd = self._get_pause_directive(r)
        assert pd.RoomId == 'bedroom'
        assert pd.LocationInfo.RoomsIds == ['bedroom']

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_another_location_on_master(self, alice):
        '''
        Запрос "выключи музыку на кухне" на мастере, колонка НЕ в кухне
        '''
        r = alice(voice('выключи музыку на кухне', scenario=Scenario('Commands', 'fast_command')))
        assert self._directives_count(r) == 1
        pd = self._get_pause_directive(r)
        assert pd.RoomId == 'kitchen'
        assert pd.LocationInfo.RoomsIds == ['kitchen']

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_another_location_on_slave(self, alice):
        '''
        Запрос "выключи музыку на кухне" на слейве, колонка НЕ в кухне
        '''
        r = alice(voice('выключи музыку на кухне', scenario=Scenario('Commands', 'fast_command')))
        assert self._directives_count(r) == 1
        pd = self._get_pause_directive(r)
        assert pd.RoomId == 'kitchen'
        assert pd.LocationInfo.RoomsIds == ['kitchen']
