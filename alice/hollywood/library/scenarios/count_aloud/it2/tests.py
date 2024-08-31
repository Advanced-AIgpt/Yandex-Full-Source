import random

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


COUNT_ALOUD_FRAME = 'alice.count_aloud'


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['count_aloud']


def _check(response, count_from, count_to, cropped=False):
    frame = response.run_response.ResponseBody.SemanticFrame
    assert frame.Name == COUNT_ALOUD_FRAME

    output_speech = response.run_response.ResponseBody.Layout.OutputSpeech

    rnd_num = ' ' + str(random.randint(min(count_from, count_to) + 1, max(count_from, count_to) - 1)) + ','
    count_from = str(count_from) + ','
    count_to = ' ' + str(count_to)
    if cropped:
        assert ' 100 ' in output_speech  # 100 is current limit
    assert count_from in output_speech
    assert count_to in output_speech
    assert rnd_num in output_speech
    assert output_speech.find(count_from) <= output_speech.find(rnd_num) <= output_speech.find(count_to)


@pytest.mark.scenario(name='CountAloud', handle='count_aloud')
@pytest.mark.parametrize('surface', surface.actual_surfaces)
@pytest.mark.experiments(f'bg_fresh_alice_form={COUNT_ALOUD_FRAME}')
class TestsCountAloud(object):
    def test_default(self, alice):
        resp = alice(voice('посчитай вслух'))
        _check(resp, count_from=1, count_to=10)
        return str(resp)

    def test_from(self, alice):
        resp = alice(voice('посчитаем с 13 в голос'))
        _check(resp, count_from=13, count_to=23)

    def test_from_backward(self, alice):
        resp = alice(voice('можешь научить меня считать с 7 по убыванию'))
        _check(resp, count_from=7, count_to=1)

    def test_to(self, alice):
        resp = alice(voice('алиса досчитай до 5 пока я прячусь'))
        _check(resp, count_from=1, count_to=5)

    def test_to_backward(self, alice):
        resp = alice(voice('запусти счет до 5 в обратном порядке'))
        _check(resp, count_from=5, count_to=1)

    def test_from_to(self, alice):
        resp = alice(voice('посчитай вместе со мной от 2 до 7 сейчас'))
        _check(resp, count_from=2, count_to=7)

    def test_from_to_large(self, alice):
        resp = alice(voice('посчитай от 2 до 1000'))
        _check(resp, count_from=2, count_to=101, cropped=True)

    def test_from_to_backward(self, alice):
        resp = alice(voice('сосчитай мне от 7 до 2 громко пожалуйста'))
        _check(resp, count_from=7, count_to=2)

    def test_countdown(self, alice):
        resp = alice(voice('давай обратный отсчет до нуля'))
        _check(resp, count_from=10, count_to=0)
