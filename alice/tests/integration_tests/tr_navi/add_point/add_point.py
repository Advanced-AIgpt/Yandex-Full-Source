import re

import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments(f'mm_enable_protocol_scenario={scenario.AddPointTr}')
@pytest.mark.parametrize('surface', [surface.navi_tr])
class TestAddPointTr(object):

    owners = ('flimsywhimsy', )

    common_text = (
        r'(Tamamdır, kaydediyorum.|'
        r'Bilgilendirme için teşekkürler.|'
        r'Teşekkürler.|'
        r'Yol bilgisi için teşekkürler.|'
        r'Yardımcı olduğun için teşekkür ederim.|'
        r'Yardımın için teşekkürler.|'
        r'Verdiğin bilgi için teşekkürler.)'
    )

    error_re = re.compile(rf'^{common_text} Hata$')

    traffic_accidents_re = re.compile(rf'^{common_text} Kaza$')

    road_works_re = re.compile(rf'^{common_text} Yol çalışması$')

    camera_re = re.compile(rf'^{common_text} Kamera$')

    @pytest.mark.parametrize('command, response_re', [
        ('sağ şerit trafik kazası yaralı var', traffic_accidents_re),  # ДТП на правой полосе, есть пострадавшие
        ('çift şerit yol çalışması', road_works_re),  # Дорожные работы на всех полосах
        ('boğaziçi köprüsü kapalı avrasya maratonu', error_re),  # Евразийский марафон с Босфорского моста
        ('Yolda kamera', camera_re),  # На дороге камера
        # TODO: Enable with Hollywood (common) ver.27
        # ('burda asfalt tamiri var', road_works_re),  # Ремонт асфальта
    ])
    def test_add_point_tr(self, alice, command, response_re):
        response = alice(command)
        assert response.scenario == scenario.AddPointTr
        assert response_re.match(response.text) is not None
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('yandexnavi://add_point?')
