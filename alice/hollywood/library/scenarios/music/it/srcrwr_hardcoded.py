import pytest


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber, datasync_stubber):
    return {
        'MUSIC_HARDCODED_SCENARIO_PROXY': f'localhost:{bass_stubber.port}',
        'MUSIC_HARDCODED_SCENARIO_DATASYNC_PROXY': f'localhost:{datasync_stubber.port}',
        'MUSIC_HARDCODED_SCENARIO_COMMIT_PROXY': f'localhost:{datasync_stubber.port}',
    }
