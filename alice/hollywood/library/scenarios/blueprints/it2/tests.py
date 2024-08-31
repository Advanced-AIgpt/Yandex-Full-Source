import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.scenarios.blueprints.proto.blueprints_pb2 import TBlueprintsState # noqa


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['blueprints']


@pytest.mark.scenario(name='Blueprints', handle='blueprints')
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class Tests:

    #
    # Этот тест произносит кодовую фразу, после которой сценарий blueprints проверяет все скрипты на корректность
    # Тест будет доработан позднее
    #
    def test_generic_report(self, alice):
        r = alice(voice('Отчет по всем blueprints'))
        assert r.scenario_stages() == {'run'}
