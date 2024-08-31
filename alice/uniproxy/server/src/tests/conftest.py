import pytest

def pytest_addoption(parser):
    parser.addoption("--qloud-env", action="store", default="testing",
        help="run tests using qloud environment ('stable' or 'testing' (default))")
    parser.addoption("--api-key", action="store",
        help="api key (by default get from config)")
    parser.addoption("--bio-group", action="store",
        help="biometry group")


@pytest.fixture
def qloud_env(request):
    return request.config.getoption("--qloud-env")


@pytest.fixture
def api_key(request):
    return request.config.getoption("--api-key")


@pytest.fixture
def bio_group(request):
    return request.config.getoption("--bio-group")
