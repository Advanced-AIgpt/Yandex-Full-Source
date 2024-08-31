import pytest


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber):
    return {
        'MUSIC_SCENARIO_PROXY': f'localhost:{bass_stubber.port}',
        'MUSIC_SCENARIO_APPLY_PROXY': f'localhost:{bass_stubber.port}',
        'MUSIC_SCENARIO_CONTINUE_PROXY': f'localhost:{bass_stubber.port}',
    }
