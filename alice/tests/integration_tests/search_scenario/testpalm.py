from urllib.parse import unquote

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest
from alice.tests.library.vins_response import DivWrapper


class SearchDivCard(DivWrapper):
    @property
    def title(self):
        return self.data[1].title.lower()


@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
    surface.yabro_win,
])
class TestPalmSearchScenario(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1089
    """

    owners = ('zhigan', )

    search_responses = [
        'Ищу ответ',
        'Найдётся всё!',
        'Ищу в Яндексе',
        'Сейчас найду',
        'Сейчас найдём',
        'Одну секунду...',
        'Открываю поиск',
        'Ищу для вас ответ',
        'Давайте поищем',
    ]

    def test_alice_1089(self, alice):
        text_search_request = 'зеленые чебурашки'
        response = alice(f'Поищи в Яндексе {text_search_request}')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Serp
        assert response.directive.name == directives.names.OpenUriDirective
        assert f'text={text_search_request}' in unquote(response.directive.payload.uri)

        assert response.text in self.search_responses
        assert len(response.buttons) == 1
        assert response.button('Поискать в Яндексе')

        receipt = 'рецепт цветаевского пирога'
        response = alice(f'Покажи мне {receipt}')
        assert response.scenario == scenario.Search
        assert response.intent in [intent.Serp, intent.Factoid]

        if response.intent == intent.Factoid:
            search_card = SearchDivCard(response.div_card)
            assert all(item in search_card.title for item in ['яблочн', 'пирог'])

        if response.intent == intent.Serp:
            assert response.directive.name == directives.names.OpenUriDirective
            assert f'text={receipt}' in unquote(response.directive.payload.uri)
            assert response.text in self.search_responses
            assert response.button('Поискать в Яндексе')
