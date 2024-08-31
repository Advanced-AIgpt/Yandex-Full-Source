import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['search', 'open_apps_fixlist']


@pytest.mark.scenario(name='OpenAppsFixlist', handle='open_apps_fixlist')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsOpenAppsFixlist:

    @pytest.mark.parametrize('command, expected_intent', [
        pytest.param('Открой читалку', 'shortcut.reader.open', id='reader'),
        pytest.param('Открой мой QR код для вакцинации', 'shortcut.covid_qr', id='covid_qr'),
    ])
    @pytest.mark.supported_features('reader_app')
    @pytest.mark.supported_features('covid_qr')
    def test_intent_in_analytics(self, alice, command, expected_intent):
        response = alice(voice(command))
        assert response.run_response.ResponseBody.AnalyticsInfo.Intent == expected_intent

    @pytest.mark.supported_features('cloud_ui')
    def test_close_dialog_directive(self, alice):
        response = alice(voice('Открой мои штрафы пожалуйста'))
        have_close_dialog_directive = False
        for directive in response.run_response.ResponseBody.Layout.Directives:
            if directive.HasField('CloseDialogDirective'):
                assert directive.CloseDialogDirective.Name == 'open_apps_fixlist_close_dialog'
                assert directive.CloseDialogDirective.ScreenId == 'cloud_ui'
                have_close_dialog_directive = True
        assert have_close_dialog_directive, 'Not found CloseDialogDirective'

    @pytest.mark.unsupported_features('cloud_ui')
    def test_close_dialog_directive_unsupported(self, alice):
        response = alice(voice('Открой мои штрафы пожалуйста'))
        for directive in response.run_response.ResponseBody.Layout.Directives:
            assert not directive.HasField('CloseDialogDirective')
