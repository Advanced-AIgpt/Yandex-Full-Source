import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice, server_action, Scenario
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture


logger = logging.getLogger(__name__)

bass_stubber = create_localhost_bass_stubber_fixture()


@pytest.fixture(scope='module')
def enabled_scenarios():
    # We need 'fast_command' because 'stop/pause' command lives there in (Commands scenario)
    return ['music', 'fast_command']


MULTIROOM_EXPS = [
    'hw_music_multiroom_redirect',
    'hw_music_thin_client',
    'hw_music_thin_client_generative',
    'hw_music_thin_client_multiroom',
]


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
    },
    'audio_player': {
        'last_play_timestamp': 1579488271000,
        'player_state': 2  # 'Playing'
    },
}


# the first device is the master, the second is a slave
COMMON_IOT_USER_INFO = '''
  {
    "devices": [
      {
        "group_ids": [
          "0cbc849b-4d29-4c13-844d-3968aa7475f3"
        ],
        "quasar_info": {
          "device_id": "feedface-e8a2-4439-b2e7-000000000001.yandexstation_2"
        }
      },
      {
        "group_ids": [
          "0cbc849b-4d29-4c13-844d-3968aa7475f3"
        ],
        "quasar_info": {
          "device_id": "feedface-e8a2-4439-b2e7-000000000002.unknown_platform"
        }
      }
    ]
  }
'''


COMMON_MASTER_DEVICE_STATE = {
    'device_id': 'feedface-e8a2-4439-b2e7-000000000001.yandexstation_2',
    'multiroom': {
        'mode': 1,  # 'Master'
        'master_device_id': 'feedface-e8a2-4439-b2e7-000000000001.yandexstation_2',
        'multiroom_session_id': 'blahblahblah',
    },
    'audio_player': {
        'last_play_timestamp': 1579488271000,
        'player_state': 2  # 'Playing'
    },
}


COMMON_SLAVE_DEVICE_STATE = {
    'device_id': 'feedface-e8a2-4439-b2e7-000000000002.unknown_platform',
    'multiroom': {
        'mode': 2,  # 'Slave'
        'master_device_id': 'feedface-e8a2-4439-b2e7-000000000001.yandexstation_2',
        'multiroom_session_id': 'blahblahblah',
    },
    'audio_player': {
        'last_play_timestamp': 1579488271000,
        'player_state': 2  # 'Playing'
    },
}


COMMON_NON_PLAYING_SLAVE_DEVICE_STATE = {
    'device_id': 'feedface-e8a2-4439-b2e7-000000000002.unknown_platform',
    'multiroom': {
        'mode': 2,  # 'Slave'
        'master_device_id': 'feedface-e8a2-4439-b2e7-000000000001.yandexstation_2',
        'multiroom_session_id': 'blahblahblah',
    },
    'audio_player': {
        'last_play_timestamp': 1579488271000,
        'player_state': 5  # 'Stopped'
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


LOCATION_DEVICE_STATE = {
    'device_id': 'station_in_the_bedroom_1',  # is excluded from 'visible_peers'
    'multiroom': {
        'visible_peers': [
            'station_in_the_kitchen_1',
            'station_in_the_kitchen_2',
            'mini_in_the_kitchen_1',
            'mini_in_the_bedroom_1',
            'mini_in_the_bedroom_2',
        ],
    },
}


def build_location_playing_device_state(is_slave):
    mode = 2 if is_slave else 1
    return {
        'device_id': 'station_in_the_bedroom_1',  # is excluded from 'visible_peers'
        'multiroom': {
            'visible_peers': [
                'station_in_the_kitchen_1',
                'station_in_the_kitchen_2',
                'mini_in_the_kitchen_1',
                'mini_in_the_bedroom_1',
                'mini_in_the_bedroom_2',
            ],
            'mode': mode,
            'master_device_id': 'mini_in_the_bedroom_2',
            'multiroom_session_id': 'blahblahblah',
        },
        'audio_player': {
            'last_play_timestamp': 1579488271000,
            'player_state': 2,  # 'Playing'
        },
    }


LOCATION_PLAYING_DEVICE_STATE_SLAVE = build_location_playing_device_state(is_slave=True)
LOCATION_PLAYING_DEVICE_STATE_MASTER = build_location_playing_device_state(is_slave=False)


@pytest.mark.experiments(*MULTIROOM_EXPS)
@pytest.mark.supported_features('multiroom', 'multiroom_cluster', 'multiroom_audio_client')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
class _TestsThinMultiroomBase:

    def _get_response_body(self, response):
        stages = response.scenario_stages()
        if 'apply' in stages:
            return response.apply_response.ResponseBody
        elif 'continue' in stages:
            return response.continue_response.ResponseBody
        elif 'commit' in stages:
            return response.run_response.CommitCandidate.ResponseBody
        else:
            return response.run_response.ResponseBody

    def _get_directive(self, response, directive_name):
        response_body = self._get_response_body(response)
        directives = response_body.Layout.Directives
        for d in directives:
            if d.HasField(directive_name):
                return getattr(d, directive_name)
        return None

    def _get_push_directive(self, response):
        response_body = self._get_response_body(response)
        for d in response_body.ServerDirectives:
            if d.HasField('PushTypedSemanticFrameDirective'):
                return d.PushTypedSemanticFrameDirective
        return None

    def _get_audio_play_directive(self, response):
        return self._get_directive(response, 'AudioPlayDirective')

    def _get_start_multiroom_directive(self, response):
        return self._get_directive(response, 'StartMultiroomDirective')

    def _get_stop_multiroom_directive(self, response):
        return self._get_directive(response, 'StopMultiroomDirective')


class TestsThinMultiroom(_TestsThinMultiroomBase):

    @pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
    @pytest.mark.device_state(device_id='feedface-e8a2-4439-b2e7-000000000001.yandexstation_2')
    def test_continue_in_group(self, alice):
        '''
        Колонка находится в группе, и не играла в MR ранее. "Обычный" запрос запустит MR в группе, где эта колонка.
        '''
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        assert self._get_start_multiroom_directive(r).LocationInfo.GroupsIds[0] == '0cbc849b-4d29-4c13-844d-3968aa7475f3'

    @pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
    @pytest.mark.device_state(COMMON_SLAVE_DEVICE_STATE)
    def test_common_query_in_multiroom_session(self, alice):
        '''
        Колонки находятся в MR-сессии. Обычные муз. запросы обработаются в текущей MR-сессии.
        (В случае слейва это обычный пуш на мастер)
        '''
        r = alice(voice('включи rammstein'))
        pd = self._get_push_directive(r)
        return str(pd)

    @pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
    @pytest.mark.device_state(COMMON_SLAVE_DEVICE_STATE)
    def test_alarm_query_in_multiroom_session(self, alice):
        '''
        Колонки находятся в MR-сессии. Запрос с будильника не должен редиректиться со слейва на мастер.
        То, что запрос от будильника, понимаем по наличию поля alarm_id.
        '''
        object_id = '38646012'
        alarm_id = 'sample_alarm_id'
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'object_id': {
                        'string_value': object_id,
                    },
                    'object_type': {
                        'enum_value': 'Track',
                    },
                    'play_single_track': {
                        'bool_value': True,
                    },
                    'disable_autoflow': {
                        'bool_value': True,
                    },
                    'disable_nlg': {
                        'bool_value': True,
                    },
                    'alarm_id': {
                        'string_value': alarm_id,
                    }
                }
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music',
            },
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert not self._get_push_directive(r)

    @pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
    @pytest.mark.device_state(COMMON_SLAVE_DEVICE_STATE)
    def test_onyourwave_query_in_multiroom_session(self, alice):
        '''
        Колонки находятся в группе и в MR-сессии. Запрос а-ля "включи музыку" (user:onyourwave) особый и стопает сессию.
        (В случае слейва пуша нету, MR-сессия разрушается)
        Если колонка в группе, то создаем новую MR-сессию
        '''
        r = alice(voice('включи музыку'))
        assert not self._get_push_directive(r)
        assert not self._get_stop_multiroom_directive(r)
        assert self._get_start_multiroom_directive(r).LocationInfo.GroupsIds[0] == '0cbc849b-4d29-4c13-844d-3968aa7475f3'
        assert self._get_audio_play_directive(r)

    @pytest.mark.iot_user_info(SIMPLE_IOT_USER_INFO)
    @pytest.mark.device_state(SINGLE_PLAYING_DEVICE_STATE)
    def test_play_everywhere_from_single(self, alice):
        r = alice(voice('включи музыку везде'))
        assert not self._get_push_directive(r)
        assert self._get_start_multiroom_directive(r).LocationInfo.Everywhere
        assert self._get_audio_play_directive(r)


@pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
class TestThinMultiroomLocations(_TestsThinMultiroomBase):

    @pytest.mark.device_state(LOCATION_DEVICE_STATE)
    @pytest.mark.parametrize('location, device_ids', [
        pytest.param(
            'комнате кухня',
            {'station_in_the_kitchen_1', 'station_in_the_kitchen_2'},
            id='room_kitchen',
        ),
        pytest.param(
            'группе миники',
            {'mini_in_the_kitchen_1', 'mini_in_the_bedroom_1', 'mini_in_the_bedroom_2'},
            id='group_minis',
        ),
    ])
    def test_play_music_from_another_location(self, alice, location, device_ids):
        r = alice(voice('Включи rammstein в {}'.format(location)))

        assert not self._get_audio_play_directive(r)
        assert not self._get_start_multiroom_directive(r)
        assert not self._get_stop_multiroom_directive(r)

        # make check for device_id (this is hard to see in canonization)
        pd = self._get_push_directive(r)
        assert pd.DeviceId in device_ids

        return str(pd)

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_play_music_another_location_stop_current_multiroom(self, alice):
        '''
        В случае если мы просим включить музыку в другой локации, в текущей локации
        мультирумную сессию нужно остановить
        '''
        r = alice(voice('Включи rammstein в комнате кухня'))
        assert not self._get_start_multiroom_directive(r)
        assert not self._get_audio_play_directive(r)
        assert self._get_stop_multiroom_directive(r).MultiroomSessionId == 'blahblahblah'

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_play_music_same_location_stop_current_multiroom(self, alice):
        '''
        В случае если мы явно просим включить музыку В ЭТОЙ ЖЕ локации, нужно стартовать
        новую мультирумную сессию
        '''
        r = alice(voice('Включи rammstein в комнате спальня'))
        assert self._get_audio_play_directive(r)
        assert self._get_start_multiroom_directive(r).LocationInfo.RoomsIds[0] == 'bedroom'
        assert not self._get_stop_multiroom_directive(r)

    @pytest.mark.device_state(LOCATION_DEVICE_STATE)
    @pytest.mark.parametrize('location, location_id, attr', [
        pytest.param('комнате спальня', 'bedroom', 'RoomsIds', id='room_bedroom'),
        pytest.param('группе пол', 'floor', 'GroupsIds', id='group_floor'),
    ])
    def test_play_music_from_same_location(self, alice, location, location_id, attr):
        r = alice(voice('Включи rammstein в {}'.format(location)))
        assert not self._get_push_directive(r)
        assert getattr(self._get_start_multiroom_directive(r).LocationInfo, attr)[0] == location_id
        assert self._get_audio_play_directive(r)


@pytest.mark.device_state(COMMON_NON_PLAYING_SLAVE_DEVICE_STATE)
@pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
class TestThinMultiroomNonPlaying(_TestsThinMultiroomBase):

    def test_play_after_pause(self, alice):
        '''
        Если когда-то была активна MR сессия и играла музыка, но потом ее поставили на паузу,
        то музыкальный запрос разрушает MR-сессию
        '''
        r = alice(voice('Включи rammstein'))
        assert not self._get_push_directive(r)
        assert not self._get_start_multiroom_directive(r)
        assert not self._get_stop_multiroom_directive(r)
        assert self._get_audio_play_directive(r)

    def test_play_everywhere_after_pause(self, alice):
        '''
        Запрос на проигрывание в локации запускает новую сессию (неважно какой колонке это скажем)
        При этом stop_multiroom-а НЕТУ, старая MR-сессия закончится сама (?)
        '''
        r = alice(voice('Включи rammstein везде'))
        assert not self._get_push_directive(r)
        assert not self._get_stop_multiroom_directive(r)
        assert self._get_audio_play_directive(r)
        assert self._get_start_multiroom_directive(r).LocationInfo.Everywhere

    def test_continue(self, alice):
        '''
        "Продолжи" - единственная команда на слейве, которая эвакуируется на мастер.
        Все остальные команды выполнятся на слейве и почти наверняка ответят херово
        (т.к. очередь произведения - на мастере)
        '''
        r = alice(voice('продолжи'))
        pd = self._get_push_directive(r)
        return str(pd)


@pytest.mark.iot_user_info(LOCATIONS_IOT_USER_INFO)
class TestThinMultiroomCommands(_TestsThinMultiroomBase):

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_MASTER)
    def test_stop_playing_master(self, alice):
        r = alice(voice('хватит', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}
        assert self._get_directive(r, 'ClearQueueDirective')

    @pytest.mark.device_state(LOCATION_PLAYING_DEVICE_STATE_SLAVE)
    def test_stop_playing_slave(self, alice):
        r = alice(voice('хватит', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}
        assert self._get_directive(r, 'ClearQueueDirective')
