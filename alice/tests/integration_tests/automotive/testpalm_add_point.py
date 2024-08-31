import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


class PointCategory:
    CRUSH = 0
    ROADWORKS = 1
    CAMERA = 2


@pytest.mark.parametrize('surface', [surface.automotive])
class TestPalmAddPoint(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1591
    https://testpalm.yandex-team.ru/testcase/alice-1592
    https://testpalm.yandex-team.ru/testcase/alice-1593
    """

    owners = ('yagafarov')

    def _assert_response(self, response, utterance, category):
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith(f'yandexnavi://add_point?category={category}')
        assert utterance.lower() in response.text.lower()

    @pytest.mark.parametrize('command', [
        'авария',
        'дтп',
        'столкновение',
        'автомобиль сломался',
        'дорогу не поделили',
        'притерлись',
        'догнал',
        'стукнулись',
        'вошли в клинч',
        'заварушка',
        'на аварийке, влетел',
    ])
    @pytest.mark.parametrize('row', ['в левом', 'в правом', 'в среднем'])
    def test_alice_1591(self, alice, command, row):
        response = alice(command)
        assert response.intent == intent.AddPoint
        assert 'ряд' in response.text.lower()

        response = alice(row)
        self._assert_response(response, 'дтп', PointCategory.CRUSH)

        response = alice(f'впереди авария {row} ряду')
        self._assert_response(response, 'дтп', PointCategory.CRUSH)

    @pytest.mark.parametrize('command', [
        'Дорожные работы',
        'ремонт дороги',
        'ремонт моста',
        'ремонтные работы',
        'перекопано',
        'перекопали',
        'реконструкция',
        'раскопки',
        'разрыли',
        'золото ищут',
        'роют',
        'копают',
        'нефть ищут',
        'искромсали дорогу',
    ])
    @pytest.mark.parametrize('row', ['в левом', 'в правом', 'в среднем'])
    def test_alice_1592(self, alice, command, row):
        response = alice(command)
        assert response.intent == intent.AddPoint
        assert 'ряд' in response.text.lower()

        response = alice(row)
        self._assert_response(response, 'дорожные работы', PointCategory.ROADWORKS)

        response = alice(f'впереди дорожные работы {row} ряду')
        self._assert_response(response, 'дорожные работы', PointCategory.ROADWORKS)

    @pytest.mark.parametrize('command', [
        'камера',
        'камера в левом ряду',
        'дпс',
        'гаи',
        'менты',
        'гайцы',
        'гаишники',
        'депсы',
        'тринога',
        'радар',
        'мент',
        'копы',
        'засада',
        'ловят',
        'экипаж',
        'облава',
        'гибдд',
        'караулят',
        'полиция',
        'фотографируют',
        'светят',
        'светимся',
    ])
    def test_alice_1593(self, alice, command):
        response = alice(command)
        self._assert_response(response, 'камера', PointCategory.CAMERA)
