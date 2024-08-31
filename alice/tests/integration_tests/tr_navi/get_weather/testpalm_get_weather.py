import re
from typing import List

import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def parse_temperature(temp_reg: str, response: str) -> List[int]:
    return [int(_) for _ in re.findall(temp_reg, response)]


def has_positive_order(temps: List[int]) -> bool:
    return temps[0] < temps[1] and temps[1] >= 0


def has_negative_order(temps: List[int]) -> bool:
    return temps[1] < temps[0] < 0


def check_temperature_range(temps: List[int]) -> None:
    assert len(temps) in [1, 2]
    assert len(temps) == 1 or has_positive_order(temps) or has_negative_order(temps)


@pytest.mark.parametrize('surface', [surface.navi_tr])
@pytest.mark.experiments(f'mm_enable_protocol_scenario={scenario.GetWeatherTr}')
class TestPalmGetWeatherTr(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2105
    """

    owners = ('flimsywhimsy', )

    def test(self, alice):
        temp_reg = r'(0|\+\d+|-\d+)'
        temp_range_reg = rf'{temp_reg}( ile {temp_reg})?'

        response = alice('Hava nasıl?')  # 'Какая погода?'
        assert response.scenario == scenario.GetWeatherTr
        assert re.match(rf'Şimdi Moskova\'da {temp_reg} derece, ', response.text) is not None

        response = alice('Hava bugün nasıl?')  # 'Какая погода сегодня?'
        assert response.scenario == scenario.GetWeatherTr
        assert re.match(rf'Bugün Moskova\'da {temp_reg} derece, ', response.text) is not None

        response = alice('Yarın hava durumu ne?')  # 'Какая погода завтра?'
        assert response.scenario == scenario.GetWeatherTr
        assert re.match(rf'Yarın Moskova\'da {temp_range_reg} derece, ', response.text) is not None
        check_temperature_range(
            parse_temperature(temp_reg, response.text)
        )

        response = alice('Haftasonu hava nasıl olacak?')  # 'Какая будет погода в выходные?'
        assert response.scenario == scenario.GetWeatherTr
        expected_response = (
            rf'Şimdi Moskova\'da {temp_reg} derece, '
            if alice.datetime_now.weekday() == 6
            else rf'Hafta sonu Moskova\'da {temp_range_reg} derece, '
        )
        assert re.match(expected_response, response.text) is not None
        check_temperature_range(
            parse_temperature(temp_reg, response.text)
        )
