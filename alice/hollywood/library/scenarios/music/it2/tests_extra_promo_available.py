import logging
import re

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_static_stubber_fixture, HttpResponseStub


logger = logging.getLogger(__name__)


EXPERIMENTS_PROMO = [
    'music_extra_promo_period',
    'music_check_plus_promo',
]

EXPERIMENTS_THIN = [
    'hw_music_thin_client',
]


billing_stubber = create_static_stubber_fixture(  # This overrides billing_stubber fixture from conftest.py
    '/billing/requestPlus',
    'alice/hollywood/library/scenarios/music/it2/billing_data/promo_available_extra_period.json'
)


@pytest.fixture(scope="function")  # This overrides srcrwr_params fixture from conftest.py
def srcrwr_params(bass_stubber, music_back_stubber, storage_mds_stubber, billing_stubber, apphost):
    return {
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}',
        'HOLLYWOOD_MUSIC_BACKEND_PROXY': f'localhost:{music_back_stubber.port}',
        'HOLLYWOOD_MUSIC_MDS_PROXY': f'localhost:{storage_mds_stubber.port}',
        'MUSIC_SCENARIO_BILLING_PROXY': f'localhost:{billing_stubber.port}',

        # We need this additional srcrwr because 'stop/pause' command lives in 'fast_command' (aka Commands) scenario
        'Commands': f'localhost:{apphost.http_adapter_port}',
    }


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
class TestsExtraPromoAvailable:

    output_speech = 'Чтобы слушать музыку, вам нужно оформить подписку Яндекс Плюс. Сейчас вам доступен' \
                    ' промо-период. Вы можете активировать его в приложении Яндекс.'
    output_speech_extra_promo = r'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс. Сейчас вам доступен промо-период.' \
                                 ' Если активируете его в приложении яндекса до 30 сентября( 2022 года)?, то получите месяц в подарок.'

    # thin client
    @pytest.mark.experiments(*EXPERIMENTS_THIN)
    @pytest.mark.parametrize('surface', [surface.loudspeaker])
    def test_mini_no_exp_flag_thin(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == self.output_speech
        assert len(body.ServerDirectives) == 1
        assert body.ServerDirectives[0].PushMessageDirective

    @pytest.mark.experiments(*EXPERIMENTS_PROMO, *EXPERIMENTS_THIN)
    @pytest.mark.parametrize('surface', [surface.loudspeaker])
    def test_mini_thin(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert re.match(self.output_speech_extra_promo, body.Layout.OutputSpeech)
        assert len(body.ServerDirectives) == 1
        assert body.ServerDirectives[0].PushMessageDirective

    @pytest.mark.experiments(*EXPERIMENTS_PROMO, *EXPERIMENTS_THIN)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_station_thin(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == self.output_speech
        assert len(body.ServerDirectives) == 1
        assert body.ServerDirectives[0].PushMessageDirective

    # thick client
    @pytest.mark.experiments('music_check_plus_promo')
    @pytest.mark.parametrize('surface', [surface.loudspeaker])
    @pytest.mark.freeze_stubs(bass_stubber={
        '/megamind/prepare': [
            HttpResponseStub(200, 'freeze_stubs/test_mini_no_exp_flag_think_bass_post_megamind_prepare.json'),
        ],
        '/megamind/apply': [
            HttpResponseStub(200, 'freeze_stubs/test_mini_no_exp_flag_think_bass_post_megamind_apply.json'),
        ],
    })
    def test_mini_no_exp_flag_thick(self, alice):
        r = alice(voice('включи queen'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == self.output_speech

    @pytest.mark.experiments(*EXPERIMENTS_PROMO)
    @pytest.mark.parametrize('surface', [surface.loudspeaker])
    @pytest.mark.freeze_stubs(bass_stubber={
        '/megamind/prepare': [
            HttpResponseStub(200, 'freeze_stubs/test_mini_thick_bass_post_megamind_prepare.json'),
        ],
        '/megamind/apply': [
            HttpResponseStub(200, 'freeze_stubs/test_mini_thick_bass_post_megamind_apply.json'),
        ],
    })
    def test_mini_thick(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert re.match(self.output_speech_extra_promo, body.Layout.OutputSpeech)

    @pytest.mark.experiments(*EXPERIMENTS_PROMO)
    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.freeze_stubs(bass_stubber={
        '/megamind/prepare': [
            HttpResponseStub(200, 'freeze_stubs/test_station_thick_bass_post_megamind_prepare.json'),
        ],
        '/megamind/apply': [
            HttpResponseStub(200, 'freeze_stubs/test_station_thick_bass_post_megamind_apply.json'),
        ],
    })
    def test_station_thick(self, alice):
        r = alice(voice('включи музыку'))
        assert r.scenario_stages() == {'run', 'continue'}
        body = r.continue_response.ResponseBody
        assert body.Layout.OutputSpeech == self.output_speech
