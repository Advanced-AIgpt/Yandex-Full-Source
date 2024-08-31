import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestSearchSuggest(object):

    owners = ('tolyandex', )

    serp_phrases = [
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

    def test_serp(self, alice):
        response = alice('еби доеби')

        search_suggest = response.suggest('🔍 "еби доеби"')
        assert search_suggest
        response = alice.click(search_suggest)

        assert response.scenario == scenario.Search
        assert response.intent == intent.Serp
        assert response.text in self.serp_phrases
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'Поискать в Яндексе' == response.buttons[0].title

    def test_fact(self, alice):
        response = alice('ленинградский проспект')

        search_suggest = response.suggest('🔍 "ленинградский проспект"')
        assert search_suggest
        response = alice.click(search_suggest)

        assert response.scenario == scenario.Search
        assert response.intent in [intent.Factoid, intent.ObjectAnswer, intent.ObjectSearchOO]
