import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


SUPPORTED_SURFACES = [surface.station, surface.station_pro]
UNSUPPORTED_SURFACES = [s for s in surface.actual_surfaces if s not in surface.smart_speakers]


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['equalizer']


@pytest.mark.scenario(name='Equalizer', handle='equalizer')
@pytest.mark.experiments('bg_fresh_granet', 'mm_enable_protocol_scenario=Equalizer')
class Tests:
    SURFACE_NOT_SUPPORTED_RESPONSE_TEXT = 'На этом устройстве нет эквалайзера.'
    GO_TO_SEARCH_APP = 'откройте приложение Яндекс.'
    CHOOSE_A_SPEAKER = 'Выберите устройство.'
    ENABLE_RESPONSE_TEXT = f'Чтобы включить эквалайзер, {GO_TO_SEARCH_APP}'
    DISABLE_RESPONSE_TEXT = f'Чтобы выключить эквалайзер, {GO_TO_SEARCH_APP}'
    WHICH_PRESET_IS_SET_RESPONSE_TEXT = f'Чтобы узнать, какой пресет установлен, {GO_TO_SEARCH_APP}'
    MORE_BASS_RESPONSE_TEXT = f'Чтобы настроить эквалайзер, {GO_TO_SEARCH_APP}'
    LESS_BASS_RESPONSE_TEXT = f'Чтобы настроить эквалайзер, {GO_TO_SEARCH_APP}'
    HOW_TO_SET_RESPONSE_TEXT = f'Чтобы настроить эквалайзер, {GO_TO_SEARCH_APP}'

    ALL_DEVICES_SCREEN_LINK = 'ya-search-app-open://?uri=yellowskin://?url=https%3A%2F%2Fyandex.ru%2Fquasar'

    PUSH_TITLE = 'Настройка звука'
    PUSH_TEXT = 'Нажмите, чтобы настроить звук колонки'
    PUSH_LINK_PREFIX = f'{ALL_DEVICES_SCREEN_LINK}%2Fexternal%2Fopen-equalizer%3FdeviceId%3D'

    IOT_USER_INFO = '''
      {
        "devices": [
          {
            "id": "device-id",
            "quasar_info": {
              "device_id": "quasar-device-id"
            }
          },
          {
            "id": "other-device-id",
            "quasar_info": {
              "device_id": "quasar-other-device-id"
            }
          }
        ]
      }
    '''

    SOURCE_DEVICE_QUASAR_DEVICE_ID = 'quasar-device-id'
    SOURCE_DEVICE_ID = 'device-id'
    NO_DEVICE_ID = ''

    ENV_STATE_WITH_EQUALIZER_CAPABILITY = {
        'endpoints': [{
            'id': SOURCE_DEVICE_QUASAR_DEVICE_ID,
            'capabilities': [
                {
                    '@type': 'type.googleapis.com/NAlice.TEqualizerCapability',
                },
            ],
        }]
    }

    TEST_CASES = [
        pytest.param('включи эквалайзер', ENABLE_RESPONSE_TEXT, id='equalizer-enable'),
        pytest.param('выключи эквалайзер', DISABLE_RESPONSE_TEXT, id='equalizer-disable'),
        pytest.param('какой пресет установлен в эквалайзере', WHICH_PRESET_IS_SET_RESPONSE_TEXT, id='equalizer-which-preset-is-set'),
        pytest.param('сделай больше басов', MORE_BASS_RESPONSE_TEXT, id='equalizer-add-more-bass'),
        pytest.param('сделай меньше басов', LESS_BASS_RESPONSE_TEXT, id='equalizer-less-bass'),
        pytest.param('как настроить звук на эквалайзере', HOW_TO_SET_RESPONSE_TEXT, id='equalizer-how-to-set'),
    ]

    SEARCHAPP_TEST_CASES = [
        pytest.param('включи эквалайзер', CHOOSE_A_SPEAKER, id='equalizer-enable'),
        pytest.param('выключи эквалайзер', CHOOSE_A_SPEAKER, id='equalizer-disable'),
        pytest.param('какой пресет установлен в эквалайзере', CHOOSE_A_SPEAKER, id='equalizer-which-preset-is-set'),
        pytest.param('сделай больше басов', CHOOSE_A_SPEAKER, id='equalizer-add-more-bass'),
        pytest.param('сделай меньше басов', CHOOSE_A_SPEAKER, id='equalizer-less-bass'),
        pytest.param('как настроить звук на эквалайзере', CHOOSE_A_SPEAKER, id='equalizer-how-to-set'),
    ]

    @staticmethod
    def _check_response_text(response_body, expected_text):
        assert len(response_body.Layout.Cards) == 1
        assert response_body.Layout.Cards[0].Text == expected_text

    def _check_has_push(self, response_body, device_id):
        assert len(response_body.ServerDirectives) == 1
        assert response_body.ServerDirectives[0].WhichOneof('Directive') == 'SendPushDirective'

        push_directive = response_body.ServerDirectives[0].SendPushDirective
        assert push_directive.Settings.Title == self.PUSH_TITLE
        assert push_directive.Settings.Text == self.PUSH_TEXT
        assert push_directive.Settings.Link == self.PUSH_LINK_PREFIX + device_id

    def _check_has_open_uri_directive(self, response_body):
        directives = response_body.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'OpenUriDirective'
        assert directives[0].OpenUriDirective.Uri == self.ALL_DEVICES_SCREEN_LINK

    def _check_is_irrelevant(self, response):
        assert response.run_response.Features.IsIrrelevant

        response_body = response.run_response.ResponseBody
        self._check_response_text(response_body, self.SURFACE_NOT_SUPPORTED_RESPONSE_TEXT)

    @pytest.mark.parametrize('command,expected_text', TEST_CASES)
    @pytest.mark.parametrize('surface', SUPPORTED_SURFACES)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    @pytest.mark.device_state(device_id=SOURCE_DEVICE_QUASAR_DEVICE_ID)
    def test_equalizer(self, alice, command, expected_text):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        self._check_response_text(response_body, expected_text)
        self._check_has_push(response_body, self.SOURCE_DEVICE_ID)

    @pytest.mark.parametrize('command,expected_text', TEST_CASES)
    @pytest.mark.parametrize('surface', SUPPORTED_SURFACES)
    @pytest.mark.device_state(device_id=SOURCE_DEVICE_QUASAR_DEVICE_ID)
    def test_equalizer_with_no_iot_user_info(self, alice, command, expected_text):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        self._check_response_text(response_body, expected_text)
        self._check_has_push(response_body, self.NO_DEVICE_ID)

    @pytest.mark.parametrize('command,expected_text', TEST_CASES)
    @pytest.mark.parametrize('surface', UNSUPPORTED_SURFACES)
    def test_equalizer_on_unsupported_surfaces(self, alice, command, expected_text):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        self._check_is_irrelevant(response)

    @pytest.mark.parametrize('command,expected_text', TEST_CASES)
    @pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station_lite_beige])
    @pytest.mark.device_state(device_id=SOURCE_DEVICE_QUASAR_DEVICE_ID)
    @pytest.mark.environment_state(ENV_STATE_WITH_EQUALIZER_CAPABILITY)
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    def test_equalizer_with_env_state(self, alice, command, expected_text):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        self._check_response_text(response_body, expected_text)
        self._check_has_push(response_body, self.SOURCE_DEVICE_ID)

    @pytest.mark.parametrize('command,expected_text', SEARCHAPP_TEST_CASES)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.iot_user_info(IOT_USER_INFO)
    def test_requests_from_search_app_and_non_empty_iot(self, alice, command, expected_text):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        self._check_response_text(response_body, expected_text)
        self._check_has_open_uri_directive(response_body)

    @pytest.mark.parametrize('command,expected_text', SEARCHAPP_TEST_CASES)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_requests_from_search_app_and_empty_iot(self, alice, command, expected_text):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        self._check_is_irrelevant(response)
