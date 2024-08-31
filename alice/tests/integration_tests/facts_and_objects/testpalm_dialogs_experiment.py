import re

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments('read_factoid_source')
class TestPalmDialogsExp(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1675
    """

    owners = ('svetlana-yu',)

    @pytest.mark.parametrize('surface', [
        surface.loudspeaker,
        surface.station,
    ])
    @pytest.mark.parametrize('command', [
        'Расскажи мне где живут слоны',
        'Кто изобрел электричество',
        'Почему люки круглые а не квадратные',
        'Что такое ураган',
    ])
    def test_alice_1675_proto_search(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid
        assert any(re.search(pattern, response.text) for pattern in [
            'В интернете пишут:',
            'Вот, что нашла в сети:',
            r'Если что, это не моё. Читала с сайта .*\.',
            'На .* есть такой ответ:',
            'Нашла в интернете:',
            'Нашла ответ на .*:',
            'Нашлось в интернете:',
            r'Ответ есть на .*\. Читаю:',
            'Ответ из интернета:',
            'Ответ:',
            r'Ответ нашла на .*\.',
            'Отвечает .*:',
            'Сайт .* даёт такой ответ:',
            r'Это на .* написано\.',
            r'Это с сайта .*, если что\.',
        ]), f'No match in response "{response.text}"'
