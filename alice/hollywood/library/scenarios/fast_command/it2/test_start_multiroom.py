import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
import alice.megamind.protos.common.device_state_pb2 as device_state


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['fast_command']


@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.scenario(name='Commands', handle='fast_command')
@pytest.mark.parametrize('surface', [
    surface.station,
])
@pytest.mark.device_state(audio_player={
    'player_state': device_state.TDeviceState.TAudioPlayer.TPlayerState.Playing
})
class TestStartMultiroom:
    KITCHEN_ROOM_ID = 'kitchen_room_id'
    KITCHEN_ROOM_NAME = 'кухня'
    IOT_USER_INFO = f'''
    {{
        "rooms": [
            {{
                "name": "{KITCHEN_ROOM_NAME}",
                "id": "{KITCHEN_ROOM_ID}"
            }}
        ],
        "devices": [
            {{
                "room_id": "{KITCHEN_ROOM_ID}",
                "quasar_info": {{
                    "device_id": "station_at_kitchen_device_id"
                }}
            }}
        ]
    }}
    '''

    @staticmethod
    def _check_multiroom_directive(directive):
        assert directive.HasField('StartMultiroomDirective')
        assert directive.StartMultiroomDirective.LocationInfo.IncludeCurrentDeviceId

    def test_start_everywhere(self, alice):
        r = alice(voice('включи эту музыку везде'))
        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        self._check_multiroom_directive(directives[0])
        assert directives[0].StartMultiroomDirective.LocationInfo.Everywhere

    @pytest.mark.iot_user_info(IOT_USER_INFO)
    def test_start_at_location(self, alice):
        r = alice(voice(f'включи это в комнате {self.KITCHEN_ROOM_NAME}'))
        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        self._check_multiroom_directive(directives[0])
        assert directives[0].StartMultiroomDirective.LocationInfo.RoomsIds == [self.KITCHEN_ROOM_ID]
