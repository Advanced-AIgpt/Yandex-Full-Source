import logging
import pytest

from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.hamcrest import non_empty_dict
from alice.hollywood.library.python.testing.it2.input import voice, server_action
from alice.hollywood.library.python.testing.it2.scenario_responses import Accumulator
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture
from hamcrest import assert_that, has_entries, contains, is_not


logger = logging.getLogger(__name__)

bass_stubber = create_localhost_bass_stubber_fixture()


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ['music']


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


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
class _TestsMultiroomBase:
    pass


START_PLAYLIST_OF_THE_DAY_DISABLE_MULTIROOM = server_action(name='@@mm_semantic_frame', payload={
    'typed_semantic_frame': {
        'music_play_semantic_frame': {
            'special_playlist': {
                'string_value': 'playlist_of_the_day',
            },
            'disable_multiroom': {
                'bool_value': True,
            },
        },
    },
    'analytics': {
        'origin': 'Scenario',
        'purpose': 'play_music',
    },
})


@pytest.mark.device_state(device_id='feedface-e8a2-4439-b2e7-000000000001.yandexstation_2')
@pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
class TestsMultiroom(_TestsMultiroomBase):

    def test_continue_in_group(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        assert r.continue_response.ResponseBody.Layout.Directives[0].HasField('StartMultiroomDirective')
        start_multiroom = r.continue_response.ResponseBody.Layout.Directives[0].StartMultiroomDirective
        assert start_multiroom.RoomId == 'all'

        assert r.continue_response.ResponseBody.Layout.Directives[1].HasField('MusicPlayDirective')

    def test_disable_multiroom(self, alice):
        r = alice(START_PLAYLIST_OF_THE_DAY_DISABLE_MULTIROOM)
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('MusicPlayDirective')

    @pytest.mark.experiments('hw_music_thin_client', 'hw_music_thin_client_playlist')
    def test_disable_multiroom_thin_client(self, alice):
        r = alice(START_PLAYLIST_OF_THE_DAY_DISABLE_MULTIROOM)
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('AudioPlayDirective')


class TestMultiroomLocations(_TestsMultiroomBase):

    IOT_USER_INFO = '''
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
                    "group_ids": [],
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

    def _check_response_is_multiroom_one(self, response, room_id):
        directives = response.continue_response.ResponseBody.Layout.Directives
        assert len(directives) == 2

        assert directives[0].HasField('StartMultiroomDirective')
        start_multiroom = directives[0].StartMultiroomDirective
        assert start_multiroom.RoomId == room_id
        assert directives[1].HasField('MusicPlayDirective')

    @pytest.mark.device_state(device_id='station_in_the_bedroom_1')
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    @pytest.mark.supported_features('multiroom', 'multiroom_cluster')
    @pytest.mark.parametrize('location,room_id', [
        pytest.param('кухня', 'kitchen', id='kitchen'),
        pytest.param('спальня', 'bedroom', id='bedroom'),
    ])
    @pytest.mark.parametrize('subject', [
        pytest.param('музыку', id='music'),
        pytest.param('beatles', id='artist')
    ])
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_play_music_in_room(self, alice, location, room_id, subject):
        r = alice(voice('Включи {} в комнате {}'.format(subject, location)))
        self._check_response_is_multiroom_one(r, room_id)

    @pytest.mark.device_state(device_id='station_in_the_bedroom_1')
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    @pytest.mark.supported_features('multiroom', 'multiroom_cluster')
    @pytest.mark.parametrize('subject', [
        pytest.param('музыку', id='music'),
        pytest.param('beatles', id='artist')
    ])
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_play_music_in_group(self, alice, subject):
        r = alice(voice('Включи {} на группе миники'.format(subject)))
        self._check_response_is_multiroom_one(r, 'minis')

    @pytest.mark.device_state(device_id='station_in_the_kitchen_1')
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    @pytest.mark.supported_features('multiroom', 'multiroom_cluster')
    @pytest.mark.parametrize('subject', [
        pytest.param('музыку', id='music'),
        pytest.param('beatles', id='artist')
    ])
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_play_music_at_location_started_from_other_group(self, alice, subject):
        r = alice(voice('включи {} на группе миники'.format(subject)))
        self._check_response_is_multiroom_one(r, 'minis')

        r = alice(voice('включи {} в комнате кухня'.format(subject)))
        self._check_response_is_multiroom_one(r, 'kitchen')


@pytest.mark.supported_features('multiroom', 'multiroom_cluster')
@pytest.mark.experiments('hw_music_thin_client', 'hw_music_thin_client_multiroom')
class TestsMultiroomDirectiveThinClient(_TestsMultiroomBase):

    @pytest.mark.skip
    @pytest.mark.device_state(device_id='feedface-e8a2-4439-b2e7-000000000001.yandexstation_2')
    @pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
    @pytest.mark.parametrize('query', [
        pytest.param('включи рамштайн', id='artist'),
        pytest.param('включи рок', id='radio'),
    ])
    def test_continue_in_group(self, alice, query):
        '''
        Обработка мультирумного запроса в тонком клиенте отличается от толстого (который внутри BASS)
        То, что сработал тонкий клиент, понимаем по наличию директивы audio_play
        '''
        r = alice(voice(query))
        assert r.scenario_stages() == {'run', 'continue'}

        assert_that(r.continue_response_pyobj, has_entries({
            'response': has_entries({
                'layout': has_entries({
                    'directives': contains({
                        'start_multiroom_directive': {
                            'room_id': 'all',
                        },
                    }, has_entries({
                        'audio_play_directive': non_empty_dict(),
                    })),
                }),
            }),
        }))


@pytest.mark.experiments('hw_music_multiroom_redirect', 'hw_music_thin_client_generative')
class _TestsRedirectMultiroomBase(_TestsMultiroomBase):
    IOT_USER_INFO = '''
      {
        "devices": [
          {
            "group_ids": [
              "komnata"
            ],
            "quasar_info": {
              "device_id": "slave_device_id"
            }
          },
          {
            "group_ids": [
              "komnata"
            ],
            "quasar_info": {
              "device_id": "master_device_id"
            }
          }
        ]
      }
    '''

    SLAVE_DEVICE_STATE = {
        'device_id': 'slave_device_id',
        'multiroom': {
            'mode': 2,  # 'Slave'
            'master_device_id': 'master_device_id',
            'multiroom_session_id': 'blahblahblah',
        },
        'audio_player': {
            'last_play_timestamp': 1579488271000,
            'player_state': 2  # 'Playing'
        },
    }

    MASTER_DEVICE_STATE = {
        'device_id': 'master_device_id',
        'multiroom': {
            'mode': 1,  # 'Master'
            'master_device_id': 'master_device_id',
            'multiroom_session_id': 'blahblahblah',
        },
        'audio_player': {
            'last_play_timestamp': 1579488271000,
            'player_state': 2  # 'Playing'
        },
    }

    OLD_PLAYER_RECENT_DEVICE_STATE = {
        'device_id': 'slave_device_id',
        'multiroom': {
            'mode': 2,  # 'Slave'
            'master_device_id': 'master_device_id',
            'multiroom_session_id': 'blahblahblah',
        },
        'audio_player': {
            'last_play_timestamp': 1579488271000,
        },
        'music': {
            'last_play_timestamp': 9999999999999,
        },
    }

    def _check_frame(self, response, frame_name, is_missing=False):
        check_obj = has_entries({
            'response_body': has_entries({
                'ServerDirectives': contains(has_entries({
                    'PushTypedSemanticFrameDirective': has_entries({
                        'device_id': 'master_device_id',
                        'ttl': 5,
                        'semantic_frame_request_data': has_entries({
                            'typed_semantic_frame': has_entries({
                                frame_name: has_entries({}),
                            }),
                            'analytics': has_entries({
                                'product_scenario': 'music',
                                'origin': 'Scenario',
                                'purpose': 'multiroom_redirect',
                            }),
                        }),
                    }),
                })),
            }),
        })
        if is_missing:
            check_obj = is_not(check_obj)
        assert_that(response.run_response_pyobj, check_obj)

    def _get_response_body(self, response):
        stages = response.scenario_stages()
        if 'apply' in stages:
            response_body = response.apply_response.ResponseBody
        elif 'continue' in stages:
            response_body = response.continue_response.ResponseBody
        elif 'commit' in stages:
            response_body = response.run_response.CommitCandidate.ResponseBody
        else:
            response_body = response.run_response.ResponseBody
        return response_body

    def _get_push_directive(self, response):
        for d in self._get_response_body(response).ServerDirectives:
            if d.HasField('PushTypedSemanticFrameDirective'):
                return d.PushTypedSemanticFrameDirective
        return None

    @pytest.mark.device_state(SLAVE_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    @pytest.mark.parametrize('query', [
        # pytest.param('включи музыку', id='autoplay'),
        pytest.param('включи du hast', id='track'),
        pytest.param('включи rammstein', id='artist'),
        pytest.param('включи классическую музыку', id='radio'),
    ])
    def test_redirect_simple(self, alice, query):
        r = alice(voice(query))
        assert r.scenario_stages() == {'run'}
        self._check_frame(r, 'music_play_semantic_frame')

    @pytest.mark.device_state(OLD_PLAYER_RECENT_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    def test_non_redirect_simple(self, alice):
        '''
        Если несмотря на все флаги, последним играла все-таки старая музыка,
        фреймы не нужно редиректить, отработают и так
        '''
        r = alice(voice('включи du hast'))
        assert r.scenario_stages() == {'run', 'continue'}
        self._check_frame(r, 'music_play_semantic_frame', is_missing=True)

    @pytest.mark.device_state(SLAVE_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    @pytest.mark.parametrize('query, frame_name', [
        pytest.param('следующее', 'player_next_track_semantic_frame', id='next_track'),
        pytest.param('предыдущее', 'player_prev_track_semantic_frame', id='prev_track'),
        pytest.param('продолжи', 'player_continue_semantic_frame', id='continue'),
        pytest.param('мне нравится', 'player_like_semantic_frame', id='like'),
        pytest.param('мне не нравится', 'player_dislike_semantic_frame', id='dislike'),
        pytest.param('перемешай', 'player_shuffle_semantic_frame', id='shuffle'),
        pytest.param('заново', 'player_replay_semantic_frame', id='replay'),
        pytest.param('перемотай на 5 часов 10 минут', 'player_rewind_semantic_frame', id='rewind'),
        pytest.param('повторяй это', 'player_repeat_semantic_frame', id='repeat'),
    ])
    def test_player_commands_redirect(self, alice, query, frame_name):
        '''
        Команды редиректятся на мастер
        '''
        r = alice(voice(query))
        assert r.scenario_stages() == {'run'}
        self._check_frame(r, frame_name)

        push_directive = self._get_push_directive(r)
        return str(push_directive)

    @pytest.mark.device_state(SLAVE_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    @pytest.mark.parametrize('query, frame_name', [
        pytest.param('что играет', 'player_what_is_playing_semantic_frame', id='what_is_playing'),
    ])
    def test_player_commands_non_redirect(self, alice, query, frame_name):
        '''
        Некоторые команды не должны редиректиться на мастер,
        потому что слейв может их выполнить
        '''
        r = alice(voice(query))
        self._check_frame(r, frame_name, is_missing=True)
        assert not self._get_push_directive(r)

    @pytest.mark.device_state(MASTER_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    def test_push_back_to_slave(self, alice):
        '''
        Если запрос в мастер-колонку пришел от слейва (что видно по origin), то голос и текст
        должна произнести слейв-колонка. Для этого посылается еще один пуш обратно в слейв.
        '''
        # enable track
        payload = {
            'typed_semantic_frame': {
                'music_play_semantic_frame': {
                    'action_request': {
                        'action_request_value': 'autoplay',
                    },
                    'genre': {
                        'genre_value': 'industrial',
                    },
                    'language': {
                        'language_value': 'german',
                    },
                },
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music',
            },
            'origin': {
                'device_id': 'slave_device_id',
                'uuid': 'slave_uuid',
            },
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        acc = Accumulator()
        acc.add(self._get_push_directive(r))

        # ask to replay
        payload['typed_semantic_frame'] = {
            'player_replay_semantic_frame': {
            },
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        acc.add(self._get_push_directive(r))

        # give a like
        payload['typed_semantic_frame'] = {
            'player_like_semantic_frame': {
            },
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        acc.add(self._get_push_directive(r))

        return str(acc)

    @pytest.mark.device_state(SLAVE_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    @pytest.mark.experiments('hw_music_multiroom_client_redirect')
    def test_all_push_directives_types(self, alice):
        '''
        Директив на редирект несколько - есть серверная и клиентская.
        Этот тест проверяет наличие всех директив при флагах:
        "hw_music_multiroom_redirect" - серверная директива (ServerDirective)
        "hw_music_multiroom_client_redirect" - клиентская директива (Directive)
        '''
        r = alice(voice('перемотай на 5 часов 10 минут'))
        assert r.scenario_stages() == {'run'}

        response_body = self._get_response_body(r)

        # check server directive
        dirs = response_body.ServerDirectives
        assert len(dirs) == 1
        assert dirs[0].HasField('PushTypedSemanticFrameDirective')

        # check client directive
        dirs = response_body.Layout.Directives
        assert len(dirs) == 1
        assert dirs[0].HasField('MultiroomSemanticFrameDirective')

        return str(r)


@pytest.mark.experiments('hw_music_thin_client', 'hw_music_thin_client_multiroom')
class TestsRedirectMultiroomThinClient(_TestsRedirectMultiroomBase):
    pass
