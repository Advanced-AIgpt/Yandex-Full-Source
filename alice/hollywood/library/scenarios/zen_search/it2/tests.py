import urllib

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice, server_action


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['zen_search']


@pytest.mark.scenario(name='ZenSearch', handle='zen_search')
class Tests:
    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command, search', [
        pytest.param('алиса открой дзен про еду', 'еду', id='1Word'),
        pytest.param('доброе утро найди в яндекс дзене космические полеты', 'космические полеты', id='2Words')
    ])
    def test(self, alice, command, search):
        r = alice(voice(command))

        encodedSearch = urllib.parse.quote(search)
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Cards[0].Text
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == \
            'zen://open_feed?export_params=alice_search%3D{}&scroll_to_zen=1&zenf=alice'.format(encodedSearch.replace('%20', '+'))

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.parametrize('command', [
        pytest.param('алиса открой дзен про еду', id='station'),
    ])
    def test_on_station(self, alice, command):
        r = alice(voice(command))

        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Cards[0].Text

        assert len(r.run_response.ResponseBody.Layout.Directives) == 0

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command, search', [
        pytest.param('котиков', 'котиков', id='context'),
        pytest.param('найди котиков', 'котиков', id='contextSearch'),
    ])
    def test_context(self, alice, command, search):
        payload = {
            'typed_semantic_frame': {
                'zen_context_search_start_semantic_frame': {
                },
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'zen-search',
            },
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'run'}

        r = alice(voice(command))

        encodedSearch = urllib.parse.quote(search)
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.Cards[0].Text
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri == \
            'zen://open_feed?export_params=alice_search%3D{}&scroll_to_zen=1&zenf=alice'.format(encodedSearch.replace('%20', '+'))
