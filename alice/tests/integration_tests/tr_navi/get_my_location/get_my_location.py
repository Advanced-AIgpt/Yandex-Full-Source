import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.navi_tr])
class TestGetMyLocationTr(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2101
    """

    owners = ('ardulat', 'zhigan')

    @pytest.mark.parametrize('command', [
        'ben nerdeyim',
        'neredeyim',
        'şuanda nerdeyiz',
        'şuan nerdeyim',
        'konumumu göster',
        'bulunduğum yer',
    ])
    def test_alice_2101(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.GetMyLocationTr
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://show_user_position'
        assert response.text.startswith((
            'Şu anki konumumuzu gösteriyorum',
            'Konumumuz tam burada',
            'Şu anda buradasın',
            'Tam olarak şurada bulunuyoruz',
            'İşte bulunduğumuz yer',
            'İşte tam buradayız',
            'Haritada bulunduğumuz yeri gösteriyorum'
        ))
        assert 'Moskova, ulitsa Lva Tolstogo, No:16, Rusya.' in response.text
