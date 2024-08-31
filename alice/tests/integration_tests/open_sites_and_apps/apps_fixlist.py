import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('command, expected_text', [
    ('я хочу есть', 'приложение «Яндекс.Еда»'),
    ('купи бургер', 'приложение «Яндекс.Еда»'),
    ('закажи пиццу', 'приложение «Яндекс.Еда»'),
    ('доставь еды', 'приложение «Яндекс.Еда»'),
    ('привези еду', 'приложение «Яндекс.Еда»'),
    ('настройка утреннего шоу', 'страницу настройки шоу Алисы'),
    ('вруби настройку шоу', 'страницу настройки шоу Алисы'),
    ('настрой шоу с Алисой', 'страницу настройки шоу Алисы'),
    ('покажи настройки утреннего шоу Алисы', 'страницу настройки шоу Алисы'),
    ('открывай настройку утреннего шоу', 'страницу настройки шоу Алисы'),
    ('настрой колонку', 'настройки устройств'),
    ('настройка новой станции', 'настройки устройств'),
    ('настроить яндекс мини', 'настройки устройств'),
    ('настроить новую джейбиэль', 'настройки устройств'),
    ('покажи мои штрафы', 'штрафы'),
])
@pytest.mark.experiments(
    'bg_fresh_granet_form=alice.apps_fixlist',
    'enable_ya_eda_fixlist',
    'enable_morning_show_settings_fixlist',
    'enable_quasar_settings_fixlist',
)
class TestGranetAppsFixlist(object):
    owners = ('tolyandex', 'yagafarov')

    @pytest.mark.parametrize('surface', [surface.station])
    def test_station(self, alice, command, expected_text):
        response = alice(command)
        assert response.intent.startswith(intent.ShortcutPrefix)
        assert response.scenario == scenario.OpenAppsFixlist
        assert expected_text in response.text

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_searchapp(self, alice, command, expected_text):
        response = alice(command)
        assert response.intent.startswith(intent.ShortcutPrefix)
        assert response.scenario == scenario.OpenAppsFixlist

        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.FillCloudUiDirective
        assert response.directives[1].name == directives.names.OpenUriDirective


@pytest.mark.version(hollywood=211)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestCloudUiAppsFixlist(object):
    owners = ('zhigan',)

    def test_show_secret(self, alice):
        response = alice('покажи секретный промокод')
        assert response.scenario == scenario.OpenAppsFixlist
        assert response.intent == intent.ShortcutTvSecretPromo

        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.FillCloudUiDirective
        assert response.directives[1].name == directives.names.OpenUriDirective
