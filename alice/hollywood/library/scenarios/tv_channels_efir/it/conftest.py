# -*- coding: utf-8 -*-
import pytest

from alice.hollywood.library.scenarios.tv_channels_efir.it.test_cases import TESTS_DATA_PATH
from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture
from alice.hollywood.library.python.testing.stubber.stubber_server import StubberEndpoint


video_quasar_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'yandex.ru',
    443,
    [
        StubberEndpoint('/video/quasar/channels', ['GET']),
    ],
    scheme="https",
    stubs_subdir='video_quasar_channels_stubs',
    hash_pattern=True,
)


@pytest.fixture(scope="function")
def srcrwr_params(video_quasar_stubber):
    return {
        'SHOW_TV_CHANNELS_GALLERY_VIDEO_QUASAR_PROXY': f'http://localhost:{video_quasar_stubber.port}',
    }
