import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.navi_tr])
class TestSwitchLayerTr(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2095
    """

    owners = ('flimsywhimsy', )

    responses = {
        'İşte karşında.',
        'Tamamdır, bak bakalım.',
        'Hemen gösteriyorum.',
        'Tamamdır, gösteriyorum.',
        'İşte burada.',
        'Buyursunlar efendim.',
    }

    @pytest.mark.parametrize('command, traffic_on', [
        ('alisa sıkışıklığı', '1'),  # Показать слой с дорожным трафиком
        ('alisa yoğunluğunu kapatır mısın', '0'),  # Скрыть слой с дорожным трафиком
        ('Trafik katmanını aç', '1'),
        ('Trafiği kapat', '0'),
    ])
    def test_alice_2095(self, alice, command, traffic_on):
        response = alice(command)
        assert response.scenario == scenario.SwitchLayerTr
        assert response.text in self.responses
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == f"yandexnavi://traffic?traffic_on={traffic_on}"
