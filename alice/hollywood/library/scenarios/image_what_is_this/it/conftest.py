from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture, StubberEndpoint
import alice.hollywood.library.scenarios.image_what_is_this.it.test_cases as tests_data
from alice.hollywood.library.scenarios.image_what_is_this.proto.image_what_is_this_pb2 import TImageWhatIsThisState  # noqa: F401
import pytest


@pytest.fixture(scope="function")
def srcrwr_params(image_what_is_this_stubber, image_what_is_this_clothes_stubber, image_what_is_this_cbir_features_stubber):
    return {
        'IMAGE_WHAT_IS_THIS_PROXY': f'localhost:{image_what_is_this_stubber.port}',
        'IMAGE_WHAT_IS_THIS_CLOTHES_PROXY_0': f'localhost:{image_what_is_this_clothes_stubber.port}',
        'IMAGE_WHAT_IS_THIS_CLOTHES_PROXY_1': f'localhost:{image_what_is_this_clothes_stubber.port}',
        'IMAGE_WHAT_IS_THIS_CLOTHES_PROXY_2': f'localhost:{image_what_is_this_clothes_stubber.port}',
        'IMAGE_WHAT_IS_THIS_CLOTHES_PROXY_3': f'localhost:{image_what_is_this_clothes_stubber.port}',
        'IMAGE_WHAT_IS_THIS_CLOTHES_PROXY_4': f'localhost:{image_what_is_this_clothes_stubber.port}',
        'IMAGE_WHAT_IS_THIS_CBIR_FEATURES_PROXY': f'localhost:{image_what_is_this_cbir_features_stubber.port}',
    }


image_what_is_this_stubber = create_stubber_fixture(
    tests_data.TESTS_DATA_PATH,
    'yandex.ru',
    443,
    [
        StubberEndpoint('/images-apphost/alice', ['GET'])
    ],
    scheme='https',
    stubs_subdir='alice'
)


image_what_is_this_clothes_stubber = create_stubber_fixture(
    tests_data.TESTS_DATA_PATH,
    'yandex.ru',
    443,
    [
        StubberEndpoint('/images-apphost/detected-objects', ['GET'])
    ],
    scheme='https',
    stubs_subdir='detected-objects'
)

image_what_is_this_cbir_features_stubber = create_stubber_fixture(
    tests_data.TESTS_DATA_PATH,
    'yandex.ru',
    443,
    [
        StubberEndpoint('/images-apphost/cbir-features', ['GET'])
    ],
    scheme='https',
    stubs_subdir='cbir-features'
)
