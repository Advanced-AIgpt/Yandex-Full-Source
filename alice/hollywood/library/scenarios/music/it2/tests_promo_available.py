import logging

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_static_stubber_fixture
from alice.hollywood.library.scenarios.music.proto.music_arguments_pb2 import TMusicArguments  # noqa
from alice.megamind.protos.common.app_type_pb2 import EAppType
from conftest import get_scenario_state


logger = logging.getLogger(__name__)

EXPERIMENTS_THIN = [
    'hw_music_thin_client',
]

EXPERIMENTS_THICK_PROMO = [
    'music_check_plus_promo',
    'test_music_skip_plus_promo_check'
]

billing_stubber = create_static_stubber_fixture(  # This overrides billing_stubber fixture from conftest.py
    '/billing/requestPlus',
    'alice/hollywood/library/scenarios/music/it2/billing_data/promo_available.json'
)


@pytest.fixture(scope='function')  # This overrides srcrwr_params fixture from conftest.py
def srcrwr_params(bass_stubber, music_back_stubber, storage_mds_stubber, billing_stubber, apphost):
    return {
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}',
        'HOLLYWOOD_MUSIC_BACKEND_PROXY': f'localhost:{music_back_stubber.port}',
        'HOLLYWOOD_MUSIC_MDS_PROXY': f'localhost:{storage_mds_stubber.port}',
        'MUSIC_SCENARIO_BILLING_PROXY': f'localhost:{billing_stubber.port}',

        # Scheme in url was specified intentionally: https://docs.yandex-team.ru/alice-scenarios/testing/srcrwr#internal-logic-bass-hollywood
        'BASS_SRCRWR_QuasarBillingPromoAvailability': f'http://localhost:{billing_stubber.port}',

        # We need this additional srcrwr because 'stop/pause' command lives in 'fast_command' (aka Commands) scenario
        'Commands': f'localhost:{apphost.http_adapter_port}',
    }


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class TestsPromoAvailable:

    output_speech = 'Чтобы слушать музыку, вам нужно оформить подписку Яндекс Плюс. Сейчас вам доступен' \
                    ' промо-период. Вы можете активировать его в приложении Яндекс.'

    @pytest.mark.experiments(*EXPERIMENTS_THIN)
    def test_play_artist(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == TestsPromoAvailable.output_speech
        assert len(body.Layout.Directives) == 0
        assert len(body.ServerDirectives) == 1
        push_directive = body.ServerDirectives[0].PushMessageDirective
        self._assert_push_message_directive_valid(push_directive,
                                                  activate_push_uri='http://nonexistentdomain.yandex.net/activate_promo_uri',
                                                  push_id='alice_quasar_promo_period_yandexplus',
                                                  push_tag='QUASAR.PROMO_PERIOD.yandexplus.1083813279')
        state = get_scenario_state(r.continue_response)
        assert not state.HasField('Queue')
        assert not state.BiometryUserId

    @pytest.mark.experiments(*EXPERIMENTS_THIN)
    def test_play_mood(self, alice):
        r = alice(voice('включи грустную музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == TestsPromoAvailable.output_speech
        assert len(body.Layout.Directives) == 0
        assert len(body.ServerDirectives) == 1
        push_directive = body.ServerDirectives[0].PushMessageDirective
        self._assert_push_message_directive_valid(push_directive,
                                                  activate_push_uri='http://nonexistentdomain.yandex.net/activate_promo_uri',
                                                  push_id='alice_quasar_promo_period_yandexplus',
                                                  push_tag='QUASAR.PROMO_PERIOD.yandexplus.1083813279')
        state = get_scenario_state(r.continue_response)
        assert not state.HasField('Queue')
        assert not state.BiometryUserId

    @pytest.mark.experiments(*EXPERIMENTS_THICK_PROMO, 'music_extra_promo_period')
    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_extra_promo_thick(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == TestsPromoAvailable.output_speech
        assert len(body.Layout.Directives) == 0
        assert len(body.ServerDirectives) == 0

    @pytest.mark.experiments(*EXPERIMENTS_THICK_PROMO)
    def test_extra_promo_thick_push_via_directive(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == TestsPromoAvailable.output_speech
        assert len(body.Layout.Directives) == 0
        assert len(body.ServerDirectives) == 1
        push_directive = body.ServerDirectives[0].SendPushDirective
        self._assert_send_push_directive_valid(push_directive,
                                               activate_push_uri='http://nonexistentdomain.yandex.net/activate_promo_uri',
                                               push_id='alice_quasar_bass_promo_period_yandexplus',
                                               push_tag='QUASAR_BASS.PROMO_PERIOD.yandexplus.1083813279')
        state = get_scenario_state(r.continue_response)
        assert not state.HasField('Queue')
        assert not state.BiometryUserId

    @pytest.mark.experiments(*EXPERIMENTS_THIN, 'music_extra_promo_period')
    def test_extra_promo_thin(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == TestsPromoAvailable.output_speech
        assert len(body.Layout.Directives) == 0
        assert len(body.ServerDirectives) == 1
        push_directive = body.ServerDirectives[0].PushMessageDirective
        self._assert_push_message_directive_valid(push_directive,
                                                  activate_push_uri='http://nonexistentdomain.yandex.net/activate_promo_uri',
                                                  push_id='alice_quasar_promo_period_yandexplus',
                                                  push_tag='QUASAR.PROMO_PERIOD.yandexplus.1083813279')
        state = get_scenario_state(r.continue_response)
        assert not state.HasField('Queue')
        assert not state.BiometryUserId

    def _assert_push_message_directive_valid(self, push_directive, activate_push_uri, push_id, push_tag):
        assert push_directive.Title == 'Яндекс.Плюс'
        assert push_directive.Body == 'Нажмите для активации Яндекс.Плюс'
        assert push_directive.Link == activate_push_uri
        assert push_directive.PushId == push_id
        assert push_directive.PushTag == push_tag
        assert push_directive.ThrottlePolicy == 'quasar_default_install_id'
        assert len(push_directive.AppTypes) == 1
        assert push_directive.AppTypes[0] == EAppType.AT_SEARCH_APP

    def _assert_send_push_directive_valid(self, push_directive, activate_push_uri, push_id, push_tag):
        assert push_directive.Settings.Title == 'Яндекс.Плюс'
        assert push_directive.Settings.Text == 'Нажмите для активации Яндекс.Плюс'
        assert push_directive.Settings.Link == activate_push_uri
        assert push_directive.PushId == push_id
        assert push_directive.PushTag == push_tag
        assert push_directive.PushMessage.ThrottlePolicy == 'quasar_default_install_id'
        assert len(push_directive.PushMessage.AppTypes) == 1
        assert push_directive.PushMessage.AppTypes[0] == EAppType.AT_SEARCH_APP
