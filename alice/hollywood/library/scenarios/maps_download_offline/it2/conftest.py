import pytest


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['maps_download_offline']
