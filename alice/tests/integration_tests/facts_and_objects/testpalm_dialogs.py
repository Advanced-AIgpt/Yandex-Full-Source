import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.loudspeaker,
    surface.searchapp,
    surface.station,
])
class TestPalmDialogs(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-428
    """

    owners = ('svetlana-yu', )
    search_intents = [intent.Search, intent.Factoid, intent.ObjectAnswer, intent.ObjectSearchOO]

    @pytest.mark.parametrize('command', [
        'кто такой Джон Купер',
        'Что такое гламур',
        'Сколько калорий в пиве',
        'Высота Эвереста',
        'расскажи про значение имени Джоан',
        'Что такое полнолуние',
        'Что такое школа',
        'Когда был основан Лондон',
        'Сколько весит апельсин',
        'Сколько серий в Твин Пиксе',
    ])
    def test_alice_428(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Search
        if surface.is_launcher(alice):
            assert response.intent in self.search_intents or response.intent == intent.Serp
        else:
            assert response.intent in self.search_intents
        assert response.text_card or response.div_card
