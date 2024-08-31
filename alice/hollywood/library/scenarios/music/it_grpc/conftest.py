import pytest


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['music']
