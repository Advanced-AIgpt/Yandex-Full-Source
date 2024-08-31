import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.automotive])
class TestPalmLayers(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1586
    """

    owners = ('mihajlova', 'g:maps-auto-crossplatform')

    def test_alice_1586(self, alice):
        response = alice('Включи пробки')
        assert response.intent == intent.NaviShowLayer
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://traffic?traffic_on=1'
        assert response.text in ['Ок, показываю.', 'Сделано.', 'Готово.']

        response = alice('Скрой пробки')
        assert response.intent == intent.NaviHideLayer
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://traffic?traffic_on=0'
        assert response.text in ['Ок, убираю.', 'Сделано.', 'Готово.']

        response = alice('Вруби спутник')
        assert response.intent == intent.NaviShowLayer
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://set_setting?name=rasterMode&value=Sat'
        assert response.text in ['Ок, показываю.', 'Сделано.', 'Готово.']

        response = alice('Покажи парковки')
        assert response.intent == intent.NaviShowLayer
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://show_ui/map?carparks_enabled=1'
        assert response.text in ['Ок, показываю.', 'Сделано.', 'Готово.']

        response = alice('Выключи парковки')
        assert response.intent == intent.NaviHideLayer
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://show_ui/map?carparks_enabled=0'
        assert response.text in ['Ок, убираю.', 'Сделано.', 'Готово.']

        response = alice('Выключи спутник')
        assert response.intent == intent.NaviHideLayer
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://set_setting?name=rasterMode&value=Map'
        assert response.text in ['Ок, убираю.', 'Сделано.', 'Готово.']
