import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.xfail(reason='тест будет актуализирован после завершения работ с див рендером для центавра')
@pytest.mark.parametrize('surface', [
    surface.smart_display,
])
class TestCentaurSearchScenario(object):

    owners = ('d-dima', )

    def test_alice_centaur(self, alice):
        response = alice('Столица Аргентины')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid
        # TODO тест будет актуализирован после завершения работ с див рендером для центавра

        response = alice('В каком году родился Аркадий Волож')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid

        response = alice('Сколько калорий в апельсине')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid

        response = alice('Какого роста Эйнштейн')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid

        response = alice('Кто написал книгу В поисках утраченного времени')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid
