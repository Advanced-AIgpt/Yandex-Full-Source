import re

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def _assert_slots(response, is_english):
    slots = response.slots
    assert slots['lang_src'].string == 'русский'
    assert slots['text'].string == 'стол'
    assert (slots['lang_dst'].string == 'английский') == is_english
    assert (slots['result'].string == 'table') == is_english


@pytest.mark.parametrize('surface', [surface.station])
class TestPalmTranslateStationUI(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1060
    """

    owners = ('sparkle',)

    def test(self, alice):
        response = alice('переведи стол на английский')
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate
        _assert_slots(response, is_english=True)
        first_reply = response.text

        suggests = {suggest.title for suggest in response.suggests}
        assert {'Повтори', 'Быстрее', 'Медленнее'}.issubset(suggests)

        translate_suggests = [suggest for suggest in suggests if re.match(r'А на .*\?', suggest)]
        assert len(translate_suggests) == 3
        assert len(suggests) == 6

        response = alice('повтори')
        assert response.scenario == scenario.Repeat
        assert response.intent == intent.Repeat
        assert response.text == first_reply

        response = alice(translate_suggests[0])
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate
        _assert_slots(response, is_english=False)
