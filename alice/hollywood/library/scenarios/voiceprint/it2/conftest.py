import pytest

from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture, \
    create_static_stubber_fixture


bass_stubber = create_localhost_bass_stubber_fixture()

passport_stubber = create_static_stubber_fixture(
    '/',
    'alice/hollywood/library/scenarios/voiceprint/it2/passport_data/register_kolonkish_success.json'
)


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ['voiceprint']


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber, passport_stubber):
    return {
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}',
        'BASS_SRCRWR_Passport': f'http://localhost:{passport_stubber.port}',
        # Begemot yappy-beta was used for generator
        # 'ALICE__BEGEMOT_WORKER_MEGAMIND': 'cp6upu3nliigj7tn.sas.yp-c.yandex.net:81',
    }
