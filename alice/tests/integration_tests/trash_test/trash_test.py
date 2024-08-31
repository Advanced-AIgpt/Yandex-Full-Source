import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestTrashRequestIsOk(object):
    owners = ('zubchick',)

    def test_trash_ok(self, alice):
        response = alice('+')
        assert 'error' not in response.raw.get('meta', [])
