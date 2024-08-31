import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


SEARCH_INTENTS = [intent.Factoid, intent.ObjectAnswer, intent.Calculator, intent.Search, intent.ObjectSearchOO]


@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.station,
])
class TestSearchFacts(object):

    owners = ('tolyandex', 'svetlana-yu')

    @pytest.mark.parametrize('command', [
        'расскажи какая калорийность у яблока',
        'кто такой джон купер',
        'сколько будет 10 плюс 10',
        'сколько будет 10 поделить на 3',
        '15 километров в мили',
        'сколько лет наполеону',
        'Какой счет у ливерпуля',
        'разница во времени между москвой и лондоном',
        'штат Мичиган',
        'Кто такая фрея из скайрима',
        'курс акций яндекса',
    ])
    def test(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Search
        assert response.intent in SEARCH_INTENTS


@pytest.mark.experiments(
    'websearch_cgi_rearr=entsearch_experiment=alice_meta',
    'websearch_cgi_ento=0oCgVydXc3MRgCQgzQvNC-0YHQutCy0LBL35YD',
    'force_search_goodwin',
    f'mm_scenario={scenario.Search}'
)
@pytest.mark.parametrize('surface', [surface.station])
class TestSearchPostrolls(object):

    owners = ('afattakhov', )

    def test(self, alice):
        response = alice('расскажи про москву')
        assert response.scenario == scenario.Search
        assert response.intent == intent.ObjectSearchOO

        for _ in range(2):
            response = alice('ещё')
            assert response.scenario == scenario.Search
            assert response.intent == intent.ObjectSearchOO


@pytest.mark.experiments('search_use_cloud_ui')
@pytest.mark.supported_features('cloud_ui')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestSearchCloudUi(object):

    owners = ('sparkle', )

    def test_open_uri(self, alice):
        response = alice('что такое ананас')
        assert response.scenario == scenario.Search
        assert response.intent in SEARCH_INTENTS

        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri
        assert response.directive.payload.screen_id == 'cloud_ui'
