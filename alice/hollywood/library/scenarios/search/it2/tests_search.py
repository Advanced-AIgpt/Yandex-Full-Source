import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice, text
from hamcrest import assert_that, has_entries, only_contains, is_not, empty


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['search']


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.scenario(name='Search', handle='search')
class TestsBase:
    pass


@pytest.mark.parametrize('surface', [surface.searchapp])
@pytest.mark.experiments('search_use_cloud_ui')
@pytest.mark.supported_features('cloud_ui')
class TestsCloudUi(TestsBase):

    @pytest.mark.parametrize('command, has_cloud_ui', [
        pytest.param('что такое ананас', True, id='with_card'),
        pytest.param('поищи котиков', False, id='without_card'),
        pytest.param('озеро байкал', True, id='factoid'),
    ])
    def test_cloud_ui(self, alice, command, has_cloud_ui):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}

        pred = has_entries({
            'response_body': has_entries({
                'layout': has_entries({
                    'directives': only_contains(has_entries({
                        'open_uri_directive': has_entries({
                            'screen_id': 'cloud_ui',
                            'uri': is_not(empty()),
                        }),
                    })),
                }),
            }),
        })
        if not has_cloud_ui:
            pred = is_not(pred)

        assert_that(r.run_response_pyobj, pred)


@pytest.mark.experiments('websearch_cgi_GARAGE_crabik=1')
@pytest.mark.experiments('websearch_cgi_GARAGE_alice_gifts_arrows=1')
@pytest.mark.experiments('websearch_cgi_srcparams=FASTRES2=WizardsEnabled/GARAGE_alice_gifts=1')
@pytest.mark.experiments('websearch_cgi_GARAGE_alice_gifts=1')
class TestsAliceGifts(TestsBase):

    @pytest.mark.parametrize('command', [
        pytest.param('что можно подарить другу на новый год', id='gift1'),
    ])
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_alice_gifts(self, alice, command):
        r = alice(text(command))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'serp'

    @pytest.mark.parametrize('command', [
        pytest.param('что можно подарить другу на новый год', id='gift1'),
    ])
    @pytest.mark.parametrize('surface', [surface.station])
    def test_alice_gifts_unsupported(self, alice, command):
        r = alice(text(command))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent != 'serp'


@pytest.mark.parametrize('surface', [surface.webtouch])
@pytest.mark.experiments('search_use_cloud_ui')
class TestsWebtouch(TestsBase):
    def test_open_url(self, alice):
        r = alice(voice('озеро байкал'))
        assert r.scenario_stages() == {'run'}

        pred = has_entries({
            'response_body': has_entries({
                'layout': has_entries({
                    'directives': only_contains(has_entries({
                        'open_uri_directive': has_entries({
                            'screen_id': 'cloud_ui',
                            'uri': is_not(empty()),
                        }),
                    })),
                }),
            }),
        })

        assert_that(r.run_response_pyobj, pred)
