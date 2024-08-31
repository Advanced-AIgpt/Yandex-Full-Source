# -*- coding: utf-8 -*-
import pytest

from alice.hollywood.library.scenarios.video_rater.it.test_cases import TESTS_DATA_PATH
from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture, StubberEndpoint


datasync_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'api-mimino.dst.yandex.net',
    8080,
    [
        StubberEndpoint('/v1/personality/profile/alisa/kv/video_rater', ['GET', 'PUT'], idempotent=False),
    ],
    stubs_subdir='datasync_stubs',
)


entity_search_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'entitysearch.yandex.net',
    80,
    [
        StubberEndpoint('/get', ['GET']),
    ],
    stubs_subdir='entity_search_stubs',
)


@pytest.fixture(scope="function")
def srcrwr_params(entity_search_stubber, datasync_stubber):
    return {
        'VIDEO_RATER_ENTITY_SEARCH_PROXY': f'localhost:{entity_search_stubber.port}',
        'VIDEO_RATER_DATASYNC_PROXY': f'localhost:{datasync_stubber.port}',
        'VIDEO_RATER_COMMIT_PROXY': f'localhost:{datasync_stubber.port}',
    }
