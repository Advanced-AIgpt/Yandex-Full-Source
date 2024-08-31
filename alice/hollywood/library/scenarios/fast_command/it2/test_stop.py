import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth, ALICE_START_TIME
from alice.hollywood.library.python.testing.it2.input import voice, text, server_action
from hamcrest import assert_that, has_entries, contains, is_not, empty


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['fast_command']


@pytest.mark.scenario(name='Commands', handle='fast_command')
@pytest.mark.parametrize('surface', [surface.legatus])
class TestLegatusStop:

    def test_stop(self, alice):
        r = alice(voice('стоп'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Извините, такая команда не поддерживается этим плеером.'


@pytest.mark.scenario(name='Commands', handle='fast_command')
class TestCommandsStop:
    last_play_timestamp_5_seconds_ago = {'last_play_timestamp': (ALICE_START_TIME.timestamp() - 5) * 1000}  # Playback started 5 sec ago

    player_started_5_seconds_ago = {
        **last_play_timestamp_5_seconds_ago,
        'player': {
            'pause': False
        }
    }

    @staticmethod
    def _check_analytics_info(analytics_info):
        assert analytics_info.ProductScenarioName == 'stop'
        assert analytics_info.Intent == 'personal_assistant.scenarios.player_pause'

    @staticmethod
    def _check_player_pause_directive(directive, player_pause_name='player_pause'):
        assert directive.HasField('PlayerPauseDirective')
        player_pause = directive.PlayerPauseDirective
        assert player_pause.Name == player_pause_name

    @staticmethod
    def _check_clear_queue_directive(directive):
        assert directive.HasField('ClearQueueDirective')

    def _check_has_single_player_pause_directive(self, run_response):
        directives = run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        self._check_player_pause_directive(directives[0])
        self._check_analytics_info(run_response.ResponseBody.AnalyticsInfo)

    def _check_has_player_pause_and_clear_queue_directives(self, run_response, player_pause_name='player_pause'):
        directives = run_response.ResponseBody.Layout.Directives
        assert len(directives) == 2

        self._check_player_pause_directive(directives[0], player_pause_name=player_pause_name)
        self._check_clear_queue_directive(directives[1])
        self._check_analytics_info(run_response.ResponseBody.AnalyticsInfo)

    @staticmethod
    def _check_go_home_directive(run_response):
        directives = run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('GoHomeDirective')


class TestCommandsStopVideo(TestCommandsStop):

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.device_state(video=TestCommandsStop.player_started_5_seconds_ago)
    def test_stop_video(self, alice):
        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}
        self._check_has_single_player_pause_directive(r.run_response)

    @pytest.mark.parametrize('surface', [surface.smart_tv])
    @pytest.mark.device_state(video={
        **TestCommandsStop.player_started_5_seconds_ago,
        'player_capabilities': [
            'pause'
        ],
    })
    def test_stop_video_tv(self, alice):
        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}
        self._check_has_single_player_pause_directive(r.run_response)

    @pytest.mark.parametrize('surface', [surface.smart_tv])
    @pytest.mark.device_state(video=TestCommandsStop.player_started_5_seconds_ago)
    def test_stop_video_tv_unsupported(self, alice):
        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == 'Извините, такая команда не поддерживается этим плеером.'


class TestCommandsStopMusic(TestCommandsStop):

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.device_state(music=TestCommandsStop.player_started_5_seconds_ago)
    def test_stop_music(self, alice):
        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}

        self._check_has_single_player_pause_directive(r.run_response)


class TestCommandsStopRadio(TestCommandsStop):

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.device_state(radio=TestCommandsStop.player_started_5_seconds_ago)
    def test_stop_radio(self, alice):
        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}

        # TODO player_pause.Name should be player_pause
        self._check_has_player_pause_and_clear_queue_directives(run_response=r.run_response,
                                                                player_pause_name='general_conversation_player_pause')


class TestCommandsStopBluetooth(TestCommandsStop):

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.device_state(bluetooth=TestCommandsStop.player_started_5_seconds_ago)
    def test_stop_bluetooth(self, alice):
        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}

        # TODO player_pause.Name should be player_pause
        self._check_has_player_pause_and_clear_queue_directives(run_response=r.run_response,
                                                                player_pause_name='general_conversation_player_pause')


class TestCommandsStopAudioPlayer(TestCommandsStop):

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.device_state(audio_player={
        **TestCommandsStop.last_play_timestamp_5_seconds_ago,
        'player_state': 'Playing'
    })
    def test_stop_bluetooth(self, alice):
        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}

        # TODO player_pause.Name should be player_pause
        self._check_has_player_pause_and_clear_queue_directives(run_response=r.run_response,
                                                                player_pause_name='general_conversation_player_pause')

    @pytest.mark.parametrize('surface', [surface.webtouch])
    def test_stop_webtouch(self, alice):
        r = alice(voice('стоп'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech
        assert r.run_response.ResponseBody.Layout.Directives[0].ShowPromoDirective


@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestCommandsStopMusicAtLocation(TestCommandsStop):

    kitchen_room_id = 'kitchen_room_id'
    group_one_id = 'group_one_id'
    all_rooms_room_id = '__all__'

    COMMON_IOT_USER_INFO = f'''
    {{
        "devices": [
            {{
                "group_ids": [
                    "{group_one_id}"
                ],
                "room_id": "{kitchen_room_id}",
                "quasar_info": {{
                    "device_id": "station_at_kitchen_device_id"
                }}
            }}
        ],
        "rooms": [
            {{
                "name": "кухня",
                "id": "{kitchen_room_id}"
            }}
        ],
        "groups": [
            {{
                "name": "группа 1",
                "id": "{group_one_id}"
            }}
        ]
    }}
    '''

    def _check_player_pause_at_location_directive(self, directive, location_id):
        self._check_player_pause_directive(directive)
        assert directive.PlayerPauseDirective.RoomId == location_id

    @pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
    @pytest.mark.parametrize('command,location_id', [
        pytest.param('стоп на кухне', kitchen_room_id, id='kitchen'),
        pytest.param('пауза в группе 1', group_one_id, id='group-1'),
        pytest.param('выключи везде', all_rooms_room_id, id='everywhere'),
    ])
    def test_stop_music_at_location(self, alice, command, location_id):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        self._check_player_pause_at_location_directive(directives[0], location_id=location_id)
        self._check_analytics_info(analytics_info=r.run_response.ResponseBody.AnalyticsInfo)


class TestCommandsStopGoHome(TestCommandsStop):

    @pytest.mark.parametrize('surface', [surface.station])
    def test_go_home_as_pause(self, alice):
        r = alice(text('домой'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.ResponseBody.Layout.Cards
        self._check_has_player_pause_and_clear_queue_directives(r.run_response,
                                                                player_pause_name='general_conversation_player_pause')

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.supported_features('go_home')
    def test_go_home(self, alice):
        r = alice(text('домой'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.ResponseBody.Layout.Cards
        self._check_go_home_directive(r.run_response)


@pytest.mark.oauth(auth.YandexPlus)  # needed for Multiroom
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('commands_multiroom_redirect')
class TestCommandsStopFrameRedirect(TestCommandsStop):
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
    }

    MASTER_DEVICE_STATE = {
        'device_id': 'master_device_id',
        'multiroom': {
            'mode': 1,  # 'Master'
            'master_device_id': 'master_device_id',
            'multiroom_session_id': 'blahblahblah',
        },
    }

    def _check_frame_redirect(self, response, frame_name, is_missing=False):
        assert_obj = has_entries({
            'response_body': has_entries({
                'ServerDirectives': contains(has_entries({
                    'PushTypedSemanticFrameDirective': has_entries({
                        'puid': is_not(empty()),
                        'device_id': 'master_device_id',
                        'ttl': 5,
                        'semantic_frame_request_data': has_entries({
                            'typed_semantic_frame': has_entries({
                                frame_name: has_entries({}),
                            }),
                            'analytics': has_entries({
                                'product_scenario': 'stop',
                                'origin': 'Scenario',
                                'purpose': 'multiroom_redirect',
                            }),
                        }),
                    }),
                })),
            }),
        })
        if is_missing:
            assert_obj = is_not(assert_obj)
        assert_that(response.run_response_pyobj, assert_obj)

    @pytest.mark.skip(reason='теперь это неправильное поведение, см. https://wiki.yandex-team.ru/alice/scenarios/music/multiroom/thin/#vykljucheniemuzykivrabotajushhemmr')
    @pytest.mark.device_state(SLAVE_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    def test_redirect(self, alice):
        '''
        Запрос в слейв-колонку должен редиректить на мастер-колонку
        _и_ отправлять на слейв директиву тоже
        '''
        r = alice(voice('стоп'))
        self._check_frame_redirect(r, 'player_pause_semantic_frame')

        directives = r.run_response.ResponseBody.Layout.Directives
        pause_directive = next(d for d in directives if d.HasField('PlayerPauseDirective'))
        assert pause_directive

        return str(r)

    @pytest.mark.device_state(MASTER_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    def test_no_redirect(self, alice):
        '''
        Запрос в мастер-колонку не должен никуда редиректить
        '''
        r = alice(voice('стоп'))
        self._check_frame_redirect(r, 'player_pause_semantic_frame', is_missing=True)
        return str(r)

    @pytest.mark.device_state(MASTER_DEVICE_STATE)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    def test_frame_redirected(self, alice):
        '''
        Редирект от слейва к мастеру приходит в виде запроса семантик фреймом
        '''
        payload = {
            'typed_semantic_frame': {
                'player_pause_semantic_frame': {
                },
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'multiroom_redirect',
            },
            'origin': {
                'device_id': 'slave_device_id',
                'uuid': 'slave_uuid',
            },
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        return str(r)
