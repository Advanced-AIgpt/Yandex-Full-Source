import re

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


class TranslateSpeeds(object):
    DefaultValue = 0.9
    MinValue = 0.6
    MaxValue = 1.2


def _extract_speed(voice_response):
    speed_pattern = re.search('.*speed=\"(.*)\".*', voice_response)
    speed = None
    if speed_pattern:
        speed = speed_pattern.group(1)
    return speed


def _assert_speed(response, speed):
    voice_response = response.slots['voice'].string

    assert _extract_speed(voice_response) == speed
    suggest_titles = {suggest.title for suggest in response.suggests}
    assert ('Быстрее' in suggest_titles) == (float(speed) < TranslateSpeeds.MaxValue)
    assert ('Медленнее' in suggest_titles) == (float(speed) > TranslateSpeeds.MinValue)


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    surface.loudspeaker,
    surface.navi,
    surface.searchapp,
    surface.station,
    surface.watch,
])
class TestPalmTranslateSpeed(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1876
    https://testpalm.yandex-team.ru/testcase/alice-1877
    https://testpalm.yandex-team.ru/testcase/alice-1878
    """

    owners = ('sparkle',)

    @pytest.mark.parametrize('command, speeds, target_intents', [
        ('быстрее', ['1', '1.1', '1.2', '1.2', '1.2'], {intent.ProtocolTranslate, intent.TranslateQuicker}),
        ('медленнее', ['0.8', '0.7', '0.6', '0.6', '0.6'], {intent.ProtocolTranslate, intent.TranslateSlower}),
    ])
    def test(self, alice, command, speeds, target_intents):
        response = alice('как будет один два три четыре пять по-английски')
        assert response.scenario == scenario.Translation
        assert response.intent == intent.ProtocolTranslate
        assert response.slots['result'].string == 'one two three four five'
        _assert_speed(response, str(TranslateSpeeds.DefaultValue))

        for speed in speeds:
            response = alice(command)
            _assert_speed(response, speed)
            assert response.intent in target_intents
