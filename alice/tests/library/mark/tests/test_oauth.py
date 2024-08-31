import alice.tests.library.auth as auth
import alice.tests.library.mark as mark
import pytest


@pytest.fixture(scope='function')
def oauth(request):
    return mark.get_oauth_token(request)


class TestSingle(object):

    def test_none(self, oauth):
        assert oauth is None

    @pytest.mark.no_oauth
    def test_no_oauth(self, oauth):
        assert oauth is None

    @pytest.mark.oauth(auth.FAKE_FOR_CI_TEST)
    def test_oauth(self, oauth):
        assert oauth == auth.FAKE_FOR_CI_TEST


@pytest.mark.oauth(auth.FAKE_FOR_CI_TEST)
class TestNested(object):

    def test_none(self, oauth):
        assert oauth == auth.FAKE_FOR_CI_TEST

    @pytest.mark.no_oauth
    def test_no_oauth(self, oauth):
        assert oauth is None

    @pytest.mark.oauth(auth.FAKE_FOR_CI_TEST_2)
    def test_oauth(self, oauth):
        assert oauth == auth.FAKE_FOR_CI_TEST_2


@pytest.mark.no_oauth
class TestNestedNoOauth(object):

    def test_none(self, oauth):
        assert oauth is None

    @pytest.mark.no_oauth
    def test_no_oauth(self, oauth):
        assert oauth is None

    @pytest.mark.oauth(auth.FAKE_FOR_CI_TEST)
    def test_oauth(self, oauth):
        assert oauth == auth.FAKE_FOR_CI_TEST
