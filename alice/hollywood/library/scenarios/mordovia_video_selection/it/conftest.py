# -*- coding: utf-8 -*-
import pytest

from alice.hollywood.library.scenarios.mordovia_video_selection.it.test_cases import TESTS_DATA_PATH
from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture
from alice.hollywood.library.python.testing.stubber.stubber_server import StubberEndpoint


player_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'frontend.vh.yandex.ru',
    443,
    [
        StubberEndpoint('/player/{vh_uuid}.json', ['GET']),
    ],
    scheme="https",
    stubs_subdir='player_stubs',
    hash_pattern=True,
)

ott_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'api-testing.ott.yandex.net',
    443,
    [
        StubberEndpoint('/content/{ott_uuid}/stream', ['GET']),
    ],
    scheme="https",
    stubs_subdir='ott_stubs',
    hash_pattern=True,
)


@pytest.fixture(scope="function")
def srcrwr_params(player_stubber, ott_stubber):
    return {
        'MORDOVIA_VIDEO_SELECTION_VH_PROXY': f'http://localhost:{player_stubber.port}',
        'MORDOVIA_OTT_PROXY': f'http://localhost:{ott_stubber.port}',
    }
